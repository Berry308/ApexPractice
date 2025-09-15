// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "EnemyManagerSubsystem.generated.h"

class AThirdPersonCharacter;
class AEnemyCharacter;

UCLASS()
class FIRSTPERSONSHOOT_API UEnemyManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	UEnemyManagerSubsystem();

	UPROPERTY(EditAnywhere)
	float spawnRadius = 1000.0f;//敌人的生成范围

	UPROPERTY(EditAnywhere)
	TObjectPtr<AThirdPersonCharacter> PlayerCharacter;

	UPROPERTY()
	TSubclassOf<AEnemyCharacter> EnemyClass;

private:
	TArray<TObjectPtr<AEnemyCharacter>> EnemyList;

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintCallable)
	void RegisterEnemy(AEnemyCharacter* EnemyToRegister);

	UFUNCTION(BlueprintCallable)
	void UnRegisterEnemy(AEnemyCharacter* EnemyToRegister);

	void SpawnEnemy();

private:
	UFUNCTION()
	void HandleEnemyDestroyed(AActor* DestroyedActor);
};
