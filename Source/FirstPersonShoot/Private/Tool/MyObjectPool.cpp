#include "Tool/MyObjectPool.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

UMyObjectPool::UMyObjectPool()
{
    bCanGrow = true;
}

void UMyObjectPool::SetObjectActive(AActor* object, bool bActive)
{
    if (object)
    {
        object->SetActorHiddenInGame(!bActive);
        object->SetActorEnableCollision(bActive);
        object->SetActorTickEnabled(bActive);
    }
}

bool UMyObjectPool::InitializeObjectPool(TSubclassOf<AActor> ActorClass, int32 PoolSize, bool bGrowIfEmpty)
{
    UE_LOG(LogTemp, Warning, TEXT("InitializeObjectPool"));

    if (!ActorClass || PoolSize == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("ActorClass is not exist"));
        return false;
    }

    PooledObjectClass = ActorClass;
    bCanGrow = bGrowIfEmpty;
    Pool.Empty();

    for (int i = 0; i < PoolSize; i++)
    {
        AActor* NewObject = AddNewObjectToPool(false);
        if (NewObject)
        {
            Pool.Push(NewObject);
        }
    }
    return true;
}

AActor* UMyObjectPool::GetPooledObject(const FVector& SpawnLocation, const FRotator& SpawnRotator)
{
    //UE_LOG(LogTemp, Warning, TEXT("MyObjectPool:GetPooledObject"));
    AActor* objectGetFromPool = nullptr;

    for (AActor* object : Pool)
    {
        if (IsValid(object) &&
            !object->GetActorEnableCollision() &&
            !object->IsActorTickEnabled() &&
            object->IsHidden())
        {
            objectGetFromPool = object;
            break;
        }
    }

    if (!IsValid(objectGetFromPool) && bCanGrow)
    {
        objectGetFromPool = AddNewObjectToPool(false);
        if (IsValid(objectGetFromPool))
        {
            Pool.Push(objectGetFromPool);
        }
    }

    if (IsValid(objectGetFromPool))
    {
        SetObjectActive(objectGetFromPool, true);
        objectGetFromPool->SetActorLocationAndRotation(SpawnLocation, SpawnRotator);
        OnPooledObjectSpawn.Broadcast(objectGetFromPool);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("ObjectPool: Pool is empty and cannot grow!"));
    }

    return objectGetFromPool;
}

void UMyObjectPool::ReturnPooledObject(AActor* objectToReturn)
{
    if (!IsValid(objectToReturn) || !Pool.Contains(objectToReturn))
    {
        UE_LOG(LogTemp, Warning, TEXT("The object returning to the objectpool is invaild or not belong to this objectpool!"));
        return;
    }

    SetObjectActive(objectToReturn, false);

    objectToReturn->SetActorLocation(FVector::ZeroVector);

    OnPooledObjectReturn.Broadcast(objectToReturn);
}

void UMyObjectPool::ClearPool()
{
    //UObject没有直接的World访问权限
    for (AActor* object : Pool)
    {
        if (object)
        {
            object->Destroy();
        }
    }
    
    Pool.Empty();
}

AActor* UMyObjectPool::AddNewObjectToPool(bool bActive)
{
    //由于UObject不持有对世界的引用（与Actor不同），所以需要根据对象的Outer链找到有效的世界上下文，Outer在UObject创建时就被指定
    UWorld* World = GEngine->GetWorldFromContextObject(this, EGetWorldErrorMode::LogAndReturnNull);
    if (!World || !PooledObjectClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("AddNewObjectToPool have failed"));
        return nullptr;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AActor* OuterActor = Cast<AActor>(GetOuter());//获取这个object所属的UObject，即对象池的拥有者
    if (OuterActor)
    {
        SpawnParams.Owner = OuterActor;//将子弹的所属者设置为对象池的拥有者，即武器
    }

    AActor* NewActor = World->SpawnActor<AActor>(
        PooledObjectClass,
        FVector::ZeroVector,
        FRotator::ZeroRotator,
        SpawnParams
    );

    if (NewActor)
    {
        SetObjectActive(NewActor, bActive);
    }

    return NewActor;
}



