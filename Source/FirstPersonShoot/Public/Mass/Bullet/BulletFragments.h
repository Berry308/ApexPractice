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
    UPROPERTY()
	FVector Gravity;
    //当前速度向量
	UPROPERTY()
	FVector Velocity;
    //该子弹的伤害
    UPROPERTY()
    float Damage;
    //剩余存活时间
    UPROPERTY()
	float RemainingLifeTime;
    //碰撞半径
    UPROPERTY()
    float CollisionRadius = 5.0f;
};

//SimulationProcessor通过更新该片段，来指定VisionProcessor中子弹插值更新的下一个目标点
USTRUCT()
struct FBulletVisionFragment : public FMassFragment
{
    GENERATED_BODY()

    UPROPERTY()
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

    UPROPERTY()
	float DamageTimer = 0; // 应用命中时间计时器

    UPROPERTY()
	float TimeToApplyDamage; // 从命中到应用伤害的延迟时间

    UPROPERTY()
    FVector HitLocation;
};

//定义一个ChunkFragment用于在SimulationProcessor中累积时间，控制模拟逻辑的更新频率
USTRUCT()
struct FBulletSimTimerChunkFragment : public FMassChunkFragment
{
	//注：第一次执行时需要初始化该值为设定的模拟步长，以确保第一帧就能执行模拟逻辑更新
    GENERATED_BODY()
    float TimeAccumulator = 0.032f;
};

// 子弹标识标签（无数据，仅用于过滤命中的子弹实体）
USTRUCT()
struct FBulletHitTag : public FMassTag { GENERATED_BODY() };
