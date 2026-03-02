// Fill out your copyright notice in the Description page of Project Settings.


#include "MyVehiclePawn.h"
#include "BuffBase.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

AMyVehiclePawn::AMyVehiclePawn()
{
    StoredBuff = nullptr;
}

void AMyVehiclePawn::BeginPlay()
{
    Super::BeginPlay();
}

void AMyVehiclePawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    // Action pour utiliser le buff
    PlayerInputComponent->BindAction("UseBuff", IE_Pressed, this, &AMyVehiclePawn::UseStoredBuff);
}

void AMyVehiclePawn::UseStoredBuff()
{
    if (StoredBuff)
    {
        StoredBuff->Activate(this);
        StoredBuff = nullptr;

        UE_LOG(LogTemp, Warning, TEXT("Buff utilisé !"));
    }
    else
    {
        //UE_LOG(LogTemp, Warning, TEXT("Aucun buff stocké."));
    }
}

void AMyVehiclePawn::SetStoredBuff(UBuffBase* NewBuff)
{
    if (!StoredBuff && NewBuff)
    {
        StoredBuff = NewBuff;
        UE_LOG(LogTemp, Warning, TEXT("Buff stocké !"));
    }
}

bool AMyVehiclePawn::HasBuff() const
{
    return StoredBuff != nullptr;
}
