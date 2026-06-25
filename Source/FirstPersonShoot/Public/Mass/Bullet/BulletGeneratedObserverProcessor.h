// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassObserverProcessor.h"
#include "BulletGeneratedObserverProcessor.generated.h"

/**
 * 
 */
UCLASS()
class FIRSTPERSONSHOOT_API UBulletGeneratedObserverProcessor : public UMassObserverProcessor
{
	GENERATED_BODY()
public:
	UBulletGeneratedObserverProcessor();

protected:
    virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
    virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
    FMassEntityQuery EntityQuery;
};
