// Fill out your copyright notice in the Description page of Project Settings.
//武器系统概述
/*
* 我需不需要让发射出来的子弹网格体来告诉武器已经击中敌人？子弹网格体的渲染是根据已经完成的射线物理模拟，在完成射线模拟
后，已经知道了命中的结果，我只需要延迟结果的产生就足够了。那么使用计时器？但是一个计时器对应一个子弹结果，如果创建多个
计时器肯定是不行的。那么，我还是应该让子弹告诉武器类击中的时刻并且调用对应函数，让子弹负责视觉的渲染，根据击中信息来决
定特效，然后回调武器类中定义的函数来对目标造成伤害。
* 武器决定命中特效？子弹决定命中特效？命中的物体决定命中特效？根据命中物体的种类决定特效和音效？
解决方案：定义一个蓝图接口，包含了一个函数用来播放特效和音效，让部分需要播放指定特效的特殊物体都实现这个接口和函数。子
弹类先尝试获取该接口，调用接口函数由命中的Actor来实现特效播放。如果获取接口失败，那就根据命中信息FHitResult获取表面物理
材质并解析SurfaceType，查询某个全局管理器单例中持有的数据资产DA_ImpactEffects，使用 UGameplayStatics::SpawnSoundAtLocation 
和 UNiagaraFunctionLibrary::SpawnSystemAtLocation 在命中点（Hit.ImpactPoint）生成效果。
*/

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponBase.generated.h"


UENUM()
enum class EWeaponType :uint8//为什么要继承自uint8（无符号八位整数）？节省内存。网络复制的需求。与反射和属性系统集成，告诉引擎该数据类型
{
	StandardRifle UMETA(DisplayName = "StandardRifle")
};

class USkeletalMeshComponent;
class USphereComponent;
class UAnimMontage;
class USoundBase;
class UParticleSystem;
class UMyObjectPool;

class AThirdPersonCharacter;
class AProjectileBase;

UCLASS()
class FIRSTPERSONSHOOT_API AWeaponBase : public AActor
{
	GENERATED_BODY()
	
public:	
	AWeaponBase();
#pragma region WeaponProperty
	UPROPERTY(EditAnywhere, Category = "WeaponProperty")
	EWeaponType WeaponType;//editor assign

	UPROPERTY(EditAnywhere, Category = "WeaponProperty")
	float WeaponDamage;

	UPROPERTY(EditAnywhere, Category = "WeaponProperty")
	TSubclassOf<AProjectileBase> WeaponProjectileClass;

	UPROPERTY(EditAnywhere, Category = "WeaponProperty")
	int32 MaxClipAmmo;//最大弹夹子弹数量

	UPROPERTY(EditAnywhere, Category = "WeaponProperty")
	float ShootFrequency;//射击频率，两次射击之间的间隔

	UPROPERTY(EditAnywhere, Category = "WeaponProperty")
	float BulletMaxLifeTime;//射程(子弹最大存活时间)

	UPROPERTY(EditAnywhere, Category = "WeaponProperty")
	float BulletRadius;//子弹的半径大小

	UPROPERTY(EditAnywhere, Category = "WeaponProperty")
	float BulletInitialSpeed;//子弹初速度

	UPROPERTY(EditAnywhere, Category = "WeaponProperty")
	float BulletGravity;//子弹的下坠

	UPROPERTY(EditAnywhere, Category = "WeaponProperty")
	float WeaponSpreadAngle;//武器腰射散布

	UPROPERTY(EditAnywhere, Category = "WeaponProperty")
	TObjectPtr<UCurveFloat> VerticalRecoilCurve;//垂直后座力曲线

	UPROPERTY(EditAnywhere, Category = "WeaponProperty")
	TObjectPtr<UCurveFloat> HorizontalRecoilCurve;//水平后座力曲线
	


#pragma endregion

	UPROPERTY(EditAnywhere, Category = "WeaponState")
	int32 CurrentClipAmmo;//当前弹夹子弹数量

	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	TObjectPtr<USkeletalMeshComponent> WeaponMesh;

	UPROPERTY(EditAnywhere)
	TObjectPtr<USphereComponent> SphereCollision;

	UPROPERTY(EditAnywhere)
	TObjectPtr<AThirdPersonCharacter> WeaponOwner;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UAnimMontage> ArmsShootingMontage;//不同武器有不同的手臂射击动画

	UPROPERTY(EditAnywhere)
	TObjectPtr<USoundBase> WeaponShootingSound;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UParticleSystem> MuzzleFlash;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UDamageType> HeadShotDamage;

	UPROPERTY(EditAnywhere, Category = "Test")
	bool bIsTrajectoryDraw = false;

protected:
	UPROPERTY()
	TObjectPtr<UMyObjectPool> ProjectileObjectPool;

private:
	UPROPERTY()
	TArray<TWeakObjectPtr<const AActor>> ActorToIgnore;//使用弱指针，以便当持有武器的角色类被消灭时，避免悬空指针的发生

	UPROPERTY()
	TWeakObjectPtr<AActor> CurrentActorToIgnored;

	//计时器
	FTimerHandle _fireTimerHandle;
	float _lastShootTilNow = BIG_NUMBER;

public:	
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable)
	void ResetWeaponProperty();

	UFUNCTION()
	void TryShoot();

	UFUNCTION(BlueprintImplementableEvent, Category = "WeaponAnimation")//蓝图实现，动画也由蓝图指定
	void PlayShootAnimation();
	
	UFUNCTION()
	void PlayWeaponEffect();

	UFUNCTION()
	void OnWeaponBeEquipedBy(AActor* NewOwner);

	UFUNCTION()
	void OnWeaponBeUnEquiped();

	void BulletReachTarget(TObjectPtr<AProjectileBase> Bullet, bool bIsBulletHit,
		const FHitResult& HitResult, const FVector& BulletShootDirection);//子弹网格体到达目标后调用的函数，判断是否击中物体，播放对应特效(音效)，造成伤害

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	bool ShootValidate();

	bool SimulatePhysicsTrajectory(
		const FVector& OriginalPosition, const FVector& Direction,
		float InitialSpeed, float GravityScale,
		float MaxLifetime, float Radius,
		FHitResult& OutHitResult, float& OutTimeToHit, TArray<FVector>& OutTrajectoryPoints) const;//使用引用将需要的数据传递出去

	void PlayProjectileLaunch(const FRotator& SpawnDirection,
		const TArray<FVector>& Trajectory, float FLightTime,
		const FHitResult& HitResult, bool bIsHit);

	float CalculateDamage(TObjectPtr<UPhysicalMaterial> PhysicalMaterial);
};
