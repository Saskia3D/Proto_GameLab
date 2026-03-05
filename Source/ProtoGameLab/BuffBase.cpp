// BaseBuff.cpp

#include "BuffBase.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "TimerManager.h"

void UBuffBase::Activate(APawn* Player)
{
	if (!Player) return;

	CachedPlayer = Player;

	UWorld* World = Player->GetWorld();
	if (!World) return;

	World->GetTimerManager().SetTimer(
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