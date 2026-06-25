// Fill out your copyright notice in the Description page of Project Settings.


#include "Mass/Bullet/MassBulletInitializer.h"
#include "Mass/Bullet/BulletFragments.h"
#include "MassCommonFragments.h"
#include "MassExecutionContext.h"

UMassBulletInitializer::UMassBulletInitializer()
{
    // 关键点：Initializer 通常不需要每帧运行，它只由 Spawner 手动触发
    bAutoRegisterWithProcessingPhases = false;
    RegisterQuery(EntityQuery);
}

void UMassBulletInitializer::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
    EntityQuery.RegisterWithProcessor(*this);
    EntityQuery.AddRequirement<FBulletSimulationFragment>(EMassFragmentAccess::ReadWrite);
    EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);
}

void UMassBulletInitializer::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
    UE_LOG(LogTemp, Log, TEXT("UMassBulletInitializer::Execute"));
    // 从 Context 中获取传入的 SpawnData
    const FBulletSpawnData* SpawnData = Context.GetAuxData().GetPtr<FBulletSpawnData>();
    if (!SpawnData) return;

    EntityQuery.ForEachEntityChunk(EntityManager, Context, [SpawnData](FMassExecutionContext& IterContext)
        {
            auto Sim = IterContext.GetMutableFragmentView<FBulletSimulationFragment>();
            auto TF = IterContext.GetMutableFragmentView<FTransformFragment>();

            for (int32 i = 0; i < IterContext.GetNumEntities(); ++i)
            {
                // 将武器传入的数据 写入到 Mass 实体的内存中
                Sim[i].bNeedFirstSim = true;
                Sim[i].InstigatorActor = SpawnData->InstigatorActor;
                Sim[i].CurrentLocation = SpawnData->CurrentLocation;
                Sim[i].Velocity = SpawnData->Velocity;
                Sim[i].CollisionRadius = SpawnData->CollisionRadius;
                Sim[i].Gravity = SpawnData->Gravity;
                Sim[i].RemainingLifeTime = SpawnData->RemainingLifeTime;
                Sim[i].Damage = SpawnData->Damage;
                Sim[i].ActorToIgnore = SpawnData->ActorToIgnore;

                TF[i].GetMutableTransform().SetLocation(SpawnData->TFLocation);
                TF[i].GetMutableTransform().SetScale3D(SpawnData->TFScale3D);
            }
        });
}


