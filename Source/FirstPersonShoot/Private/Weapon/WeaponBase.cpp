// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/WeaponBase.h"
#include "Particles/ParticleSystem.h"
#include "Kismet/GameplayStatics.h"
#include "ProjectileBase.h"
#include "Components/SphereComponent.h"
#include "Tool/MyObjectPool.h"
#include "Character/ThirdPersonCharacter.h"
#include "Character/EnemyCharacter.h"
#include "MassEntitySubsystem.h"
#include "MassSpawnerSubsystem.h"
#include "MassEntityConfigAsset.h"
#include "MassEntityManager.h"
#include "MassEntityTypes.h"
#include "MassCommonFragments.h"
//#include "MassEntityBuilder.h"
#include "Mass/Bullet/BulletFragments.h"
#include "Mass/Bullet/MassBulletInitializer.h"

// Sets default values
AWeaponBase::AWeaponBase()
{
	PrimaryActorTick.bCanEverTick = true;
	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>("WeaponMesh");
	RootComponent = WeaponMesh;
	SphereCollision = CreateDefaultSubobject<USphereComponent>("SphereCollision");
	SphereCollision->SetupAttachment(WeaponMesh);

	//以下对象池初始化代码会导致对象池为空
	//ProjectileObjectPool = CreateDefaultSubobject<AMyObjectPool>("ProjectileObjectPool");
	//ProjectileObjectPool->SetOwner(Cast<AActor>( RootComponent));//question

	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	WeaponMesh->SetCollisionObjectType(ECC_WorldStatic);
	/*WeaponMesh->SetOnlyOwnerSee(true);*/
	WeaponMesh->SetEnableGravity(true);
	WeaponMesh->SetSimulatePhysics(true);

	SphereCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SphereCollision->SetCollisionObjectType(ECC_WorldDynamic);

}

void AWeaponBase::BeginPlay()
{
	Super::BeginPlay();
	//初始化对象池
	ProjectileObjectPool = NewObject<UMyObjectPool>(this);

	//根据射击频率和子弹的存活时间计算出对象池的大小
	int ObjectPoolSize = BulletMaxLifeTime/ShootFrequency + 1;//多预留一个对象
	if (WeaponProjectileClass.Get() != nullptr && ObjectPoolSize != 0 && IsValid(ProjectileObjectPool.Get()))
	{
		ProjectileObjectPool->InitializeObjectPool(WeaponProjectileClass, ObjectPoolSize, false);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("ProjectileObjectPool initialize failed"));
	}

	//初始化碰撞列表,忽略武器自身
	ActorToIgnore.Add(this);
	//武器腰射散布调整
	WeaponMaxSpreadAngle /= 100;
}

void AWeaponBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AWeaponBase::ResetWeaponProperty()
{
	WeaponMesh->SetEnableGravity(false);
	WeaponMesh->SetSimulatePhysics(false);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SphereCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

//射击合法性检验
bool AWeaponBase::ShootValidate()
{
	return /*AmmoCheck() &&*/ ShootFrequencyValidate();
}
bool AWeaponBase::AmmoCheck()
{
	if (CurrentClipAmmo == 0)
	{
		if (IsValid(WeaponOwner))
		{
			if (!WeaponOwner->bIsEndlessAmmo)
			{
				if (WeaponOwner->bAutoReload)
				{
					//执行换弹操作(换弹动作是否可以打断)
				}
				else
				{
					//提示当前子弹数量为零
				}
				return false;
			}
			else
			{
				return true;
			}
		}
		else
		{
			return false;
		}
	}
	else
	{
		return true;
	}
}
bool AWeaponBase::ShootFrequencyValidate()
{
	//是否符合武器的射击频率
	if (_lastShootTilNow >= ShootFrequency)
	{
		_lastShootTilNow = 0;//重置上次射击直到现在的时间
		GetWorldTimerManager().SetTimer(
			_fireTimerHandle,
			[this]()
			{
				if (_lastShootTilNow >= ShootFrequency)//当上次射击的时间已经超过限制的射击间隔时间
				{
					_lastShootTilNow = ShootFrequency;
					GetWorldTimerManager().ClearTimer(_fireTimerHandle);
				}
				_lastShootTilNow += GetWorld()->GetDeltaSeconds();
			},
			GetWorld()->GetDeltaSeconds(),
			true
		);
		return true;
	}
	else
	{
		return false;
	}
}

float AWeaponBase::CalculateDamage(TObjectPtr<UPhysicalMaterial> PhysicalMaterial)
{
	UE_LOG(LogTemp, Warning, TEXT("WeaponBase:CalculateDamage"));
	//根据命中部位的不同，计算方式也有所不同
	if (!IsValid(PhysicalMaterial.Get()))
	{
		UE_LOG(LogTemp, Warning, TEXT("WeaponBase:CalculateDamage: PhysicalMaterial get failed"));
		return WeaponDamage;
	}

	float finalDamage = 0;


	UE_LOG(LogTemp, Warning, TEXT("HitedPhysicalMaterial: %s"), *PhysicalMaterial->GetName());
	switch (PhysicalMaterial->SurfaceType)
	{
	case EPhysicalSurface::SurfaceType1:
	{
		//头部
		finalDamage = WeaponDamage * WeaponHeadShotRatio;
	}
	break;
	case EPhysicalSurface::SurfaceType2:
	{
		//身体
		finalDamage = WeaponDamage * 1.0;
	}
	break;
	}
	return finalDamage;
}

FVector AWeaponBase::CalculateShootDirection()
{
	if (IsValid(WeaponOwner))
	{
		TObjectPtr<APlayerController> PC = Cast<APlayerController>(WeaponOwner->GetController());
		check(PC);
		FVector CameraLocation;
		FRotator CameraRotation;
		PC->GetPlayerViewPoint(CameraLocation, CameraRotation);

		if (WeaponOwner->bIsAiming == false)
		{
			return FMath::VRandCone(CameraRotation.Vector(), WeaponMaxSpreadAngle);
		}
		else
		{
			return CameraRotation.Vector();
		}
	}
	else
	{
		return GetActorRotation().Vector();
	}
}

void AWeaponBase::PlayWeaponSound()
{
	//UE_LOG(LogTemp, Warning, TEXT("PlayWeaponSound"));

	if (IsValid(WeaponShootingSound.Get()))
	{
		UGameplayStatics::PlaySound2D(GetWorld(), WeaponShootingSound);
	}
}

void AWeaponBase::PlayMuzzleFlash()
{
	//UE_LOG(LogTemp, Warning, TEXT("PlayMuzzleFlash"));

	if (IsValid(MuzzleFlash.Get()))
	{
		UGameplayStatics::SpawnEmitterAttached(MuzzleFlash, WeaponMesh,
			TEXT("Muzzle"), FVector::ZeroVector,
			FRotator::ZeroRotator, FVector::OneVector,
			EAttachLocation::KeepRelativeOffset, true,
			EPSCPoolMethod::AutoRelease, true
		);
	}
}

void AWeaponBase::OnWeaponBeEquipedBy(AActor* NewOwner)
{
	CurrentActorToIgnored = NewOwner;

	//添加需要忽略的持有者
	if (CurrentActorToIgnored.IsValid())
	{
		ActorToIgnore.Add(CurrentActorToIgnored);
	}
}

void AWeaponBase::OnWeaponBeUnEquiped()
{
	//释放当前的持有者
	CurrentActorToIgnored.Reset();
}

//物理弹道模拟(是否需要让子弹直飞一段时间再进行重力的模拟？节省性能或者更符合真实，子弹没有考虑空气阻力)
bool AWeaponBase::SimulatePhysicsTrajectory(
	const FVector& OriginalPosition,const FVector& Direction,
	float InitialSpeed,float GravityScale, 
	float MaxLifetime,float Radius,
	FHitResult& OutHitResult, float& OutTimeToHit,TArray<FVector>& OutTrajectoryPoints) const
{
	UE_LOG(LogTemp, Warning, TEXT("SimulatePhysicsTrajectory"));

	float currentTime = 0.0f;
	float timeStep = 0.016f;//步长代表了模拟的精度（60帧）

	//初始化(要返回一个子弹飞行的时间)
	FVector currentPosition = OriginalPosition;//OriginalPosition通常来说是相机的Location
	FVector currentVelocity = Direction * InitialSpeed;
	FVector gravity = FVector(0, 0, -980) * GravityScale;


	//在起点与目标点应用球形射线检测
	FHitResult hitInfo;
	FCollisionQueryParams queryParams;

	queryParams.AddIgnoredActors(ActorToIgnore);
	queryParams.bReturnPhysicalMaterial = true;

	//当子弹存活的时候进行模拟，直到击中物体
	while (currentTime < MaxLifetime)
	{
		//重力叠加
		currentVelocity += gravity * timeStep;

		//首先计算出一帧中的起点和目标点
		FVector targetPosition;
		if (currentTime == 0.0f)
		{
			//最开始一次射线检测直接到射击方向前方3米处，忽略重力，让OutTrajectoryPoints数组的第一个点稍微远一点，以防子弹从枪口生成后过度修正导致的视觉不匹配
			targetPosition = currentPosition + Direction.GetSafeNormal() * 300.0f;
		}
		else
		{
			targetPosition = currentPosition + currentVelocity * timeStep;
		}



		bool bhitsomething = GetWorld()->SweepSingleByChannel(
			hitInfo,
			currentPosition,
			targetPosition,
			FQuat::Identity,
			ECollisionChannel::ECC_GameTraceChannel1,//设定碰撞检测通道为Projectile
			FCollisionShape::MakeSphere(Radius),
			queryParams
		);
		/*UE_LOG(LogTemp, Warning, TEXT("hit actor: %s"), *hitInfo.GetActor()->GetName());*/

		if (bIsTrajectoryDraw)
		{
			// 绘制调试信息
			DrawDebugLine(GetWorld(), currentPosition, targetPosition, FColor::Red, false, 5.0f);
			DrawDebugSphere(GetWorld(), targetPosition, Radius, 8, FColor::Green, false, 5.0f);
		}

		//如果击中物体就退出循环
		if (bhitsomething)
		{
			UE_LOG(LogTemp, Warning, TEXT("linetrace hit sth"));
			//命中处理(返回命中目标需要的时间，返回碰撞信息)
			OutHitResult = hitInfo;
			OutTimeToHit = currentTime + (hitInfo.Time * timeStep);
			OutTrajectoryPoints.Add(hitInfo.Location);//添加命中点到轨迹数组
			if (bIsTrajectoryDraw)
			{
				// 绘制调试信息
				DrawDebugPoint(GetWorld(), hitInfo.Location, 10.0f, FColor::Red, false, 5.0f);
				DrawDebugLine(GetWorld(), hitInfo.Location, hitInfo.Location + hitInfo.Normal * 50.0f, FColor::Blue, false, 5.0f);
			}
			return true;
		}

		//添加目标点到轨迹数组
		OutTrajectoryPoints.Add(targetPosition);

		//更新位置和子弹时间
		currentPosition = targetPosition;
		currentTime += timeStep;

		//如果子弹飞的太远
		if (FVector::Dist(OriginalPosition, currentPosition) > 10000.0f)
		{
			break;
		}
	}

	OutTimeToHit = MaxLifetime;//当没有击中物体时，飞行时间就是子弹最大生存时间
	OutTrajectoryPoints.Add(currentPosition);
	return false;
}


void AWeaponBase::PlayProjectileLaunch(const FRotator& SpawnDirection,
	const TArray<FVector>& Trajectory,float FLightTime,
    const FHitResult& HitResult,bool bIsHit)
{
	//UE_LOG(LogTemp, Warning, TEXT("PlayProjectileLaunch"));
	//获得将要用到的参数
	FVector MuzzlePosition = WeaponMesh->GetSocketLocation(FName("Muzzle"));
	
	//从子弹对象池中取出子弹将其生成,并且发射
	TObjectPtr<AProjectileBase> projectile = Cast<AProjectileBase>(ProjectileObjectPool->GetPooledObject(MuzzlePosition, SpawnDirection));
	if (IsValid(projectile.Get()))
	{
		projectile->_projectileOwner = this;//以便子弹销毁前回调武器函数

		/*UE_LOG(LogTemp, Warning, TEXT("FLightTime: %f"), FLightTime);*/
		//UE_LOG(LogTemp, Warning, TEXT("PlayProjectileLaunch:HitActor: %s"), *HitResult.GetActor()->GetName());
		projectile->ShootProjectileWithTrajectory(MuzzlePosition, 
			Trajectory,HitResult,
			FLightTime,bIsHit,SpawnDirection.Vector());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("projectile cast failed"));
	}
}


void AWeaponBase::ObjectBulletShoot()
{
	//定义弹道模拟将要用到的参数
	FVector ShootLocation;
	FRotator ShootRotation;
	if (IsValid(WeaponOwner))
	{
		TObjectPtr<APlayerController> PC = Cast<APlayerController>(WeaponOwner->GetController());
		if (!IsValid(PC.Get())) return;

		FVector CameraLocation;
		FRotator CameraRotation;
		PC->GetPlayerViewPoint(CameraLocation, CameraRotation);
		ShootLocation = CameraLocation;
		ShootRotation = CameraRotation;
	}
	else
	{
		ShootLocation = GetActorLocation();
		ShootRotation = GetActorRotation();
	}

	FHitResult hitResult;
	hitResult.Init();
	float TimeToHit = 0;//子弹从发射到击中的时间
	TArray<FVector> TrajectoryPoints;
	TrajectoryPoints.Empty();//先清空弹道轨迹点
	bool bIsBulletHit = false;

	if (IsValid(WeaponOwner) && WeaponOwner->bIsAiming)//开镜瞄准
	{
		//模拟物理弹道
		bIsBulletHit = SimulatePhysicsTrajectory(ShootLocation,
			ShootRotation.Vector(), BulletInitialSpeed,
			BulletGravity, BulletMaxLifeTime,
			BulletRadius, hitResult,
			TimeToHit, TrajectoryPoints);
#pragma region TestCode
		//物理模拟射线检测测试代码
		//UE_LOG(LogTemp, Warning, TEXT("TimeToHit: %f"), TimeToHit);
		//UE_LOG(LogTemp, Warning, TEXT("AfterSimulatePhysicsTrajectory:HitActor: %s"),*hitResult.GetActor()->GetName());
		//TObjectPtr<UPhysicalMaterial> physMaterial = hitResult.PhysMaterial.Get();
		//if (!physMaterial)
		//{
		//	UE_LOG(LogTemp, Error, TEXT("physMaterial is null"));
		//	return;
		//}
		//FString HitComponentName = hitResult.GetComponent() ? hitResult.GetComponent()->GetName() : TEXT("None");
		//FString PhysMatName = hitResult.PhysMaterial->GetName();//崩溃
		//FString BoneName = hitResult.BoneName.ToString();

		//UE_LOG(LogTemp, Warning, TEXT("Hit Component: %s,Hit Bone: %s, Physical Material: %s"),
		//	*HitComponentName, *BoneName, *PhysMatName);
#pragma endregion
		//渲染子弹视觉表现，一般来说子弹击中物体的反馈特效由物体来决定，也就是说每个可以被击中的物体都附带有可以供外部调用的特效
		PlayProjectileLaunch(ShootRotation, TrajectoryPoints, TimeToHit, hitResult, bIsBulletHit);
	}
	else//腰射
	{
		FVector shootdirection;
		shootdirection = FMath::VRandCone(ShootRotation.Vector(), WeaponMaxSpreadAngle);//圆锥体内散布
		bIsBulletHit = SimulatePhysicsTrajectory(
			ShootLocation, shootdirection,
			BulletInitialSpeed, BulletGravity,
			BulletMaxLifeTime, BulletRadius,
			hitResult, TimeToHit, TrajectoryPoints
		);
		//渲染子弹视觉表现
		PlayProjectileLaunch(ShootRotation, TrajectoryPoints, TimeToHit, hitResult, bIsBulletHit);
	}
}

void AWeaponBase::MassBulletShoot()
{
	UWorld* World = GetWorld();
	UMassEntitySubsystem* MassSubsystem = UWorld::GetSubsystem<UMassEntitySubsystem>(World);
	check(MassSubsystem);
	FMassEntityManager& EntityManager = MassSubsystem->GetMutableEntityManager();

#pragma region 通过UMassSpawnerSubsystem创建实体
	if (!BulletMassConfig)
	{
		UE_LOG(LogTemp, Error, TEXT("AWeaponBase::MassBulletShoot : Do not has a BulletMassConfig"));
		return;
	}
	// 1. 获取该配置对应的模板
	FMassEntityTemplateID TemplateID = BulletMassConfig->GetConfig().GetOrCreateEntityTemplate(*World).GetTemplateID();

	// 2. 构建初始化数据
	FBulletSpawnData SpawnData;
	if (IsValid(WeaponOwner))
	{
		TObjectPtr<APlayerController> PC = Cast<APlayerController>(WeaponOwner->GetController());
		check(PC);
		FVector CameraLocation;
		FRotator CameraRotation;
		PC->GetPlayerViewPoint(CameraLocation, CameraRotation);
		//根据开关镜计算腰射散布角度
		FVector ShootDirection = CalculateShootDirection();

		SpawnData.InstigatorActor = this;
		SpawnData.CurrentLocation = CameraLocation;
		SpawnData.Velocity = ShootDirection * BulletInitialSpeed;
		SpawnData.CollisionRadius = BulletRadius;
		SpawnData.Gravity = FVector(0, 0, -980) * BulletGravity;
		SpawnData.RemainingLifeTime = BulletMaxLifeTime;
		SpawnData.Damage = WeaponDamage;
		SpawnData.ActorToIgnore = ActorToIgnore;
		SpawnData.TFLocation = WeaponMesh->GetSocketLocation(TEXT("Muzzle"));
		SpawnData.TFScale3D = BulletMeshTransformScale;
	}
	else
	{
		SpawnData.InstigatorActor = WeaponOwner;
		SpawnData.CurrentLocation = GetActorLocation();
		SpawnData.Velocity = GetActorRotation().Vector() * BulletInitialSpeed;
		SpawnData.CollisionRadius = BulletRadius;
		SpawnData.Gravity = FVector(0, 0, -980) * BulletGravity;
		SpawnData.RemainingLifeTime = BulletMaxLifeTime;
		SpawnData.Damage = WeaponDamage;
		SpawnData.ActorToIgnore = ActorToIgnore;
		SpawnData.TFLocation = WeaponMesh->GetSocketLocation(TEXT("Muzzle"));
		SpawnData.TFScale3D = BulletMeshTransformScale;
	}

	// 3. 生成实体
	UMassSpawnerSubsystem* SpawnerSubsystem = UWorld::GetSubsystem<UMassSpawnerSubsystem>(World);
	check(SpawnerSubsystem);
	TArray<FMassEntityHandle> OutEntities;
	SpawnerSubsystem->SpawnEntities(
		TemplateID,
		1,                                      // 数量
		FConstStructView::Make(SpawnData),    // 传入动态数据
		UMassBulletInitializer::StaticClass(),  // 传入负责初始化的处理器
		OutEntities                             // 结果返回
	);
#pragma endregion

#pragma region 通过Builder创建实体
	//FTransformFragment Tran;
	//Tran.GetMutableTransform().SetLocation(WeaponMesh->GetSocketLocation(TEXT("Muzzle")));
	//Tran.GetMutableTransform().SetScale3D(BulletMeshTransformScale);
	//FBulletSimulationFragment Sim;
	//Sim.bNeedFirstSim = true;
	//Sim.InstigatorActor = WeaponOwner;
	//Sim.RemainingLifeTime = BulletMaxLifeTime;
	//Sim.CurrentLocation = CameraLocation;
	//Sim.Velocity = ShootDirection * BulletInitialSpeed;
	//Sim.CollisionRadius = BulletRadius;
	//Sim.Gravity = FVector(0, 0, -980) * BulletGravity;
	//Sim.RemainingLifeTime = BulletMaxLifeTime;
	//Sim.Damage = WeaponDamage;
	//Sim.ActorToIgnore = ActorToIgnore;
	//
	//FMassEntityHandle NewEntity = EntityManager.MakeEntityBuilder()
	//	.Add<FTransformFragment>(Tran)
	//	.Add<FBulletSimulationFragment>(Sim)
	//	.Commit();
#pragma endregion
}

//计算伤害，将伤害传递给持有该武器的角色类，让角色类进行伤害的造成.将子弹回收到对象池中。让子弹类来播放特效，让武器类调用击中对象造成伤害的函数
void AWeaponBase::BulletReachTarget(TObjectPtr<AProjectileBase> Bullet,bool bIsBulletHit,const FHitResult& HitResult, const FVector& BulletShootDirection)
{
	UE_LOG(LogTemp, Warning, TEXT("WeaponBase:BulletReachTarget"));
	//返回对象池(属性需要重置吗?不需要)
	ProjectileObjectPool->ReturnPooledObject(Bullet);
	
	if (bIsBulletHit)//如何判断子弹击中？在射线检测后传递信息给子弹类，让子弹返回信息给武器。
	{
		//判断击中对象,并且根据部位计算伤害,然后调用持有该武器的角色类的造成伤害函数。
		TObjectPtr<AEnemyCharacter> hitEnemy = Cast<AEnemyCharacter>(HitResult.GetActor());
		if (IsValid(hitEnemy.Get()))
		{
			TObjectPtr<UPhysicalMaterial> hitPhysicalMaterial = HitResult.PhysMaterial.Get();
			if (!IsValid(hitPhysicalMaterial.Get()))
			{
				UE_LOG(LogTemp, Error, TEXT("WeaponBase:BulletReachTarget: Get HitActorPhysicalMaterial failed"));

				//FString PhysMatName = HitResult.PhysMaterial->GetName();
				//FString BoneName = HitResult.BoneName.ToString();

				//UE_LOG(LogTemp, Warning, TEXT("Hit Bone: %s, Physical Material: %s"),
				//	*BoneName, *PhysMatName);

				//// 检查材质是否为默认材质
				//if (PhysMatName.Contains("Default"))
				//{
				//	UE_LOG(LogTemp, Error, TEXT("Default material detected on bone: %s"), *BoneName);
				//}

				//return;
			}

			float actualDamage = 0;//由于HitResult.PhysMaterial是智能指针，不存在从TWeakObjectPtr到TObjectPtr的转换
			actualDamage = CalculateDamage(hitPhysicalMaterial);

			//此处为了传递给被击中角色被爆头的信息
			TSubclassOf<UDamageType> damageType = UDamageType::StaticClass();
			if (hitPhysicalMaterial->SurfaceType == EPhysicalSurface::SurfaceType1)
			{
				//说明爆头
				damageType = HeadShotDamage;
			}
			
			if (actualDamage <= 0)
			{
				UE_LOG(LogTemp, Error, TEXT("actualDamage <= 0"));
			}
			else
			{
				WeaponOwner->CauseDamage(hitEnemy, actualDamage,
					BulletShootDirection, HitResult,
					damageType);
			}
		}

	}
}

//分为开镜和腰射，射击的起点
void AWeaponBase::TryShoot()
{
	//UE_LOG(LogTemp, Display, TEXT("TryShoot"));
	//判断是否满足射击条件（子弹数量、射击频率）
	if (!ShootValidate())
	{
		return;
	}

	//动画表现
	//播放枪械射击动画
	PlayShootAnimation();

	//播放手臂后座力动画
	if (IsValid(WeaponOwner))
	{
		WeaponOwner->PlayArmsShootingAnimation();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("weapon have not owner"));
	}

	//播放枪械的音效的特效
	PlayWeaponSound();

	//应用后座力改变角色摄像机
	if (IsValid(WeaponOwner))
	{
		WeaponOwner->PlayerShootingRecoil();
	}

	//基于对象池的射击系统
	//ObjectBulletShoot();

	//基于MassEntity的射击系统
	MassBulletShoot();
}



