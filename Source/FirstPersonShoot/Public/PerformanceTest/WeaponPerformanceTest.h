// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponPerformanceTest.generated.h"

class AWeaponBase;

UCLASS()
class FIRSTPERSONSHOOT_API AWeaponPerformanceTest : public AActor
{
	GENERATED_BODY()
	
public:	
	AWeaponPerformanceTest();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, Category = "PerformanceTest")
	TSubclassOf<AWeaponBase> WeaponClassToSpawn;

	UPROPERTY(EditAnywhere, Category = "PerformanceTest")
	int32 WeaponCount = 10;

	UPROPERTY(EditAnywhere, Category = "PerformanceTest")
	float FireInterval = 0.1f;

	UPROPERTY(EditAnywhere, Category = "PerformanceTest")
	bool bEnableTest = true;

private:
	UPROPERTY()
	TArray<TObjectPtr<AWeaponBase>> SpawnedWeapons;

	UFUNCTION()
	void SpawnWeapons();

	UFUNCTION()
	void StartContinuousFire();

	UFUNCTION()
	void FireAllWeapons();
};
