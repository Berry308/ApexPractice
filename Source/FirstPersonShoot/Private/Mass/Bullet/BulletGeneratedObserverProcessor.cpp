// Fill out your copyright notice in the Description page of Project Settings.


#include "Mass/Bullet/BulletGeneratedObserverProcessor.h"
#include "Mass/Bullet/BulletFragments.h"
#include "MassExecutionContext.h"


UBulletGeneratedObserverProcessor::UBulletGeneratedObserverProcessor()
{
	Operation = EMassObservedOperation::Add;
    ObservedType = FBulletSimulationFragment::StaticStruct();
    RegisterQuery(EntityQuery);
}

void UBulletGeneratedObserverProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
    EntityQuery.RegisterWithProcessor(*this);
    EntityQuery.AddRequirement<FBulletSimulationFragment>(EMassFragmentAccess::ReadWrite);
    EntityQuery.AddChunkRequirement<FBulletSimTimerChunkFragment>(EMassFragmentAccess::ReadWrite);
}

void UBulletGeneratedObserverProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
    UE_LOG(LogTemp, Log, TEXT("UBulletGeneratedObserverProcessor::Execute"));
    EntityQuery.ForEachEntityChunk(Context, [this](FMassExecutionContext& IterContext)
        {
            FBulletSimTimerChunkFragment& Timer = IterContext.GetMutableChunkFragment<FBulletSimTimerChunkFragment>();
			Timer.bIsChunkDirty = true; // 标记该块为脏块，表示需要立即进行模拟更新

            UE_LOG(LogTemp, Warning, TEXT("Observer: A new BulletEntity has added to a chunk.Mark the chunk as Dirty %d"), Timer.bIsChunkDirty);
        }
    );
}