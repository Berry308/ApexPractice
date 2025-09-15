// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileBase.h"
#include "Weapon/WeaponBase.h"
#include "Interface/BeHitedInterface.h"
#include "Particles/ParticleSystem.h"
#include "Character/EnemyCharacter.h"
#include "Manager/GameplaySoundSubsystem.h"
#include "Engine.h"

// Sets default values
AProjectileBase::AProjectileBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
	RootComponent = ProjectileMesh;
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	//禁用子弹阴影的渲染
	ProjectileMesh->SetCastShadow(false);
	ProjectileMesh->bCastStaticShadow = false;
	ProjectileMesh->bCastDynamicShadow = false;
	ProjectileMesh->SetCastInsetShadow(false);

	////子弹尾迹
	//ProjectileTrail = CreateDefaultSubobject<UNiagaraComponent>(TEXT("ProjectileTrail"));
	//ProjectileTrail -> SetupAttachment(RootComponent);
}

void AProjectileBase::ShootProjectileWithTrajectory(
	const FVector& StartLocation,
	const TArray<FVector>& TrajectoryPoints,
	const FHitResult& HitResult,
	float FlightTime,
	bool bIsHit,
	const FVector& StartDirection
	)
{
	//UE_LOG(LogTemp, Warning, TEXT("ProjectilBase:ShootProjectileWithTrajectory"));

	/*if (ProjectileTrail)
	{
		ProjectileTrail->Activate();
	}*/

	//将传入的参数传递给私有成员
	_trajectoryPoints = TrajectoryPoints;
	if (_trajectoryPoints.Num() < 2)//说明几乎是瞬时命中的，不用渲染子弹网格体了
	{
		OnTargetReach();
	}

	_projectileHitResult = HitResult;
	//UE_LOG(LogTemp, Warning, TEXT("ShootProjectileWithTrajectory:HitActor: %s"), *_projectileHitResult.GetActor()->GetName());
	_bIsHit = bIsHit;
	_projectileFlyTime = FlightTime;
	_elapsedTime = 0.0f;
	_segmentProgress = 0.0f;
	_currentPathIndex = 0;
	_startDirection = StartDirection;

	SetActorLocation(StartLocation);//通常传入的是枪口的坐标

	//设置计时器,当达到飞行时间时调用函数
	//出现问题：当射线检测没有击中目标的时候，计时器部署失败。但是当射线检测击中目标时可以正常运行。
	GetWorldTimerManager().SetTimer(LifeTimerHandle,
		this,
		&AProjectileBase::OnTargetReach,
		FlightTime,
		false
	);

	////Test:计时器是否成功部署
	//if (GetWorldTimerManager().IsTimerActive(LifeTimerHandle))
	//{
	//	UE_LOG(LogTemp, Warning, TEXT("Timer successfully deployed!"));
	//}
	//else
	//{
	//	UE_LOG(LogTemp, Error, TEXT("Failed to deploy timer!"));
	//}
}

void AProjectileBase::BeginPlay()
{
	Super::BeginPlay();
}

void AProjectileBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//更新时间与比例
	_elapsedTime += DeltaTime;
	float elapasedTimePercent = _elapsedTime / _projectileFlyTime;//到了飞行时间直接销毁，所以该值小于1吗？

	//计算子弹当前所在弹道轨迹点的区域段
	int segmentNum = _trajectoryPoints.Num();//假如轨迹数组中有两个点，那么飞行段数也就有两段，因为已经有一个起点为Muzzle
	float currentSegment = segmentNum * elapasedTimePercent;//根据过去时间与总飞行时间的比例，计算出包含小数的当前段数
	_currentPathIndex = FMath::FloorToInt32(currentSegment)-1;//向下取整
	_currentPathIndex = FMath::Clamp(_currentPathIndex, 0, segmentNum - 2);//此处的边界要想清楚，不光是数组序号本身要减一，飞行段数本来也要比数组的数量少一
	_segmentProgress = FMath::Frac(currentSegment);//取小数部分,作为插值的计算参数

	//进行插值改变物体位置(question:实际表现出来的弹道会不会分段式位移显得不够平滑？毕竟是模拟，如果不够平滑，或许可以通过缩短射线模拟时的时间步长来更精细化模拟)
	//子弹的起始点是枪口,应该先模拟枪口到第一弹道轨迹点
	FVector startPos, endPos;
	if (currentSegment < 1)
	{
		startPos = _muzzlePosition;
		endPos = _trajectoryPoints[0];
	}
	else
	{
		startPos = _trajectoryPoints[_currentPathIndex];
		endPos = _trajectoryPoints[_currentPathIndex + 1];//崩溃，超出数组的边界。
	}
	FVector targetPos = FMath::Lerp(startPos, endPos, _segmentProgress);
	SetActorLocation(targetPos);

	//更新物体的旋转方向为近似速度方向
	FVector moveDirection = (endPos - startPos).GetSafeNormal();
	if (!moveDirection.IsNearlyZero())
	{
		SetActorRotation(moveDirection.Rotation());
	}
}

void AProjectileBase::RenderHitEffect()
{
	//获取击中信息，击中的物体一定是Actor吗？大地算不算Actor呢？
	TObjectPtr<AActor> ActorBeHited = _projectileHitResult.GetActor();
	//判断该Actor是否实现了蓝图接口
	if (IsValid(ActorBeHited))
	{
		if (ActorBeHited->Implements<UBeHitedInterface>())
		{
			//调用蓝图接口的函数
			IBeHitedInterface::Execute_OnHitByProjectile(ActorBeHited, _projectileHitResult);
		}
		else
		{
			//调用全局特效
		}
	}
	//else if()//如果没有实现蓝图接口，判断其是否是某种材质
	//{

	//}
}

void AProjectileBase::PlayHitSound()
{
	//播放音效,question:音频管理器怎么知道击中的是敌人的哪个部位呢？因为需要根据击中部位播放声音并且覆盖
	bool isHeadShot = false;
	TObjectPtr<UPhysicalMaterial> hitPhysicalMaterial = _projectileHitResult.PhysMaterial.Get();
	if (IsValid(hitPhysicalMaterial.Get()))
	{
		if (hitPhysicalMaterial->SurfaceType == EPhysicalSurface::SurfaceType1)//如果击中的是头部
			isHeadShot = true;
	}

	UWorld* world = GetWorld();
	if (world)
	{
		TObjectPtr<UGameplaySoundSubsystem> soundManager = world->GetSubsystem<UGameplaySoundSubsystem>();
		if (!IsValid(soundManager.Get())) UE_LOG(LogTemp, Error, TEXT("AProjectileBase:ReduceHitSound():Get soundManager failed"));
		//调用音频管理器
		soundManager->PlayBulletHitSound(_projectileHitResult.GetActor(), _projectileHitResult, isHeadShot);
	}
}

//如何让子弹来渲染特效,使用全局音效管理器单例播放音效，特效由击中对象来播放
void AProjectileBase::OnTargetReach()
{
	UE_LOG(LogTemp, Warning, TEXT("ProjectileBase:OnTargetReach"));
	//如果子弹击中，渲染画面效果和播放音效
	if (_bIsHit)
	{
		RenderHitEffect();
		PlayHitSound();
	}
	else
	{

	}
	//告诉WeaponBase类子弹网格体已经到达目标,计算并且应用伤害,然后将子弹返回对象池

	//UE_LOG(LogTemp, Warning, TEXT("OnTargetReach:HitActor: %s"), *_projectileHitResult.GetActor()->GetName());
	_projectileOwner->BulletReachTarget(this,_bIsHit,_projectileHitResult,_startDirection);
}

