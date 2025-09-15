// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MyPlayerController.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerInput,float,inputValue);

class UInputMappingContext;
class UInputAction;
class UInputComponent;
class AThirdPersonCharacter;

struct FInputActionValue;

UCLASS()
class FIRSTPERSONSHOOT_API AMyPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	AMyPlayerController();

	void HandleThirdLook(const FInputActionValue& Value);
	void HandleRun(const FInputActionValue& Value);
	void HandleStopRun(const FInputActionValue& Value);
	void HandleMoveForward(const FInputActionValue& Value);
	void HandleMoveRight(const FInputActionValue& Value);
	void HandleJump(const FInputActionValue& Value);
	void HandleStopJumping(const FInputActionValue& Value);
	void HandleToggleCameraView();
	void HandleFireAction();
	void HandleStopFireAction();
	void HandleAimAction();
	void HandleStopAimAction();

	UFUNCTION(BlueprintImplementableEvent, Category = "PlayerUI")
	void CreatePlayerUI();

	void CppCreatePlayerUI();

protected:
	virtual void SetupInputComponent() override;
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn * InPawn) override;
	virtual void OnUnPossess() override;

public:
#pragma region Delegate
	FOnPlayerInput OnPlayerInputKeyAandD;
#pragma endregion

private:
	UPROPERTY()
	TObjectPtr<AThirdPersonCharacter> playerCharacter;//使用更兼容的做法，ACharacter基类而不是具体类，但是我没办法调用自定义角色类中的方法。此处提一个设想：如果我要切换控制不同的角色类，该怎么实现？

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMapping;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> RunAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveForwardAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveRightAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> ToggleCameraViewAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> FireAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> AimAction;
};
