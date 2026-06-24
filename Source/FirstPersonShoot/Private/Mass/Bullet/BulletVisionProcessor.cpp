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
    RegisterQuery(EntityQuery);
}

void UBulletVisionProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
    EntityQuery.RegisterWithProcessor(*this);
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FBulletVisionFragment>(EMassFragmentAccess::ReadWrite);
}

void UBulletVisionProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
    EntityQuery.ForEachEntityChunk( Context, 
        [this](FMassExecutionContext& IterContext)
        {
            auto Transforms = IterContext.GetMutableFragmentView<FTransformFragment>();
            auto Interps = IterContext.GetMutableFragmentView<FBulletVisionFragment>();
            const float DeltaTime = IterContext.GetDeltaTimeSeconds();

            for (int32 i = 0; i < IterContext.GetNumEntities(); ++i)
            {
                // 计算 Alpha (0.0 到 1.0)，子弹飞行计算以30hz进行
                float Alpha = FMath::Clamp(DeltaTime / 0.032, 0.0f, 1.0f);

				// 计算当前的视觉平滑位置，当前位置和目标位置之间进行线性插值
                FVector SmoothPos = FMath::Lerp(Transforms[i].GetTransform().GetLocation(), Interps[i].TargetLocation, Alpha);

                // 更新 Transform (ISM 渲染器会自动读取这个值)
                Transforms[i].GetMutableTransform().SetLocation(SmoothPos);
            }
        }
    );
}
