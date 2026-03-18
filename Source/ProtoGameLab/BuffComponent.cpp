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
    if (!BuffClass) return;

    if (CurrentBuff)
    {
        UE_LOG(LogTemp, Warning, TEXT("[BUFF] Player already has a stored buff, ignoring new one."));
        return;
    }

    CurrentBuff = NewObject<UBuffBase>(this, BuffClass);

    UE_LOG(LogTemp, Warning, TEXT("[BUFF] Stored buff added: %s"), *BuffClass->GetName());
}

UBuffBase* UBuffComponent::FindActiveBuffByClass(UClass* BuffClass) const
{
    if (!BuffClass) return nullptr;

    for (UBuffBase* Buff : ActiveBuffs)
    {
        if (Buff && Buff->GetClass() == BuffClass)
        {
            return Buff;
        }
    }

    return nullptr;
}

void UBuffComponent::UseBuff()
{
    if (!CurrentBuff) return;

    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (!OwnerPawn) return;

    UClass* BuffClass = CurrentBuff->GetClass();

    // si un buff du meme type est deja actif, on refresh juste sa duree
    if (UBuffBase* ExistingBuff = FindActiveBuffByClass(BuffClass))
    {
        ExistingBuff->RefreshDuration();

        UE_LOG(LogTemp, Warning, TEXT("[BUFF] Refreshed active buff instead of stacking: %s"),
            *BuffClass->GetName());

        CurrentBuff = nullptr;
        return;
    }

    UBuffBase* BuffToUse = CurrentBuff;

    BuffToUse->Activate(OwnerPawn);

    if (BuffToUse->IsActive())
    {
        ActiveBuffs.Add(BuffToUse);

        UE_LOG(LogTemp, Warning, TEXT("[BUFF] Activated buff: %s | ActiveBuffs = %d"),
            *GetNameSafe(BuffToUse),
            ActiveBuffs.Num());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[BUFF] Buff activation failed: %s"),
            *GetNameSafe(BuffToUse));
    }

    /*
	if (OwnerPawn)
	{
		CurrentBuff->Activate(OwnerPawn);
	}*/

    CurrentBuff = nullptr;
}

void UBuffComponent::NotifyBuffExpired(UBuffBase* ExpiredBuff)
{
    if (!ExpiredBuff) return;

    ActiveBuffs.RemoveSingle(ExpiredBuff);

    UE_LOG(LogTemp, Warning, TEXT("[BUFF] Removed expired buff: %s | ActiveBuffs = %d"),
        *GetNameSafe(ExpiredBuff),
        ActiveBuffs.Num());
}
