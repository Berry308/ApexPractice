// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ThirdPersonCharacter.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayCauseDamage);

class UCameraComponent;
class USpringArmComponent;
class USkeletalMeshComponent;
class AWeaponBase;

struct FInputActionValue;
struct FTimerHandle;
struct FMath;

UCLASS()
class FIRSTPERSONSHOOT_API AThirdPersonCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AThirdPersonCharacter();

	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;


#pragma region InputAction
	//Character Movement
	void ThirdLook(const FInputActionValue& Value);
	void Run(const FInputActionValue& Value);
	void StopRun(const FInputActionValue& Value);
	void MoveForward(const FInputActionValue& Value);
	void MoveRight(const FInputActionValue& Value);
	void Jump(const FInputActionValue& Value);
	//Player Action
	void ToggleCameraView();
	void Fire();
	void StopFire();
	void Aim();//打开瞄准镜
	void StopAim();//停止瞄准

#pragma endregion

	void PlayArmsShootingAnimation();

	void PlayerShootingRecoil();
	
	void CauseDamage(TObjectPtr<AActor> Target,float Damage,const FVector& HitFromDirection,const FHitResult& HitInfo,TSubclassOf<UDamageType> damageType);

protected:
	UFUNCTION()
	void UpdateMeshVisiability();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;


private:
    #pragma region WeaponFuc
	UFUNCTION()
	void EquipWeapon(AWeaponBase * weaponToEquip);
	UFUNCTION()
	int FindWeaponIndex();
	UFUNCTION()
	void InitWeapon();
	UFUNCTION()
	void SwitchWeapon(int index);
	
	void ResetWeaponRecoil();
#pragma endregion



public:
#pragma region PlayerSetting
	UPROPERTY(EditAnywhere)
	bool bAutoReload = true;

	UPROPERTY(EditAnywhere)
	float MouseSensitivity = 0.3f;
#pragma endregion

#pragma region CharacterProperties
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "CharacterProperties")
	float RunSpeed = 1000.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "CharacterProperties")
	float WalkSpeed = 500.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "CharacterProperties")
	float MaxJumpSpeed = 420;
#pragma endregion

#pragma region CharacterState
	UPROPERTY(BlueprintReadOnly, Category = "CharacterState")
	bool bIsHoldingGun = false;

	UPROPERTY(BlueprintReadOnly, Category = "CharacterState")
	bool bIsCrouching = false;

	UPROPERTY(EditAnywhere, Category = "CharacterState")
	bool bIsEndlessAmmo = true;

	UPROPERTY(BlueprintReadOnly, Category = "CharacterState")
	bool bIsAiming;

	UPROPERTY(BlueprintReadOnly, Category = "CharacterState")
	float Speed = 0;
#pragma endregion

#pragma region Animation
	TObjectPtr<UAnimInstance> FPArmsAnimBP;//角色的动画蓝图，question:character类中有当前使用的动画蓝图类，当我视角切换的时候，动画蓝图的变化是什么？
#pragma endregion

	//UPROPERTY(EditAnywhere, Category = "Baggage")
	//int32 CurrentWeaponAmmo;//武器备弹

#pragma region Information
	UPROPERTY()
	float hitRate;

	UPROPERTY()
	int bulletShootNum;//子弹射击数量

	UPROPERTY()
	int bulletHitNum;//子弹命中数量
#pragma endregion

#pragma region Delegate
	FOnPlayCauseDamage OnPlayerCauseDamage;
#pragma endregion

	UPROPERTY(BlueprintReadOnly, Category = "Camera")
	bool bIsFirstPerson = true;//默认为第一人称

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "UnderlyingMechanism")//底层机制
	float JumpRecoveryTime = 1;


private:
    #pragma region Camera
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> PlayerFirstCamera;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> PlayerThirdCamera;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;
    #pragma endregion

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category= "Mesh", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> ArmMesh;

    #pragma region Weapon
	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<AWeaponBase>> WeaponList;
	
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	bool bIsEquip = false;

	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<AWeaponBase> ActiveWeapon;
    #pragma endregion
	
	//计时器
	FTimerHandle JumpTimerHandle;

	float LastedJumpTilNow;

	//后座力
	float _newVerticalRecoilAmount = 0;
	float _oldVerticalRecoilAmount = 0;
	float _verticalRecoilAmount = 0;
	float _newHorizontalRecoilAmount = 0;
	float _oldHorizontalRecoilAmount = 0;
	float _horizontalRecoilAmount = 0;
	float _recoilCurveX = 0;//后座力曲线的x坐标
};
