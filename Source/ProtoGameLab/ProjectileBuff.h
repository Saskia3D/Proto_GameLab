// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BuffBase.h"
#include "ProjectileBuff.generated.h"

/**
 * 
 */
class AProjectileActor;

UCLASS()
class PROTOGAMELAB_API UProjectileBuff : public UBuffBase
{
    GENERATED_BODY()

public:
    virtual void Activate(APawn* Player) override;

    void FireProjectile();

    int32 RemainingShots = 3;


protected:
    UPROPERTY(EditDefaultsOnly)
    TSubclassOf<AProjectileActor> ProjectileClass;

};
