// BaseBuff.cpp

#include "BuffBase.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "BuffComponent.h"

void UBuffBase::Activate(APawn* Player)
{
	if (!Player) return;

	CachedPlayer = Player;
	bIsActive = true;

	StartDurationTimer();
	/*
	UWorld* World = Player->GetWorld();
	if (!World) return;

	World->GetTimerManager().SetTimer(
		DurationHandle,
		this,
		&UBuffBase::OnBuffExpired,
		Duration,
		false
	);*/
}

void UBuffBase::RefreshDuration()
{
	if (!CachedPlayer) return;

	StartDurationTimer();

	UE_LOG(LogTemp, Warning, TEXT("[BUFF] Duration refreshed for %s on %s"),
		*GetClass()->GetName(),
		*GetNameSafe(CachedPlayer));
}

void UBuffBase::StartDurationTimer()
{
	UWorld* World = CachedPlayer ? CachedPlayer->GetWorld() : nullptr;
    if (!World) return;

	World->GetTimerManager().ClearTimer(DurationHandle);
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
	if (CachedPlayer)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BUFF] Expired: %s on %s"),
			*GetClass()->GetName(),
			*GetNameSafe(CachedPlayer));

		if (UBuffComponent* BuffComp = CachedPlayer->FindComponentByClass<UBuffComponent>())
		{
			BuffComp->NotifyBuffExpired(this);
		}
	}

	bIsActive = false;
	CachedPlayer = nullptr;
}