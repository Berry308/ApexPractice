// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerController/MyPlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Character/ThirdPersonCharacter.h"
#include "Manager/UIManagerSubsystem.h"

AMyPlayerController::AMyPlayerController()
{
	
}


void AMyPlayerController::HandleThirdLook(const FInputActionValue& Value)
{
	if(IsValid(playerCharacter))
	{
		playerCharacter->ThirdLook(Value);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to cast Pawn to AThirdPersonCharacter in HandleThirdLook"));
	}
}

void AMyPlayerController::HandleRun(const FInputActionValue& Value)
{
	if(IsValid(playerCharacter))
	{
		playerCharacter->Run(Value);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to cast Pawn to AThirdPersonCharacter in HandleRun"));
	}
}

void AMyPlayerController::HandleStopRun(const FInputActionValue& Value)
{
	if(IsValid(playerCharacter))
	{
		playerCharacter->StopRun(Value);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to cast Pawn to AThirdPersonCharacter in HandleStopRun"));
	}
}

void AMyPlayerController::HandleMoveForward(const FInputActionValue& Value)
{
	if(IsValid(playerCharacter))
	{
		playerCharacter->MoveForward(Value);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to cast Pawn to AThirdPersonCharacter in HandleMoveForward"));
	}
}

void AMyPlayerController::HandleMoveRight(const FInputActionValue& Value)
{
	if(IsValid(playerCharacter))
	{
		playerCharacter->MoveRight(Value);
		OnPlayerInputKeyAandD.Broadcast(Value.Get<float>());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to cast Pawn to AThirdPersonCharacter in HandleMoveRight"));
	}
}

void AMyPlayerController::HandleJump(const FInputActionValue& Value)
{
	if(IsValid(playerCharacter))
	{
		playerCharacter->Jump(Value);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to cast Pawn to AThirdPersonCharacter in HandleJump"));
	}
}

void AMyPlayerController::HandleStopJumping(const FInputActionValue& Value)
{
	if(IsValid(playerCharacter))
	{
		playerCharacter->StopJumping();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to cast Pawn to AThirdPersonCharacter in HandleStopJumping"));
	}
}

void AMyPlayerController::HandleToggleCameraView()
{
	if (IsValid(playerCharacter))
	{
		playerCharacter->ToggleCameraView();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to cast Pawn to AThirdPersonCharacter in HandleToggleCameraView"));
	}
}

void AMyPlayerController::HandleFireAction()
{
	if (IsValid(playerCharacter))
	{
		playerCharacter->Fire();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to cast Pawn to AThirdPersonCharacter in HandleFireAction"));
	}
}

void AMyPlayerController::HandleStopFireAction()
{
	if (IsValid(playerCharacter))
	{
		playerCharacter->StopFire();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to cast Pawn to AThirdPersonCharacter in HandleStopFireAction"));
	}
}

void AMyPlayerController::HandleAimAction()
{
	if (IsValid(playerCharacter.Get()))
	{
		playerCharacter->Aim();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to cast Pawn to AThirdPersonCharacter in HandleAimAction"));

	}
}

void AMyPlayerController::HandleStopAimAction()
{
	if (IsValid(playerCharacter.Get()))
	{
		playerCharacter->StopAim();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to cast Pawn to AThirdPersonCharacter in HandleStopAimAction"));

	}
}

void AMyPlayerController::CppCreatePlayerUI()
{
	TObjectPtr<UUIManagerSubsystem> UIManager = GetGameInstance()->GetSubsystem<UUIManagerSubsystem>();
	if (UIManager)
	{
		UIManager->CreateBattleUI();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("AMyPlayerController::CreatePlayerUI fail"));
	}
}

void AMyPlayerController::BeginPlay()
{
	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	if (Subsystem && DefaultMapping)
	{
		Subsystem->AddMappingContext(DefaultMapping, 0);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("DefaultMapping is not set or Subsystem is null"));
	}
}

void AMyPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	playerCharacter = Cast<AThirdPersonCharacter>(InPawn);
}

void AMyPlayerController::OnUnPossess()
{
	playerCharacter = nullptr;
	Super::OnUnPossess();
}


void AMyPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (DefaultMapping)
	{
		UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
		if (EnhancedInputComponent)
		{
			if (MoveForwardAction && MoveRightAction)
			{
				EnhancedInputComponent->BindAction(MoveRightAction, ETriggerEvent::Triggered, this, &AMyPlayerController::HandleMoveRight);
				EnhancedInputComponent->BindAction(MoveForwardAction, ETriggerEvent::Triggered, this, &AMyPlayerController::HandleMoveForward);
			}
			if (LookAction)
			{
				EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMyPlayerController::HandleThirdLook);
			}
			if (JumpAction)
			{
				EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AMyPlayerController::HandleJump);
				EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &AMyPlayerController::HandleStopJumping);
			}
			if (RunAction)
			{
				EnhancedInputComponent->BindAction(RunAction, ETriggerEvent::Started, this, &AMyPlayerController::HandleRun);
				EnhancedInputComponent->BindAction(RunAction, ETriggerEvent::Completed, this, &AMyPlayerController::HandleStopRun);
			}
			if (ToggleCameraViewAction)
			{
				EnhancedInputComponent->BindAction(ToggleCameraViewAction, ETriggerEvent::Started, this, &AMyPlayerController::HandleToggleCameraView);
			}
			if(FireAction)
			{
				EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Triggered, this, &AMyPlayerController::HandleFireAction);
				EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &AMyPlayerController::HandleStopFireAction);
			}
			if (AimAction)//可能有按住瞄准和切换瞄准两种方式，先默认为按住瞄准
			{
				EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Started, this, &AMyPlayerController::HandleAimAction);
				EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this, &AMyPlayerController::HandleStopAimAction);
			}
		}
	}

}


