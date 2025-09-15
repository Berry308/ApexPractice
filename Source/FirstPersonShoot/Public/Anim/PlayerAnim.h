// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "PlayerAnim.generated.h"

class AThirdPersonCharacter;
class UCharacterMovementComponent;


UCLASS()
class FIRSTPERSONSHOOT_API UPlayerAnim : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<AThirdPersonCharacter> PlayerCharacter;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UCharacterMovementComponent> PlayerCharacterMovement;

	UPROPERTY(BlueprintReadOnly)
	float SpeedX;

	UPROPERTY(BlueprintReadOnly)
	float SpeedY;

private:
	// 缓存组件（弱引用自动处理销毁）
	UPROPERTY(Transient)
	TWeakObjectPtr<AThirdPersonCharacter> CachedCharacter;

	UPROPERTY(Transient)
	TWeakObjectPtr<UCharacterMovementComponent> CachedMovement;
};
