// Fill out your copyright notice in the Description page of Project Settings.


#include "Mass/Bullet/BulletHitProcessor.h"
#include "Mass/Bullet/BulletSimulationProcessor.h"
#include "Mass/Bullet/BulletFragments.h"
#include "MassCommonFragments.h"
#include "MassExecutionContext.h"

UBulletHitProcessor::UBulletHitProcessor()
{
	bRequiresGameThreadExecution = true;
	ProcessingPhase = EMassProcessingPhase::DuringPhysics;
	ExecutionOrder.ExecuteAfter.Add(UBulletSimulationProcessor::StaticClass()->GetFName());
}

void UBulletHitProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FBulletHitFragment>(EMassFragmentAccess::ReadOnly);
	// 核心优化：只查询带 HitTag 的实体
	EntityQuery.AddTagRequirement<FBulletHitTag>(EMassFragmentPresence::All);
}

void UBulletHitProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
    EntityQuery.ForEachEntityChunk(EntityManager, Context, 
        [this](FMassExecutionContext& IterContext){
            auto Hits = IterContext.GetMutableFragmentView<FBulletHitFragment>();

            for (int32 i = 0; i < IterContext.GetNumEntities(); ++i)
            {
				Hits[i].DamageTimer += IterContext.GetDeltaTimeSeconds();
                if(Hits[i].DamageTimer < Hits[i].TimeToApplyDamage)
                {
                    continue;
				}
                AActor* Target = Hits[i].TargetActor.Get();
                if (Target)
                {
					//造成伤害，question：如何在造成伤害的同时传递伤害来源和伤害类型等信息？
                    //Target->TakeDamage(Hits[i].Damage, FDamageEvent(), nullptr, nullptr);
                }

                // 处理完后销毁子弹实体
                IterContext.Defer().DestroyEntity(IterContext.GetEntity(i));
            }
        }
    );
}
