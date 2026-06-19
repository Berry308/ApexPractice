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
}


void UBulletSimulationProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
    EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);
    EntityQuery.AddRequirement<FBulletSimulationFragment>(EMassFragmentAccess::ReadWrite);
    EntityQuery.AddRequirement<FBulletHitFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FBulletVisionFragment>(EMassFragmentAccess::ReadWrite);
	//不具有FBulletHitTag标签的实体才能被BulletHitProcessor处理
    EntityQuery.AddTagRequirement<FBulletHitTag>(EMassFragmentPresence::None);
}

void UBulletSimulationProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
    EntityQuery.ForEachEntityChunk(EntityManager, Context,
        [this](FMassExecutionContext& IterContext)
        {
            auto Transforms = IterContext.GetMutableFragmentView<FTransformFragment>();
            auto Sims = IterContext.GetMutableFragmentView<FBulletSimulationFragment>();
            auto Hits = IterContext.GetMutableFragmentView<FBulletHitFragment>();
			auto Visions = IterContext.GetMutableFragmentView<FBulletVisionFragment>();

            //*控制模拟精度
            const float WorldDeltaTime = IterContext.GetDeltaTimeSeconds();
			const float TargetInterval = 0.032f; // 30Hz的检测频率
            // 获取该块的计时器
            FBulletSimTimerChunkFragment& Timer = IterContext.GetMutableChunkFragment<FBulletSimTimerChunkFragment>();
            Timer.TimeAccumulator += WorldDeltaTime;
            //如果累积的时间还没有达到目标间隔，跳过模拟逻辑更新
            if (Timer.TimeAccumulator < TargetInterval)
            {
                return;
            }
            //重置该块的模拟计时器
			Timer.TimeAccumulator = 0.0f;

			//*遍历块内实体，更新每个子弹的模拟逻辑
            for (int32 i = 0; i < IterContext.GetNumEntities(); ++i)
            {
				//根据当前速度、方向和重力计算子弹的下一个位置
				FVector Start = ;
				Sims[i].Velocity += Sims[i].Gravity * IterContext.GetDeltaTimeSeconds();//在当前速度基础上叠加重力影响
                FVector End = Start + (Sims[i].Velocity * IterContext.GetDeltaTimeSeconds());

				// 扫描检测，如果命中，记录命中信息并打上Tag
                FHitResult HitResult;
                if (GetWorld()->SweepSingleByChannel(HitResult, 
                    Start, End,
                    FQuat::Identity, ECC_GameTraceChannel1, 
                    FCollisionShape::MakeSphere(Sims[i].CollisionRadius)))
                {
                    //记录命中信息
                    Hits[i].TargetActor = HitResult.GetActor();
                    Hits[i].HitLocation = HitResult.ImpactPoint;
                    //计算从模拟起点到命中的时间
					float TotalDis = (End - Start).Size();//如果未命中物体理论上移动的距离
					float MoveDis = (HitResult.ImpactPoint - Start).Size();
					float TimeToHit = FMath::Clamp(MoveDis / TotalDis, 0.0f, 1.0f);
					Hits[i].TimeToApplyDamage = TimeToHit;
                    //给实体打上 Tag，交给BulletHitProcessor 处理
                    IterContext.Defer().AddTag<FBulletHitTag>(IterContext.GetEntity(i));
					//更新视觉片段的目标位置为命中点
					Visions[i].TargetLocation = HitResult.ImpactPoint;
                }
                else//如果没有命中
                {
					Visions[i].TargetLocation = End;
                }
                //*Transforms[i].GetMutableTransform().SetLocation(End);
            }
        }
    );
}
