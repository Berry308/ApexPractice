// Fill out your copyright notice in the Description page of Project Settings.


#include "Manager/EnemyManagerSubsystem.h"
#include "NavigationSystem.h"
#include "Kismet/GameplayStatics.h"
#include "Character/ThirdPersonCharacter.h"
#include "Character/EnemyCharacter.h"

//#include "NavFilters/NavigationQueryFilter.h"

UEnemyManagerSubsystem::UEnemyManagerSubsystem()
{
	// 设置默认敌人蓝图类
	static ConstructorHelpers::FClassFinder<AEnemyCharacter> EnemyClassFinder(TEXT("/Game/Gameplay/Character/Enemy/BP_Enemy"));
	if (EnemyClassFinder.Class != nullptr)
	{
		EnemyClass = EnemyClassFinder.Class;
	}
}

void UEnemyManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	UE_LOG(LogTemp, Warning, TEXT("UEnemyManagerSubsystem::Initialize"));
	Super::Initialize(Collection);
	//生成敌人,question:在此处生成敌人时，似乎整个世界还没有初始化完成，玩家角色也还没有生成所以获取不到其指针
	//SpawnEnemy();
}

void UEnemyManagerSubsystem::RegisterEnemy(AEnemyCharacter* EnemyToRegister)
{
	UE_LOG(LogTemp, Warning, TEXT("UEnemyManagerSubsystem::RegisterEnemy"));
	if (EnemyToRegister && !EnemyList.Contains(EnemyToRegister))
	{
		EnemyList.Add(EnemyToRegister);

		// 绑定到敌人的销毁事件，以便自动取消注册
		EnemyToRegister->OnDestroyed.AddDynamic(this, &UEnemyManagerSubsystem::HandleEnemyDestroyed);
	}
}

void UEnemyManagerSubsystem::UnRegisterEnemy(AEnemyCharacter* EnemyToRegister)
{
	UE_LOG(LogTemp, Warning, TEXT("UEnemyManagerSubsystem::UnRegisterEnemy"));
	if (EnemyToRegister && EnemyList.Contains(EnemyToRegister))
	{
		//UE_LOG(LogTemp, Warning, TEXT("UEnemyManagerSubsystem::remove"));
		EnemyList.Remove(EnemyToRegister);

		// 绑定到敌人的销毁事件，以便自动取消注册
		EnemyToRegister->OnDestroyed.RemoveDynamic(this, &UEnemyManagerSubsystem::HandleEnemyDestroyed);
	}
	if (EnemyList.IsEmpty())
	{
		SpawnEnemy();
	}
}

void UEnemyManagerSubsystem::SpawnEnemy()
{
	UE_LOG(LogTemp, Warning, TEXT("SpawnEnemy"));
	//获取玩家角色指针
	PlayerCharacter = Cast<AThirdPersonCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	if (!IsValid(PlayerCharacter.Get()))
	{
		UE_LOG(LogTemp, Error, TEXT("playercharacter is null"));
		return;
	}

	//在导航区域内以玩家角色为中心，在一定范围内随机生成
	TObjectPtr<UNavigationSystemV1> NavSystem = UNavigationSystemV1::GetCurrent(GetWorld());
	if (!IsValid(NavSystem.Get()))
	{
		UE_LOG(LogTemp, Error, TEXT("NavSystem is null"));
		return;
	}

	const FVector playerLocation = PlayerCharacter->GetActorLocation();
	FNavLocation spawnLocation;
	//// 使用导航查询过滤器
	//FSharedConstNavQueryFilter QueryFilter = UNavigationQueryFilter::GetQueryFilter(
	//	*NavSystem,
	//	GetWorld(),
	//	UNavigationQueryFilter::
	//);
	if (NavSystem->GetRandomPointInNavigableRadius(playerLocation, spawnRadius, spawnLocation))
	{
		FActorSpawnParameters spawnParams;
		FVector spawnPosition = spawnLocation.Location + FVector(0, 0, 100.0f);
		spawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
		TObjectPtr<AEnemyCharacter> EnemyToSpawn = GetWorld()->SpawnActor<AEnemyCharacter>(
			EnemyClass,
			spawnPosition,
			FRotator::ZeroRotator,
			spawnParams);
		if (IsValid(EnemyToSpawn.Get()))
		{
			/*InitializeEnemy();*/
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("GetRandomPointInNavigableRadius failed"));
	}
	
}

void UEnemyManagerSubsystem::HandleEnemyDestroyed(AActor* DestroyedActor)
{
	AEnemyCharacter* DestroyedEnemy = Cast<AEnemyCharacter>(DestroyedActor);
	if (DestroyedEnemy)
	{
		UnRegisterEnemy(DestroyedEnemy);
	}
}
