// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "BulletVisionProcessor.generated.h"

/**
 * 
 */
UCLASS()
class FIRSTPERSONSHOOT_API UBulletVisionProcessor : public UMassProcessor
{
	GENERATED_BODY()
public:
	UBulletVisionProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;


private:
	FMassEntityQuery EntityQuery;
};
