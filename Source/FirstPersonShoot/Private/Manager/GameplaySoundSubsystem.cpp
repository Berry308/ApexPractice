// Fill out your copyright notice in the Description page of Project Settings.


#include "Manager/GameplaySoundSubsystem.h"
#include "Engine.h"
#include "Character/EnemyCharacter.h"


void UGameplaySoundSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	UE_LOG(LogTemp, Warning, TEXT("GameplaySoundSubsystem:Initialize success"));
	Super::Initialize(Collection);
	SoundDataTable = LoadObject<UDataTable>(this,TEXT("/Game/Audio/DataTable/DT_Sound"));
	if (!IsValid(SoundDataTable.Get()))
	{
		UE_LOG(LogTemp, Error, TEXT("SoundDataTable get failed"));
	}
}

void UGameplaySoundSubsystem::PlayBulletHitSound(AActor* HitedActor, const FHitResult& hitResult, bool isHeadShot)
{
	//我无法在此时知道子弹是否能够击杀敌人
	//判断Actor类型
	TObjectPtr<AEnemyCharacter> hitedEnemy = Cast<AEnemyCharacter>(HitedActor);

	//击中敌人
	if (IsValid(hitedEnemy.Get()))
	{
		if (isHeadShot)//如果击中头部，覆盖其他击中部位的音效
		{
			//if (isKillEnemy)
			//{
			//	//播放爆头击杀音效
			//	SoundToPlay = FindSound("EventSound", "HeadShotKill");
			//	UGameplayStatics::PlaySound2D(this, SoundToPlay);
			//	return;
			//}
			//else
			//{
				//播放HeadShot音效
				SoundToPlay = FindSound("HitSound", "HeadShot");
				UGameplayStatics::PlaySound2D(this, SoundToPlay);
			//}
		}
		else
		{
			bool isEnemyCracked = hitedEnemy->isCracked;
			if (isEnemyCracked)
			{
				//播放打肉的声音
				SoundToPlay = FindSound("HitSound", "BulletHitBlood");
				UGameplayStatics::PlaySound2D(this, SoundToPlay);
			}
			else
			{
				//播放击中护甲的声音
				SoundToPlay = FindSound("HitSound", "BulletHitShield");
				UGameplayStatics::PlaySound2D(this, SoundToPlay);
			}
		}
		//if (isKillEnemy)
		//{
		//	//播放击杀音效
		//	SoundToPlay = FindSound("EventSound", "KillEnemy");
		//	UGameplayStatics::PlaySound2D(this, SoundToPlay);
		//}
	}

	//击中物体
}

void UGameplaySoundSubsystem::PlayEnemyCrackedSound()
{
	SoundToPlay = FindSound("EventSound", "EnemyCracked");
	UGameplayStatics::PlaySound2D(this, SoundToPlay);
}

void UGameplaySoundSubsystem::PlayEnemyDieSound(bool isHeadShot)
{
	if (isHeadShot)
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayEnemyDieSound:HeadShot"));
		SoundToPlay = FindSound("EventSound", "HeadShotKillEnemy");
		UGameplayStatics::PlaySound2D(this, SoundToPlay);
	}
	else
	{
		SoundToPlay = FindSound("EventSound", "KillEnemy");
		UGameplayStatics::PlaySound2D(this, SoundToPlay);
	}
}

USoundBase* UGameplaySoundSubsystem::FindSound(FName SoundType, FName SoundName)
{
	if (!SoundDataTable)
	{
		UE_LOG(LogTemp, Warning, TEXT("GameplaySoundSubsystem:FindSound() has failed"));
		return nullptr;
	}

	FString contextString;
	TArray<FSoundInfo*> soundArray;
	SoundDataTable->GetAllRows<FSoundInfo>(contextString, soundArray);

	for (auto& row : soundArray)
	{
		if (row->SoundType == SoundType && row->SoundName == SoundName)
		{
			return row->Sound;
		}
	}

	UE_LOG(LogTemp, Error, TEXT("GameplaySoundSubsystem:FindSound() has failed"));
	return nullptr;
}

