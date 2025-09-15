// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "BeHitedInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UBeHitedInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class FIRSTPERSONSHOOT_API IBeHitedInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
	
public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, BlueprintCallable, Category = "Combat")
	void OnHitByProjectile(const FHitResult& HitResult);
};
