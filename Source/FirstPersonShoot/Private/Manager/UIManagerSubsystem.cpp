// Fill out your copyright notice in the Description page of Project Settings.


#include "Manager/UIManagerSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"


UUIManagerSubsystem::UUIManagerSubsystem()
{
	static ConstructorHelpers::FClassFinder<UUserWidget> HitPromptFinder(TEXT("/Game/UI/WB_ShootHitPrompt"));

	if (HitPromptFinder.Succeeded())
	{
		HitPromptClass = HitPromptFinder.Class;
	}
}

void UUIManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogTemp, Warning, TEXT("UUIManagerSubsystem::Initialize"));
}

void UUIManagerSubsystem::CreateBattleUI()
{
	UE_LOG(LogTemp, Warning, TEXT("UUIManagerSubsystem::CreateBattleUI"));
	//创建命中提示UI
	if (!HitPrompt && HitPromptClass)
	{
		HitPrompt = CreateWidget<UUserWidget>(GetWorld(), HitPromptClass);
		HitPrompt->AddToViewport();
		HitPrompt->SetVisibility(ESlateVisibility::Hidden);
		KillPromptAnimation = GetAnimationFromWidget(HitPrompt, TEXT("KillPrompt_INST"));//名称需要使用uelog获取
	}
}

void UUIManagerSubsystem::ShowHitPrompt()
{
	UE_LOG(LogTemp, Warning, TEXT("UUIManagerSubsystem::ShowHitPrompt"));
	if (HitPrompt)
	{
		UE_LOG(LogTemp, Warning, TEXT("yess"));
		HitPrompt->SetVisibility(ESlateVisibility::Visible);
		//设置定时器，在一定时间内隐藏击中UI
		UWorld* world = GetWorld();
		if (world)
		{
			world->GetTimerManager().SetTimer<UUIManagerSubsystem>(hitPromptTimer,
				this,
				&UUIManagerSubsystem::HideHitPrompt, 
				0.25f
			);
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("UUIManagerSubsystem::ShowHitPrompt fail"));
	}
}

void UUIManagerSubsystem::HideHitPrompt()
{
	if (HitPrompt)
	{
		HitPrompt->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UUIManagerSubsystem::ShowKillPrompt()
{
	if (HitPrompt && KillPromptAnimation)
	{
		UE_LOG(LogTemp, Warning, TEXT("UUIManagerSubsystem::ShowKillPrompt"));
		HitPrompt->PlayAnimation(KillPromptAnimation);
	}
}

void UUIManagerSubsystem::HideKillPrompt()
{

}


UWidgetAnimation* UUIManagerSubsystem::GetAnimationFromWidget(UUserWidget* WidgetInstance, const FName& AnimationName) const
{
	if (!WidgetInstance) return nullptr;

	// 1. 获取Widget的蓝图生成类
	UWidgetBlueprintGeneratedClass* WidgetClass = Cast<UWidgetBlueprintGeneratedClass>(WidgetInstance->GetClass());
	if (!WidgetClass) return nullptr;

	// 2. 该类包含一个它拥有的所有动画的数组
	TArray<UWidgetAnimation*> AllAnimations = WidgetClass->Animations;

	// 3. 遍历数组，按名称查找
	for (UWidgetAnimation* Animation : AllAnimations)
	{
		UE_LOG(LogTemp, Warning, TEXT("UUIManagerSubsystem::GetAnimationFromWidget:%s"), *Animation->GetFName().ToString());
		if (Animation && Animation->GetFName() == AnimationName)
		{
			return Animation;
		}
	}
	UE_LOG(LogTemp, Error, TEXT("UUIManagerSubsystem::GetAnimationFromWidget is Failed"));
	return nullptr;
}
