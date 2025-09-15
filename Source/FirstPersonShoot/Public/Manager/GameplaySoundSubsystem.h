// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameplaySoundSubsystem.generated.h"

class USoundBase;
class UDataTable;

//struct FHitResult;
//class FName;

USTRUCT(BlueprintType)
struct FSoundInfo: public FTableRowBase
{
	GENERATED_BODY()
    
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName SoundType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName SoundName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<USoundBase> Sound;
};

UCLASS()
class FIRSTPERSONSHOOT_API UGameplaySoundSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
	
public:

private:
	TObjectPtr<UDataTable> SoundDataTable;

	TObjectPtr<USoundBase> SoundToPlay;

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintCallable)
	void PlayBulletHitSound(AActor* HitedActor,const FHitResult& hitResult,bool isHeadShot);

	UFUNCTION(BlueprintCallable)
	void PlayEnemyCrackedSound();

	UFUNCTION(BlueprintCallable)
	void PlayEnemyDieSound(bool isHeadShot);

private:
	USoundBase* FindSound(FName SoundType,	FName SoundName);
};
