// Fill out your copyright notice in the Description page of Project Settings.


#include "Mass/Bullet/BulletSimulationProcessor.h"
#include "Mass/Bullet/BulletFragments.h"
#include "MassCommonFragments.h"
#include "MassExecutionContext.h"

UBulletSimulationProcessor::UBulletSimulationProcessor()
{
	ProcessingPhase = EMassProcessingPhase::DuringPhysics;
    // 关键点：如果你的伤害逻辑涉及调用 Actor 函数（TakeDamage），
    // 建议开启此项以确保在主线程运行，或者在执行时进行线程安全检查。
    //bRequiresGameThreadExecution = true;
    RegisterQuery(EntityQuery);
}


void UBulletSimulationProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.RegisterWithProcessor(*this);
    //EntityQuery.AddRequirement<FBulletHitFragment>(EMassFragmentAccess::ReadWrite);
    EntityQuery.AddRequirement<FBulletSimulationFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FBulletVisionFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddChunkRequirement<FBulletSimTimerChunkFragment>(EMassFragmentAccess::ReadWrite);
    EntityQuery.AddTagRequirement<FBulletHitTag>(EMassFragmentPresence::None);//不具有FBulletHitTag标签的实体才能被BulletHitProcessor处理
}

void UBulletSimulationProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
    EntityQuery.ForEachEntityChunk( Context,
        [this](FMassExecutionContext& IterContext)
        {
            //UE_LOG(LogTemp, Log, TEXT("BulletSimulationProcessor Execute"));
            //控制模拟精度
            bool bChunkNeedUpdate = false;
            const float WorldDeltaTime = IterContext.GetDeltaTimeSeconds();
			const float TargetInterval = 0.032f; // 30Hz的检测频率
            FBulletSimTimerChunkFragment& Timer = IterContext.GetMutableChunkFragment<FBulletSimTimerChunkFragment>();
            Timer.TimeAccumulator += WorldDeltaTime;
            if (Timer.TimeAccumulator >= TargetInterval) //如果累积的时间达到目标间隔
            {
                bChunkNeedUpdate = true;
                Timer.TimeAccumulator = 0.0f;//重置该块的模拟计时器
            }

            //UE_LOG(LogTemp, Log, TEXT("UBulletSimulationProcessor query chunk,dirty status: %d"), Timer.bIsChunkDirty);
            //如果块不包含脏标记且bChunkNeedUpdate为false，跳过该块的模拟
            //if (!Timer.bIsChunkDirty && !bChunkNeedUpdate)
            //{
            //    return;
            //}

            auto Sims = IterContext.GetMutableFragmentView<FBulletSimulationFragment>();
            auto Visions = IterContext.GetMutableFragmentView<FBulletVisionFragment>();
            //auto Hits = IterContext.GetMutableFragmentView<FBulletHitFragment>();

			//遍历块内实体，更新每个子弹的模拟逻辑
            for (int32 i = 0; i < IterContext.GetNumEntities(); ++i)
            {
                //如果当前实体不需要第一次模拟 并且 当前块不需要更新
                if (!Sims[i].bNeedFirstSim && !bChunkNeedUpdate) continue;
                Sims[i].bNeedFirstSim = false;
                //判断子弹剩余存活时间
                if (Sims[i].RemainingLifeTime <= 0.0f)
                {
					IterContext.Defer().DestroyEntity(IterContext.GetEntity(i));//如果子弹存活时间耗尽，销毁实体
                    continue;
                }
                //UE_LOG(LogTemp, Log, TEXT("UBulletSimulationProcessor::Execute single entity"));
				//根据当前速度、方向和重力计算子弹的下一个位置
				FVector Start = Sims[i].CurrentLocation;
				FVector Velocity = Sims[i].Velocity + Sims[i].Gravity * IterContext.GetDeltaTimeSeconds();//在当前速度基础上叠加重力影响
                FVector End = Start + (Velocity * IterContext.GetDeltaTimeSeconds());

				Sims[i].RemainingLifeTime -= IterContext.GetDeltaTimeSeconds();//减少子弹剩余存活时间
				Sims[i].Velocity = Velocity;//更新子弹速度
				Sims[i].CurrentLocation = End;//更新子弹位置

				// 扫描检测，如果命中，打上Tag并添加HitFragment
                FHitResult HitResult;
				FVector TargetLocation;
                FCollisionQueryParams queryParams;

                queryParams.AddIgnoredActors(Sims[i].ActorToIgnore);
                queryParams.bReturnPhysicalMaterial = true;
                if (GetWorld()->SweepSingleByChannel(HitResult, 
                    Start, End,
                    FQuat::Identity, ECC_GameTraceChannel1, 
                    FCollisionShape::MakeSphere(Sims[i].CollisionRadius), queryParams)
                    )
                {
                    //更新视觉片段的目标位置为命中点
                    TargetLocation = HitResult.ImpactPoint;
                    //计算从模拟起点到命中的时间
					float TotalDis = (End - Start).Size();//如果未命中物体理论上移动的距离
					float MoveDis = (HitResult.ImpactPoint - Start).Size();
					float TimeToHit = FMath::Clamp(MoveDis / TotalDis, 0.0f, 1.0f) * IterContext.GetDeltaTimeSeconds();
					FBulletHitFragment HitFragment;
					HitFragment.InstigatorActor = Sims[i].InstigatorActor;
                    HitFragment.TimeToApplyDamage = TimeToHit;
                    HitFragment.TargetActor = HitResult.GetActor();
                    HitFragment.HitLocation = HitResult.ImpactPoint;
                    //给实体打上Tag并添加HitFragment，交给BulletHitProcessor 处理，注意当前帧不会立马添加FBulletHitTag给相应实体，即后续不会立马执行HitProcessor
                    IterContext.Defer().AddTag<FBulletHitTag>(IterContext.GetEntity(i));
                    IterContext.Defer().PushCommand<FMassCommandAddFragmentInstances>(IterContext.GetEntity(i), HitFragment);

                    //绘制轨迹
                    //DrawDebugLine(GetWorld(), Start, TargetLocation, FColor::Green, false, 5.0f);
                    //DrawDebugSphere(GetWorld(), TargetLocation, Sims[i].CollisionRadius, 8, FColor::Red, false, 5.0f);

                }
                else//如果没有命中
                {
                    TargetLocation = End;
                    //绘制轨迹
                    //DrawDebugLine(GetWorld(), Start, TargetLocation, FColor::Green, false, 5.0f);
                    //DrawDebugSphere(GetWorld(), TargetLocation, Sims[i].CollisionRadius, 8, FColor::Green, false, 5.0f);
                }
				Visions[i].TargetLocation = TargetLocation;//更新视觉片段的目标位置
            }
            //遍历完所有的块内实体后，去除该块的脏标记
            //Timer.bIsChunkDirty = false;
        }
    );
}
