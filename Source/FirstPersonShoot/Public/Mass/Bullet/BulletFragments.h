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

    //飞行速度
    UPROPERTY()
    float Velocity;

    //下坠速度
    UPROPERTY()
	float Gravity;

    //飞行方向
	UPROPERTY()
	FVector Direction;

    UPROPERTY()
    float Damage;

    //剩余存活时间
    UPROPERTY()
	float RemainingLifeTime;

    //碰撞半径
    UPROPERTY()
    float CollisionRadius = 5.0f;
};

//通过MassTag来确定命中的子弹实体。
USTRUCT()
struct FBulletHitFragment : public FMassFragment
{
    GENERATED_BODY()

    UPROPERTY()
    TWeakObjectPtr<AActor> TargetActor; // 使用弱指针防止引用悬挂

    FVector HitLocation;
};

// 子弹标识标签（无数据，仅用于过滤命中的子弹实体）
USTRUCT()
struct FBulletHitTag : public FMassTag { GENERATED_BODY() };
