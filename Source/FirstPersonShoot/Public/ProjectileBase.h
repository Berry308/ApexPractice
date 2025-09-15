// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectileBase.generated.h"


class UStaticMeshComponent;
class UNiagaraComponent;

class AWeaponBase;

UCLASS()
class FIRSTPERSONSHOOT_API AProjectileBase : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AProjectileBase();
	
	//弹道曲线
	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void ShootProjectileWithTrajectory(
		const FVector& StartLocation,
		const TArray<FVector>& TrajectoryPoints,
		const FHitResult& HitResult,
		float FlightTime,
		bool bIsHit,
		const FVector& StartDirection
	);


	bool bIsProjectileHit;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

public:	
	UPROPERTY(EditAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> ProjectileMesh;

	//UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = "Components")
	//TObjectPtr<UNiagaraComponent> ProjectileTrail;


	//UPROPERTY(EditAnywhere, Category = "Audio")
	//TObjectPtr<USoundBase> HitSound;音效考虑在单例类中使用

	UPROPERTY()
	TObjectPtr<AWeaponBase> _projectileOwner;


protected:
	

private:
	bool _bIsHit;
	FHitResult _projectileHitResult;
	TArray<FVector> _trajectoryPoints;
	float _projectileFlyTime;
	float _elapsedTime;
	float _segmentProgress;//在当前轨迹段的哪个位置
	int32 _currentPathIndex;
	FVector _muzzlePosition;
	FVector _startDirection;


	FTimerHandle LifeTimerHandle;

	UFUNCTION()
	void RenderHitEffect();

	UFUNCTION()
	void PlayHitSound();

	UFUNCTION()
	void OnTargetReach();
};
