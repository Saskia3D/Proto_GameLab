// Fill out your copyright notice in the Description page of Project Settings.


#include "BuffBase.h"
#include "MyVehiclePawn.h"
#include "TimerManager.h"
#include "Engine/World.h"

void UBuffBase::Activate(AMyVehiclePawn* Player)
{
	if (!Player) return;

	// Appliquer effet ici
	CachedPlayer = Player;

	Player->GetWorld()->GetTimerManager().SetTimer(
		DurationHandle,
		this,
		&UBuffBase::OnBuffExpired,
		Duration,
		false
		);
}

void UBuffBase::OnBuffExpired() 
{
	if (!CachedPlayer) return;

	UE_LOG(LogTemp, Warning, TEXT("Buff Expired on %s"), *CachedPlayer->GetName());

	CachedPlayer = nullptr;
}