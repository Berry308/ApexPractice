// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassEntityTypes.h"
#include "Engine/HitResult.h"
#include "BulletFragments.generated.h"
/**
 * 
 */

USTRUCT()
struct FBulletSimulationFragment : public FMassFragment
{
    GENERATED_BODY()

    // 射击发起者
    UPROPERTY()
    TWeakObjectPtr<AActor> InstigatorActor;
    //子弹当前位置
	FVector CurrentLocation;
    //下坠速度向量
	FVector Gravity;
    //当前速度向量
	FVector Velocity;
    //该子弹的伤害
    float Damage;
    //剩余存活时间
	float RemainingLifeTime;
    //碰撞半径
    float CollisionRadius;
    //该子弹是否已经进行了第一次模拟
	uint8 bNeedFirstSim : 1; 
    //使用弱指针，以便当持有武器的角色类被消灭时，避免悬空指针的发生
    TArray<TWeakObjectPtr<const AActor>> ActorToIgnore;
};

//SimulationProcessor通过更新该片段，来指定VisionProcessor中子弹插值更新的下一个目标点
USTRUCT()
struct FBulletVisionFragment : public FMassFragment
{
    GENERATED_BODY()

    FVector TargetLocation;

    //float TimeSinceLastSim = 0.0f; // 距离上次模拟逻辑更新经过的时间
    //float SimStepDuration = 0.1f;  // 模拟步长
};

//通过MassTag来确定命中的子弹实体。
USTRUCT()
struct FBulletHitFragment : public FMassFragment
{
    GENERATED_BODY()

    UPROPERTY()
	TWeakObjectPtr<AActor> InstigatorActor; // 射击发起者

    UPROPERTY()
    TWeakObjectPtr<AActor> TargetActor; // 射击命中物

	float DamageTimer = 0; // 应用命中时间计时器

	float TimeToApplyDamage; // 从命中到应用伤害的延迟时间

    FVector HitLocation;
};

//定义一个ChunkFragment用于在SimulationProcessor中累积时间，控制模拟逻辑的更新频率
USTRUCT()
struct FBulletSimTimerChunkFragment : public FMassChunkFragment
{
	//注：第一次执行时需要初始化该值为设定的模拟步长，以确保第一帧就能执行模拟逻辑更新
    GENERATED_BODY()
    float TimeAccumulator = 0.032f;
	//uint8 bIsChunkDirty : 1; // 标记该块是否存在变动
};

// 子弹标识标签（无数据，仅用于过滤命中的子弹实体）
USTRUCT()
struct FBulletHitTag : public FMassTag { GENERATED_BODY() };
