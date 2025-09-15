// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/ThirdPersonCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnHancedInputComponent.h"
#include "Weapon/WeaponBase.h"
#include "PlayerController/MyPlayerController.h"
#include "Engine.h"
#include <EnhancedInputSubsystems.h>
#include "Manager/EnemyManagerSubsystem.h"
#include "Manager/UIManagerSubsystem.h"


// Sets default values
AThirdPersonCharacter::AThirdPersonCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>("CameraBoom");
	if (CameraBoom)
	{
		CameraBoom->SetupAttachment(RootComponent);
		CameraBoom->TargetArmLength = 600.0f;
		CameraBoom->AddLocalRotation(FRotator(-30.0f, 0.0f, 0.0f));
		CameraBoom->bUsePawnControlRotation = true;
	}

	PlayerFirstCamera = CreateDefaultSubobject<UCameraComponent>("PlayerFirstCamera");
	if (PlayerFirstCamera)
	{
		PlayerFirstCamera->SetupAttachment(RootComponent);
		PlayerFirstCamera->bUsePawnControlRotation = true;
		PlayerFirstCamera->SetRelativeLocation(FVector(0, 0, 60));
		PlayerFirstCamera->SetFieldOfView(90.0f);
	}

	PlayerThirdCamera = CreateDefaultSubobject<UCameraComponent>("PlayerThirdCamera");
	if (PlayerThirdCamera)
	{
		PlayerThirdCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
		PlayerThirdCamera->bUsePawnControlRotation = false;//相机杆已经应用旋转
		PlayerThirdCamera->SetActive(false);
	}

	ArmMesh = CreateDefaultSubobject<USkeletalMeshComponent>("ArmMesh");
	if (ArmMesh)
	{
		ArmMesh->SetupAttachment(PlayerFirstCamera);
		ArmMesh->SetOnlyOwnerSee(true);
	}

	if (GetMesh())
	{
		//GetMesh()->SetOwnerNoSee(true);
		GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);//网格设置为碰撞仅查询
		GetMesh()->SetCollisionObjectType(ECC_Pawn);//此处存疑

	}

	WeaponList.SetNum(2);//初始化武器列表为2，只能装备两个武器
	LastedJumpTilNow = JumpRecoveryTime;// 初始化LastedJumpTilNow为JumpRecoveryTime，避免第一次跳跃时计算错误
}

// Called when the game starts or when spawned
void AThirdPersonCharacter::BeginPlay()
{
	//UE_LOG(LogTemp, Display, TEXT("PlayerCharacter spawn"));
	Super::BeginPlay();

	//在初始化时生成一个敌人
	UWorld* world = GetWorld();
	if (world)
	{
		TObjectPtr<UEnemyManagerSubsystem> enemyManager = world->GetGameInstance()->GetSubsystem<UEnemyManagerSubsystem>();
		if (!IsValid(enemyManager.Get())) UE_LOG(LogTemp, Error, TEXT("Get enemyManager failed"));
		enemyManager->SpawnEnemy();
	}

	UpdateMeshVisiability();
	InitWeapon();
	FPArmsAnimBP = ArmMesh->GetAnimInstance();

	//UI
	//创建玩家UI
	TObjectPtr<AMyPlayerController> currentController = Cast<AMyPlayerController>(GetController());
	if (IsValid(currentController.Get()))
	{
		currentController->CreatePlayerUI();
		currentController->CppCreatePlayerUI();
	}
	//绑定相关事件
	TObjectPtr<UUIManagerSubsystem> UIManager = GetGameInstance()->GetSubsystem<UUIManagerSubsystem>();
	if (UIManager)
	{
		OnPlayerCauseDamage.AddDynamic(UIManager, &UUIManagerSubsystem::ShowHitPrompt);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("AThirdPersonCharacter:BeginPlay:UImanager get failed"));
	}
}

void AThirdPersonCharacter::PlayArmsShootingAnimation()
{
	//先从武器中获取手臂开火动画
	TObjectPtr<UAnimMontage> armsShootingMontage = nullptr;
	if (IsValid(ActiveWeapon))
	{
		armsShootingMontage = ActiveWeapon->ArmsShootingMontage;
	}


	if(IsValid(armsShootingMontage))
	{
		//调用当前动画蓝图进行播放
		FPArmsAnimBP->Montage_SetPlayRate(armsShootingMontage, 1.0f);
		FPArmsAnimBP->Montage_Play(armsShootingMontage);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("armsShootingMontage is invalid"));
	}
}

//射击后座力
void AThirdPersonCharacter::PlayerShootingRecoil()
{
	//角色的镜头恢复应该需要一定时间，当我停止射击的时候应该重置武器后座力
	TObjectPtr<UCurveFloat> verticalRecoilCurve;
	TObjectPtr<UCurveFloat> horizontalRecoilCurve;
	if (IsValid(ActiveWeapon.Get()))
	{
		verticalRecoilCurve = ActiveWeapon->VerticalRecoilCurve;
		horizontalRecoilCurve = ActiveWeapon->HorizontalRecoilCurve;
	}
	_recoilCurveX += 0.1;

	if (verticalRecoilCurve && horizontalRecoilCurve)
	{
		_newVerticalRecoilAmount = verticalRecoilCurve->GetFloatValue(_recoilCurveX);
		_newHorizontalRecoilAmount = horizontalRecoilCurve->GetFloatValue(_recoilCurveX);
	}
	_verticalRecoilAmount = _newVerticalRecoilAmount - _oldVerticalRecoilAmount;
	_horizontalRecoilAmount = _newHorizontalRecoilAmount - _oldHorizontalRecoilAmount;

	if (Controller)
	{
		FRotator controllerRotation = Controller->GetControlRotation();
		Controller->SetControlRotation(FRotator(
			controllerRotation.Pitch + _verticalRecoilAmount,
			controllerRotation.Yaw + _horizontalRecoilAmount,
			controllerRotation.Roll
		));
	}

	_oldVerticalRecoilAmount = _newVerticalRecoilAmount;
	_oldHorizontalRecoilAmount = _newHorizontalRecoilAmount;
}

void AThirdPersonCharacter::CauseDamage(TObjectPtr<AActor> Target, float Damage,const FVector& HitFromDirection,const FHitResult& HitInfo,TSubclassOf<UDamageType> damageType)
{
	OnPlayerCauseDamage.Broadcast();
	//发通知
	UGameplayStatics::ApplyPointDamage(Target, Damage, HitFromDirection, HitInfo,
		GetController(), this, damageType);
}

void AThirdPersonCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AThirdPersonCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

#pragma region PlayerAction
void AThirdPersonCharacter::MoveForward(const FInputActionValue& Value)
{
	float ForwardValue = Value.Get<float>();
	
	if (Controller)
	{
		AddMovementInput(GetActorForwardVector(), ForwardValue);
	}
}

void AThirdPersonCharacter::MoveRight(const FInputActionValue& Value)
{
	float RightValue = Value.Get<float>();
	
	if(Controller)
	{
		AddMovementInput(GetActorRightVector(), RightValue);
	}
}

void AThirdPersonCharacter::Jump(const FInputActionValue& Value)
{
	//跳跃会疲劳，跳跃高度会随着距离上一次跳跃发生的时间而恢复，恢复时间为2秒

	//计算比例并且限制在一定范围内
	float JumpRate = FMath::Clamp(LastedJumpTilNow / JumpRecoveryTime, 0.5, 1);
	//设置此次跳跃的速度
	GetCharacterMovement()->JumpZVelocity = MaxJumpSpeed * JumpRate;
	//执行跳跃
	ACharacter::Jump();
	//设置定时器：每帧更新一次，持续JumpRecoveryTime秒
	LastedJumpTilNow = 0;
	GetWorld()->GetTimerManager().SetTimer(
		JumpTimerHandle,
		[this]()
		{
			LastedJumpTilNow += GetWorld()->GetDeltaSeconds();

			//在达到JumpRecoveryTime秒后停止计时
			if (LastedJumpTilNow >= JumpRecoveryTime)
			{
				GetWorld()->GetTimerManager().ClearTimer(JumpTimerHandle);
				LastedJumpTilNow = JumpRecoveryTime;//将LastedJumpTilNow设置为JumpRecoveryTime，避免下一次跳跃时计算错误
			}
		},
		GetWorld()->GetDeltaSeconds(), // 每帧执行（实际使用delta seconds）
		true   // 循环执行
	);
}

void AThirdPersonCharacter::ThirdLook(const FInputActionValue& Value)
{
	FVector2D LookVector = Value.Get<FVector2D>();
	if (Controller)
	{
		AddControllerYawInput(LookVector.X * MouseSensitivity);
		AddControllerPitchInput(LookVector.Y * MouseSensitivity);
	}
}

void AThirdPersonCharacter::Run(const FInputActionValue& Value)
{
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = RunSpeed; // Set running speed
	}
}

void AThirdPersonCharacter::StopRun(const FInputActionValue& Value)
{
	if(GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = WalkSpeed; // Set walking speed
	}
}

void AThirdPersonCharacter::Fire()
{
	if(IsValid( ActiveWeapon))
	{
		ActiveWeapon->TryShoot();
	}
}

void AThirdPersonCharacter::StopFire()
{
	ResetWeaponRecoil();
}

void AThirdPersonCharacter::Aim()
{
	UE_LOG(LogTemp, Warning, TEXT("PlayerAction: Aim"));
	bIsAiming = true;
}

void AThirdPersonCharacter::StopAim()
{
	UE_LOG(LogTemp, Warning, TEXT("PlayerAction: StopAim"));
	bIsAiming = false;
}


#pragma endregion

//视角切换
void AThirdPersonCharacter::ToggleCameraView()
{
	bIsFirstPerson = !bIsFirstPerson;
	if (bIsFirstPerson)//是第一人称
	{
		PlayerThirdCamera->SetActive(false);
		PlayerFirstCamera->SetActive(true);
	}
	else
	{
		PlayerFirstCamera->SetActive(false);
		PlayerThirdCamera->SetActive(true);
	}

	UpdateMeshVisiability();
}

void AThirdPersonCharacter::UpdateMeshVisiability()
{
	GetMesh()->SetOwnerNoSee(bIsFirstPerson);
	ArmMesh->SetOwnerNoSee(!bIsFirstPerson);
}

#pragma region Weapon
int AThirdPersonCharacter::FindWeaponIndex()
{
	uint8 index = 0;
	for (const auto& weapon : WeaponList)
	{
		if (weapon == nullptr)
		{
			return index;
		}
		index++;
	}
	return -1;
}

void AThirdPersonCharacter::EquipWeapon(AWeaponBase* weaponToEquip)
{
	if(weaponToEquip == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("WeaponToEquipIsNull"));
		return;
	}
	//判断装备到背包的哪个位置
	int equipIndex;
	equipIndex = FindWeaponIndex();
	weaponToEquip->ResetWeaponProperty();
	if(equipIndex == -1)
	{
		UE_LOG(LogTemp, Warning, TEXT("WeaponListHaveFull"));
	}
	else //装备武器成功
	{
		UE_LOG(LogTemp, Warning, TEXT("EquipWeaponToListSuccess"));
		weaponToEquip->OnWeaponBeEquipedBy(this);//调用武器类的被装备时将要发生的事件
		bIsEquip = true;
		weaponToEquip->WeaponOwner = this;//设置武器的拥有者
		WeaponList[equipIndex] = weaponToEquip;
		weaponToEquip->SetOwner(this);
	}

	if(ActiveWeapon == nullptr)//如果手上未持有武器
	{
		weaponToEquip->K2_AttachToComponent(ArmMesh, TEXT("GripPoint"),
			EAttachmentRule::SnapToTarget,
			EAttachmentRule::SnapToTarget,
			EAttachmentRule::SnapToTarget,
			true);
		ActiveWeapon = weaponToEquip;
	} 
}

void AThirdPersonCharacter::InitWeapon()
{
	UE_LOG(LogTemp, Warning, TEXT("ThirdPersonCharacter:InitWeapon"));
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.Owner = this;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	UClass* BlueprintVar = StaticLoadClass(AWeaponBase::StaticClass(), nullptr, TEXT("/Script/Engine.Blueprint'/Game/Weapons/MyWeapon/BP_StandardRifle.BP_StandardRifle_C'"));
	
	TObjectPtr<AWeaponBase> weapon = GetWorld()->SpawnActor<AWeaponBase>(BlueprintVar,
		GetActorTransform(),
		SpawnInfo);

	EquipWeapon(weapon);
}

void AThirdPersonCharacter::SwitchWeapon(int index)
{
}

void AThirdPersonCharacter::ResetWeaponRecoil()
{
	//重置武器后座力的相关属性
	_newVerticalRecoilAmount = 0;
	_oldVerticalRecoilAmount = 0;
	_verticalRecoilAmount = 0;
	_newHorizontalRecoilAmount = 0;
	_oldHorizontalRecoilAmount = 0;
	_horizontalRecoilAmount = 0;
	_recoilCurveX = 0;//后座力曲线的x坐标
}
#pragma endregion

