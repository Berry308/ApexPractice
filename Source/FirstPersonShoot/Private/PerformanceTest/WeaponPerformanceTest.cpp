// Fill out your copyright notice in the Description page of Project Settings.

#include "PerformanceTest/WeaponPerformanceTest.h"
#include "Weapon/WeaponBase.h"
#include "Character/ThirdPersonCharacter.h"
#include "Kismet/GameplayStatics.h"

AWeaponPerformanceTest::AWeaponPerformanceTest()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AWeaponPerformanceTest::BeginPlay()
{
	Super::BeginPlay();

	if (bEnableTest)
	{
		SpawnWeapons();
	}
}

void AWeaponPerformanceTest::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AWeaponPerformanceTest::SpawnWeapons()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("WeaponPerformanceTest: World is null"));
		return;
	}

	if (!WeaponClassToSpawn)
	{
		UE_LOG(LogTemp, Error, TEXT("WeaponPerformanceTest: WeaponClassToSpawn is not set"));
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// Spawn weapons in a grid pattern at fixed positions
	float Spacing = 200.0f;
	int32 Columns = FMath::CeilToInt(FMath::Sqrt((float)WeaponCount));

	for (int32 i = 0; i < WeaponCount; ++i)
	{
		int32 Row = i / Columns;
		int32 Col = i % Columns;
		
		FVector SpawnLocation = GetActorLocation() + FVector(Col * Spacing, Row * Spacing, 100.0f);
		FRotator SpawnRotation = FRotator(90.0f, 0.0f, 0.0f); // Face upward

		// Spawn weapon
		AWeaponBase* Weapon = World->SpawnActor<AWeaponBase>(
			WeaponClassToSpawn,
			SpawnLocation,
			SpawnRotation,
			SpawnParams
		);

		if (Weapon)
		{
			Weapon->ResetWeaponProperty();
			SpawnedWeapons.Add(Weapon);

			UE_LOG(LogTemp, Warning, TEXT("WeaponPerformanceTest: Spawned weapon %d at %s"), i, *SpawnLocation.ToString());
		}
	}

	// Start continuous fire after a short delay
	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(
		TimerHandle,
		this,
		&AWeaponPerformanceTest::StartContinuousFire,
		1.0f,
		false
	);
}

void AWeaponPerformanceTest::StartContinuousFire()
{
	// Set up a repeating timer to fire all weapons
	FTimerHandle FireTimerHandle;
	GetWorldTimerManager().SetTimer(
		FireTimerHandle,
		this,
		&AWeaponPerformanceTest::FireAllWeapons,
		FireInterval,
		true
	);

	UE_LOG(LogTemp, Warning, TEXT("WeaponPerformanceTest: Started continuous fire with %d weapons"), SpawnedWeapons.Num());
}

void AWeaponPerformanceTest::FireAllWeapons()
{
	for (AWeaponBase* Weapon : SpawnedWeapons)
	{
		if (Weapon)
		{
			Weapon->TryShoot();
		}
	}
}
