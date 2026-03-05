// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WheeledVehiclePawn.h"
#include "MyVehiclePawn.generated.h"

class UBuffBase;

/**
 *
 */
UCLASS()
class PROTOGAMELAB_API AMyVehiclePawn : public AWheeledVehiclePawn
{
    GENERATED_BODY()

public:
    AMyVehiclePawn();

protected:
    virtual void BeginPlay() override;

public:
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    UPROPERTY()
    UBuffBase* StoredBuff;

    UFUNCTION()
    void UseStoredBuff();

    void SetStoredBuff(UBuffBase* NewBuff);

    UFUNCTION(BlueprintCallable)
    bool HasBuff() const;
};
