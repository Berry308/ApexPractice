// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassBulletInitializer.generated.h"


USTRUCT()
struct FBulletSpawnData
{
    GENERATED_BODY()

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
    //使用弱指针，以便当持有武器的角色类被消灭时，避免悬空指针的发生
    TArray<TWeakObjectPtr<const AActor>> ActorToIgnore;
    //子弹画面表现所用的Location
    FVector TFLocation;
    //子弹画面表现的Scale3D
    FVector TFScale3D;
};
/**
 * 
 */
UCLASS()
class FIRSTPERSONSHOOT_API UMassBulletInitializer : public UMassProcessor
{
	GENERATED_BODY()
	
public:
    UMassBulletInitializer();

protected:
    // 必须重写 ConfigureQueries 和 Execute
    virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
    virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
    FMassEntityQuery EntityQuery;

};
