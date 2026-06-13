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
    //只有还没命中的才需要模拟
    EntityQuery.AddTagRequirement<FBulletHitTag>(EMassFragmentPresence::None);
}

void UBulletSimulationProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
    EntityQuery.ForEachEntityChunk(EntityManager, Context, [this](FMassExecutionContext& IterContext)
        {
            auto Transforms = IterContext.GetMutableFragmentView<FTransformFragment>();
            auto Sims = IterContext.GetMutableFragmentView<FBulletSimulationFragment>();
            auto Hits = IterContext.GetMutableFragmentView<FBulletHitFragment>();

            for (int32 i = 0; i < IterContext.GetNumEntities(); ++i)
            {
                // 模拟位移、重力
                FVector Start = Transforms[i].GetTransform().GetLocation();
                FVector End = Start + (Sims[i].Direction * Sims[i].Velocity * Sims[i].Gravity * IterContext.GetDeltaTimeSeconds());

                // 物理扫描
                FHitResult HitResult;
                if (GetWorld()->SweepSingleByChannel(HitResult, 
                    Start, End,
                    FQuat::Identity, ECC_GameTraceChannel1, 
                    FCollisionShape::MakeSphere(Sims[i].CollisionRadius)))
                {
                    //记录命中信息
                    Hits[i].TargetActor = HitResult.GetActor();
                    Hits[i].HitLocation = HitResult.ImpactPoint;

                    //给实体打上 Tag，交给BulletHitProcessor 处理
                    IterContext.Defer().AddTag<FBulletHitTag>(IterContext.GetEntity(i));
                }
                Transforms[i].GetMutableTransform().SetLocation(End);
            }
        });
}
