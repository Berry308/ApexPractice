// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Animation/WidgetAnimation.h"
#include "UIManagerSubsystem.generated.h"

//class UWidgetAnimation//ERROR!ERROR!

UCLASS()
class FIRSTPERSONSHOOT_API UUIManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	UUIManagerSubsystem();
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	void CreateBattleUI();

	UFUNCTION()
	void ShowHitPrompt();
	UFUNCTION()
	void HideHitPrompt();

	UFUNCTION()
	void ShowKillPrompt();
	UFUNCTION()
	void HideKillPrompt();

	UFUNCTION()
	UWidgetAnimation* GetAnimationFromWidget(UUserWidget* WidgetInstance, const FName& AnimationName) const;

public:
	UPROPERTY(Transient, Meta = (KillPrompt))
	UWidgetAnimation* KillPromptAnimation;

private:
	UPROPERTY()
	TSubclassOf<UUserWidget> HitPromptClass;
	UPROPERTY()
	TObjectPtr<UUserWidget> HitPrompt;

	//计时器句柄
	FTimerHandle hitPromptTimer;
};
