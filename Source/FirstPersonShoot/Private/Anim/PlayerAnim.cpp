// Fill out your copyright notice in the Description page of Project Settings.


#include "Anim/PlayerAnim.h"
#include "Character/ThirdPersonCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetmathLibrary.h"


void UPlayerAnim::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	//PlayerCharacter = Cast<AThirdPersonCharacter>(TryGetPawnOwner());
	//if (PlayerCharacter)
	//{
	//	PlayerCharacterMovement = PlayerCharacter->GetCharacterMovement();
	//}
}

void UPlayerAnim::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	////引擎崩溃
	//PlayerCharacter = Cast<AThirdPersonCharacter>(TryGetPawnOwner());
	//PlayerCharacterMovement = PlayerCharacter->GetCharacterMovement();
	//if (!PlayerCharacter || !PlayerCharacter->GetCharacterMovement())
	//{
	//	// 角色无效时重置速度值
	//	SpeedX = 0.0f;
	//	SpeedY = 0.0f;
	//	return;
	//}
	//// 使用const保证安全性和性能
	//const auto WorldVelocity = PlayerCharacterMovement->Velocity;
	//const auto& ActorTransform = PlayerCharacter->GetActorTransform();


	//// 智能缓存组件
	//if (!CachedCharacter.IsValid())
	//{
	//	CachedCharacter = Cast<AThirdPersonCharacter>(TryGetPawnOwner());
	//	if (CachedCharacter.IsValid()) {
	//		CachedMovement = CachedCharacter->GetCharacterMovement();
	//	}
	//}

	//// 安全检查
	//if (!CachedMovement.IsValid())
	//{
	//	SpeedX = SpeedY = 0;
	//	return;
	//}

	//// 使用const保证安全性和性能
	//const FVector& WorldVelocity = CachedMovement->Velocity;
	//const FTransform& ActorTransform = CachedCharacter->GetActorTransform();

	//// 局部空间转换
	//const FVector LocalVelocity = ActorTransform.InverseTransformVector(WorldVelocity);
	//// 直接使用局部速度分量
	//SpeedX = LocalVelocity.X;
	//SpeedY = LocalVelocity.Y; 


	if (!PlayerCharacter)
	{
		PlayerCharacter = Cast<AThirdPersonCharacter>(TryGetPawnOwner());
		if (PlayerCharacter)
		{
			PlayerCharacterMovement = PlayerCharacter->GetCharacterMovement();
		}
		else
		{
			return; // 如果角色无效，直接返回
		}
	}

	
}

