#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "MyObjectPool.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnPooledObjectSpawn, TObjectPtr<AActor>)
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPooledObjectReturn, TObjectPtr<AActor>)

UCLASS()
class FIRSTPERSONSHOOT_API UMyObjectPool : public UObject
{
    GENERATED_BODY()

public:
    UMyObjectPool();

    bool InitializeObjectPool(TSubclassOf<AActor> ActorClass, int32 PoolSize, bool bGrowIfEmpty = true);
    AActor* GetPooledObject(const FVector& SpawnLocation, const FRotator& SpawnRotator);
    void ReturnPooledObject(AActor* objectToReturn);
    void ClearPool();

public:
    FOnPooledObjectReturn OnPooledObjectReturn;
    FOnPooledObjectSpawn OnPooledObjectSpawn;

private:
    void SetObjectActive(AActor* object, bool isActive);
    AActor* AddNewObjectToPool(bool bActive);

private:
    UPROPERTY()
    TSubclassOf<AActor> PooledObjectClass;

    UPROPERTY()
    bool bCanGrow;

    UPROPERTY()
    TArray<AActor*> Pool;
};