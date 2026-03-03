// Fill out your copyright notice in the Description page of Project Settings.

#include "BuffComponent.h"
#include "BuffBase.h"
#include "GameFramework/Pawn.h"
//#include "MyVehiclePawn.h"

// Sets default values for this component's properties
UBuffComponent::UBuffComponent()
{
    // Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
    // off to improve performance if you don't need them.
    PrimaryComponentTick.bCanEverTick = false;

    // ...
}

void UBuffComponent::AddBuff(TSubclassOf<UBuffBase> BuffClass)
{
    if (BuffClass && !CurrentBuff)
    {
        CurrentBuff = NewObject<UBuffBase>(this, BuffClass);
        UE_LOG(LogTemp, Warning, TEXT("Buff added: %s"), *BuffClass->GetName());
    }
}

void UBuffComponent::UseBuff()
{
    if (!CurrentBuff) return;

    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (OwnerPawn)
    {
        CurrentBuff->Activate(OwnerPawn);
    }

    CurrentBuff = nullptr;
}
