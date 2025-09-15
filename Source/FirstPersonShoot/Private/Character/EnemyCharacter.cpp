// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/EnemyCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Manager/GameplaySoundSubsystem.h"
#include "Manager/EnemyManagerSubsystem.h"
#include "Character/ThirdPersonCharacter.h"
#include "Kismet/KismetMathLibrary.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "PlayerController/MyPlayerController.h"
#include "Manager/UIManagerSubsystem.h"



// Sets default values
AEnemyCharacter::AEnemyCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	//初始化走位时间等级的范围表
	MoveTimeLevelRangeTable.Add(EMoveTimeLevel::Short, FVector2D(0.3f, 0.6f));
	MoveTimeLevelRangeTable.Add(EMoveTimeLevel::Medium, FVector2D(0.6f, 1.2f));
	MoveTimeLevelRangeTable.Add(EMoveTimeLevel::Long, FVector2D(1.2f, 2.0f));
}

void AEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 获取敌人管理器并注册自己
	UEnemyManagerSubsystem* EnemyManager = GetGameInstance()->GetSubsystem<UEnemyManagerSubsystem>();
	if (EnemyManager)
	{
		EnemyManager->RegisterEnemy(this);
	}
	//属性初始化
	shield = 100;
	health = 100;
	isCracked = false;
	PlayerCharacter = Cast<AThirdPersonCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	if (!PlayerCharacter) UE_LOG(LogTemp, Error, TEXT("Can not get player character"));
	//绑定角色死亡和碎甲事件时，应该播放的音效函数
	UWorld* world = GetWorld();
	if (world)
	{
		TObjectPtr<UGameplaySoundSubsystem> soundmanager = world->GetSubsystem<UGameplaySoundSubsystem>();
		if (soundmanager)
		{
			OnEnemyCrackedEvent.AddDynamic(soundmanager, &UGameplaySoundSubsystem::PlayEnemyCrackedSound);
			OnEnemyDie_OneParamEvent.AddDynamic(soundmanager, &UGameplaySoundSubsystem::PlayEnemyDieSound);
		}
	}
	TObjectPtr<UUIManagerSubsystem> UImanager = GetGameInstance()->GetSubsystem<UUIManagerSubsystem>();
	if (UImanager)
	{
		OnEnemyDieEvent.AddDynamic(UImanager, &UUIManagerSubsystem::ShowKillPrompt);
	}
	OnTakePointDamage.AddDynamic(this, &AEnemyCharacter::OnHit);//订阅承受伤害的动态委托，调用自定义函数(需要用UFUNCTION宏标记)

	//敌人初始化时以常规走位规律走路
	ResetMoveRegularity();
	isEnemyMirrorMove = false;
	isEnemyReverseMirrorMove = false;
	isEnemyCommonMove = true;
	ApplyMoveRegularity();

	////测试MirrorMove
	//ResetMoveRegularity();
	//isEnemyMirrorMove = true;
	//isEnemyReverseMirrorMove = false;
	//isEnemyCommonMove = false;
	//ApplyMoveRegularity();
	OnEnemyCrackedEvent.AddDynamic(this, &AEnemyCharacter::ExchangeMoveRegularity);
}

void AEnemyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	//敌人持续朝向玩家(xy面)
	if (PlayerCharacter)
	{
		FVector playerLocation = PlayerCharacter->GetActorLocation();
		FRotator targetRotator = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), playerLocation);
		FRotator enemyRotator = GetActorRotation();
		targetRotator.Roll = enemyRotator.Roll;
		targetRotator.Pitch = enemyRotator.Pitch;

		SetActorRotation(targetRotator);
	}
	
	//// 确保移动组件存在且有效，蓝图类的AutoPossessAi要设置为Spawned
	//if (UCharacterMovementComponent* MovementComp = GetCharacterMovement())
	//{
	//	// 检查移动模式，如果不是Walking，设置为Walking
	//	if (MovementComp->MovementMode != MOVE_Walking)
	//	{
	//		UE_LOG(LogTemp, Error, TEXT("WOAIHWOD"));
	//		MovementComp->SetMovementMode(MOVE_Walking);
	//	}
	//}

	//移动
	if (isMovingRight)
	{
		AddMovementInput(GetActorRightVector(), moveSpeed * DeltaTime);
	}
	else if (isMovingLeft)
	{
		AddMovementInput(GetActorRightVector(), -moveSpeed * DeltaTime);
	}
}

void AEnemyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}


void AEnemyCharacter::ReduceHealth(float Damage,bool isHeadShot)
{
	UE_LOG(LogTemp, Warning, TEXT("EnemyCharacter:ReduceHealth"));
	if (!isCracked)//有护甲
	{
		//碎甲
		if (Damage > shield)
		{
			UE_LOG(LogTemp, Warning, TEXT("Enemy Cracked"));
			isCracked = true;
			//多播碎甲委托
			OnEnemyCrackedEvent.Broadcast();
			float overflowDamage = Damage - shield;
			shield = 0;
			health -= overflowDamage;
			if (health <= 0)
			{
				health = 0;
				Die(isHeadShot);
			}
		}
		else
		{
			shield -= Damage;
		}
	}
	else //没有护甲只剩血量
	{
		//播放击中血量的音效
		health -= Damage;
		if (health <= 0)
		{
			health = 0;
			Die(isHeadShot);
		}
	}
}

void AEnemyCharacter::Die(bool isHeadShot)
{
	UE_LOG(LogTemp, Warning, TEXT("Enemy die"));
	//播放敌人死亡的音效有音频管理器来完成
	OnEnemyDie_OneParamEvent.Broadcast(isHeadShot);
	OnEnemyDieEvent.Broadcast();
	//取消注册在敌人注册到管理器的时候就绑定了敌人销毁自动取消注册
	Destroy();//OnDestroy是先通知还是先销毁实例？
}

//改变左移和右移的范围等级
void AEnemyCharacter::ResetMoveRegularity()
{
	UE_LOG(LogTemp, Warning, TEXT("AEnemyCharacter::ResetMoveRegularity"));
	//解绑镜像走位绑定的玩家输入事件
	TObjectPtr<AMyPlayerController> playerController = Cast<AMyPlayerController>(PlayerCharacter->GetController());
	if (playerController)
	{
		playerController->OnPlayerInputKeyAandD.RemoveDynamic(this, &AEnemyCharacter::MirrorMove);
	}
	//重置走位规律
	isEnemyCommonMove = false;
	isEnemyMirrorMove = false;
	isEnemyReverseMirrorMove = false;

	//随机选取走位规律
	int8 chooseMoveRegularityIndex = FMath::RandRange(0, 2);
	switch (chooseMoveRegularityIndex)
	{
	    case 0:
		{
			isEnemyCommonMove = true;
		}
		case 1:
		{
			isEnemyMirrorMove = true;
		}
		case 2:
		{
			isEnemyReverseMirrorMove = true;
		}
	}

	//随机选取一个移动等级
	int8 chooseMoveLevelIndex = FMath::RandRange(0, 2);
	currentLeftMoveLevel = StaticCast<EMoveTimeLevel>(chooseMoveLevelIndex);//将目录转换为枚举
	chooseMoveLevelIndex = FMath::RandRange(0, 2);
	currentRightMoveLevel = StaticCast<EMoveTimeLevel>(chooseMoveLevelIndex); 
}

//随机从范围内选取移动时间
void AEnemyCharacter::RandomChooseMoveTime()
{
	//从表中查找对应等级的时间范围，并且随机选取时间
	FVector2D leftTimeRange = MoveTimeLevelRangeTable[currentLeftMoveLevel];
	FVector2D rightTimeRange = MoveTimeLevelRangeTable[currentRightMoveLevel];

	currentLeftMoveTime = FMath::FRandRange(leftTimeRange.X, leftTimeRange.Y);
	currentRightMoveTime = FMath::FRandRange(rightTimeRange.X, rightTimeRange.Y);
}


void AEnemyCharacter::AutoChangeMoveDirection()
{
	//UE_LOG(LogTemp, Display, TEXT("AEnemyCharacter::AutoChangeMoveDirection"));
	if (!isEnemyCommonMove) return;
	float timeTilChangeDirection;
	RandomChooseMoveTime();
	if (isMovingLeft)
	{
		//UE_LOG(LogTemp, Warning, TEXT("Enemy moves right"));
		//改成向右移动
		isMovingLeft = false;
		isMovingRight = true;
		timeTilChangeDirection = currentRightMoveTime;
	}
	else
	{
		//UE_LOG(LogTemp, Warning, TEXT("Enemy moves left"));
		//改成向左移动
		isMovingLeft = true;
		isMovingRight = false;
		timeTilChangeDirection = currentLeftMoveTime;
	}
	GetWorldTimerManager().SetTimer<AEnemyCharacter>(
		ChangeMoveDirectionTimer, 
		this, 
		&AEnemyCharacter::AutoChangeMoveDirection,
		timeTilChangeDirection
	);
}

//如果是镜像走位，此时需要绑定PlayerCharacter的ad按键输入事件，并且在更改走位模式时需要进行解绑
void AEnemyCharacter::MirrorMove(float moveRight)
{
	//UE_LOG(LogTemp, Warning, TEXT("AEnemyCharacter::MirrorMove"));
	//敌人拥有反应时间
	if (!GetWorldTimerManager().IsTimerActive(ChangeMoveDirectionTimer))
	{
		reactionTime = FMath::FRandRange(reactionTimeRange.X, reactionTimeRange.Y);
		GetWorldTimerManager().SetTimer(
			ChangeMoveDirectionTimer,
			[this, moveRight]()
			{
				this->DelayMirrorMove(moveRight);
			},
			reactionTime,
			false
		);
	}
}

void AEnemyCharacter::DelayMirrorMove(float moveRight)
{
	//根据玩家输入来调用
	if (isEnemyMirrorMove)
	{
		if (moveRight < 0)
		{
			//UE_LOG(LogTemp, Warning, TEXT("Enemy moves right"));
			//改成向右移动
			isMovingLeft = false;
			isMovingRight = true;
		}
		else
		{
			//UE_LOG(LogTemp, Warning, TEXT("Enemy moves left"));
			//改成向左移动
			isMovingLeft = true;
			isMovingRight = false;
		}
	}
	else if (isEnemyReverseMirrorMove)
	{
		if (moveRight > 0)
		{
			//UE_LOG(LogTemp, Warning, TEXT("Enemy moves right"));
			//改成向右移动
			isMovingLeft = false;
			isMovingRight = true;
		}
		else
		{
			//UE_LOG(LogTemp, Warning, TEXT("Enemy moves left"));
			//改成向左移动
			isMovingLeft = true;
			isMovingRight = false;
		}
	}
	
}

//根据走位规律，启动对应的函数
void AEnemyCharacter::ApplyMoveRegularity()
{
	UE_LOG(LogTemp, Warning, TEXT("AEnemyCharacter::ApplyMoveRegularity"));
	if (isEnemyCommonMove)
	{
		AutoChangeMoveDirection();
	}
	else if (isEnemyMirrorMove || isEnemyReverseMirrorMove)
	{
		//绑定玩家输入事件
		TObjectPtr<AMyPlayerController> playerController = Cast<AMyPlayerController>(PlayerCharacter->GetController());
		if (playerController)
		{
			playerController->OnPlayerInputKeyAandD.AddDynamic(this, &AEnemyCharacter::MirrorMove);
		}
	}
}

void AEnemyCharacter::ExchangeMoveRegularity()
{
	ResetMoveRegularity();
	ApplyMoveRegularity();
}

void AEnemyCharacter::OnHit(AActor* DamagedActor,
	float Damage,
	class AController* InstigatedBy,
	FVector HitLocation,
	class UPrimitiveComponent* FHitComponent,
	FName BoneName,
	FVector ShotFromDirection,
	const class UDamageType* DamageType,
	AActor* DamageCauser)
{
	bool isHeadShot = false;
	if (!hasBeenHited)//如果是第一次被击中
	{
		hasBeenHited = true;
		ResetMoveRegularity();
		ApplyMoveRegularity();
	}
	//如果此次为爆头伤害
	if (DamageType->GetClass() == HeadShotDamage) isHeadShot = true;
	ReduceHealth(Damage,isHeadShot);
	UKismetSystemLibrary::PrintString(this,FString::Printf( TEXT("EnemyHealth:%f"), health));
}



