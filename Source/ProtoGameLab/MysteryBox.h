// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MysteryBox.generated.h"

class UBuffBase;
class AMyVehiclePawn;
class UBoxComponent;

UCLASS()
class PROTOGAMELAB_API AMysteryBox : public AActor
{
    GENERATED_BODY()

public:
    AMysteryBox();

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere)
    class UBoxComponent* CollisionBox;

    UPROPERTY(EditAnywhere)
    TArray<TSubclassOf<class UBuffBase>> PossibleBuffs;

    UFUNCTION()
    void OnOverlapBegin(UPrimitiveComponent* OverlappedComp,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex,
        bool bFromSweep,
        const FHitResult& SweepResult);
};
