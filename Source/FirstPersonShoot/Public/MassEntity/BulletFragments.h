// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassEntityTypes.h"
/**
 * 
 */
USTRUCT()
struct FBulletMovementFragment : public FMassFragment
{
    GENERATED_BODY()
    UPROPERTY()
    FVector Velocity = FVector::ZeroVector;
};

// 子弹标识标签（无数据，仅用于过滤）
USTRUCT()
struct FBulletTag : public FMassTag
{
    GENERATED_BODY()
};