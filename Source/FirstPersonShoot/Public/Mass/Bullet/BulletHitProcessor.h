// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "BulletHitProcessor.generated.h"

/**
 * 
 */
UCLASS()
class FIRSTPERSONSHOOT_API UBulletHitProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UBulletHitProcessor();
	
protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	FMassEntityQuery EntityQuery;
};
