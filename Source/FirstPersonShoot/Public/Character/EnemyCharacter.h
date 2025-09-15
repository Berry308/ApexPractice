// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EnemyCharacter.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnemyDie_OneParam, bool, isHeadShot);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEnemyDie);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEnemyCracked);


class AThirdPersonCharacter;

struct FTimerHandle;

UENUM(BlueprintType)
enum class EMoveTimeLevel : uint8
{
	Short    UMETA(DisplayName = "Short"),
	Medium   UMETA(DisplayName = "Medium"),
	Long     UMETA(DisplayName = "Long")
};

UCLASS()
class FIRSTPERSONSHOOT_API AEnemyCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "EnemyProperty")
	float shield;//护甲

	UPROPERTY(EditAnywhere, Category = "EnemyProperty")
	float health;//血量

	UPROPERTY(EditAnywhere, Category = "EnemyProperty")
	FVector2D reactionTimeRange;//反应时间

	UPROPERTY(EditAnywhere, Category = "EnemyProperty")
	float moveSpeed;

#pragma region AI

	UPROPERTY()
	TMap<EMoveTimeLevel, FVector2D> MoveTimeLevelRangeTable;

	//走位规律
	UPROPERTY()
	bool isEnemyCommonMove;//常规走位
	UPROPERTY()
	bool isEnemyMirrorMove;//镜像走位
	UPROPERTY()
	bool isEnemyReverseMirrorMove;//逆镜像走位

	UPROPERTY()
	EMoveTimeLevel currentLeftMoveLevel;//走位时左移类型
	UPROPERTY()
	float currentLeftMoveTime;

	UPROPERTY()
	EMoveTimeLevel currentRightMoveLevel;//走位时右移类型
	UPROPERTY()
	float currentRightMoveTime;

#pragma endregion
	

	UPROPERTY(BlueprintReadOnly, Category = "EnemyState")
	bool isCracked;//角色是否碎甲

	UPROPERTY(EditAnywhere)
	TSubclassOf<UDamageType> HeadShotDamage;

	UPROPERTY(BlueprintAssignable, Category = "Delegate")
	FOnEnemyCracked OnEnemyCrackedEvent;

	UPROPERTY(BlueprintAssignable, Category = "Delegate")
	FOnEnemyDie_OneParam OnEnemyDie_OneParamEvent;

	UPROPERTY(BlueprintAssignable, Category = "Delegate")
	FOnEnemyDie OnEnemyDieEvent;


	UPROPERTY()
	TObjectPtr<AThirdPersonCharacter> PlayerCharacter;

protected:

private:
	float reactionTime;
	bool isMovingLeft = false;
	bool isMovingRight = false;
	bool hasBeenHited = false;
	//维护两个计时器
	FTimerHandle ChangeMoveDirectionTimer;
	FTimerHandle EnemyReactionTimer;
	
public:
	AEnemyCharacter();

protected:
	virtual void BeginPlay() override;
public:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;



public:
	void ReduceHealth(float Damage,bool isHeadShot);

private:
	void Die(bool isHeadShot);

	void ResetMoveRegularity();//重置走位规律

	void RandomChooseMoveTime();//随机选取移动时间

	void AutoChangeMoveDirection();

	UFUNCTION()//此函数需要被绑定，添加UFUNCTION
	void MirrorMove(float moveRight);

	void DelayMirrorMove(float moveRight);

	void ApplyMoveRegularity();

	UFUNCTION()
	void ExchangeMoveRegularity();

	//必须使用UFUNCTION，因为委托要绑定该函数
	UFUNCTION()
	virtual void OnHit(
		AActor* DamagedActor,
		float Damage,
		class AController* InstigatedBy,
		FVector HitLocation,
		class UPrimitiveComponent* FHitComponent,
		FName BoneName,
		FVector ShotFromDirection,
		const class UDamageType* DamageType,
		AActor* DamageCauser
	);


};
