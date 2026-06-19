// Fill out your copyright notice in the Description page of Project Settings.


#include "Mass/Bullet/BulletVisionProcessor.h"
#include "Mass/Bullet/BulletFragments.h"
#include"Mass/Bullet/BulletSimulationProcessor.h"
#include "MassCommonFragments.h"
#include "MassExecutionContext.h"

UBulletVisionProcessor::UBulletVisionProcessor()
{
	ProcessingPhase = EMassProcessingPhase::DuringPhysics;
	ExecutionOrder.ExecuteAfter.Add(UBulletSimulationProcessor::StaticClass()->GetFName());
	//ProcessingInterval = 0.016f; // 每帧更新（假设60 FPS）
    
}

void UBulletVisionProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FBulletVisionFragment>(EMassFragmentAccess::ReadWrite);
}

void UBulletVisionProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
    EntityQuery.ForEachEntityChunk(EntityManager, Context, 
        [this](FMassExecutionContext& IterContext)
        {
            auto Transforms = IterContext.GetMutableFragmentView<FTransformFragment>();
            auto Interps = IterContext.GetMutableFragmentView<FBulletVisionFragment>();
            const float DeltaTime = IterContext.GetDeltaTimeSeconds();

            for (int32 i = 0; i < IterContext.GetNumEntities(); ++i)
            {
				// 1. 推进插值进度（插值进度由FBulletSimTimerChunkFragment控制，确保与模拟更新频率一致）
                //Interps[i].TimeSinceLastSim += DeltaTime;
                


                // 2. 计算 Alpha (0.0 到 1.0)
                float Alpha = FMath::Clamp(Interps[i].TimeSinceLastSim / Interps[i].SimStepDuration, 0.0f, 1.0f);

                // 3. 计算当前的视觉平滑位置
                FVector SmoothPos = FMath::Lerp(Transforms[i].GetTransform().GetLocation(), Interps[i].TargetLocation, Alpha);

                // 4. 更新 Transform (ISM 渲染器会自动读取这个值)
                Transforms[i].GetMutableTransform().SetLocation(SmoothPos);
            }
        }
    );
}
