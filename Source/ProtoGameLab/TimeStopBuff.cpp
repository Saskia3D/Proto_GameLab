// TimeStopBuff.cpp

#include "TimeStopBuff.h"
#include "MyVehiclePawn.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

void UTimeStopBuff::Activate(APawn* Player)
{
	if (!Player) return; //verification

	CachedPlayer = Player;

	ApplyTimeStop();

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Blue, TEXT("TimeStop Activated!"));
	}

	UE_LOG(LogTemp, Warning, TEXT("TimeStop activated by %s"), *GetNameSafe(CachedPlayer));

	//Timer
	Super::Activate(Player);
}

void UTimeStopBuff::ApplyTimeStop()
{
	UE_LOG(LogTemp, Warning, TEXT("[TS] Start ApplyTimeStop"));

	UObject* WorldContext = CachedPlayer.Get();
	UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (!World) return;

	SavedDilations.Empty();

	//Geler les autres joueurs
	TArray<AActor*> Pawns;
	UGameplayStatics::GetAllActorsOfClass(CachedPlayer.Get(), APawn::StaticClass(), Pawns);
	UE_LOG(LogTemp, Warning, TEXT("[TS] Pawns found = %d"), Pawns.Num());

	for (AActor* A : Pawns)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TS] Pawn: %s  IsCached=%d"),
			*GetNameSafe(A), 
			(A == CachedPlayer));

		if (!A || A == CachedPlayer) continue;

		SavedDilations.Add(A, A->CustomTimeDilation);
		A->CustomTimeDilation = 0.001f;
	}

	//Geler les obstacles
	TArray<AActor*> Affectables;
	UGameplayStatics::GetAllActorsWithTag(CachedPlayer.Get(), AffectableTag, Affectables);

	for (AActor* A : Affectables)
	{
		if(!A || A == CachedPlayer) continue;

		if(SavedDilations.Contains(A)) continue;

		if (A->CustomTimeDilation <= 0.001f)
		{
			UE_LOG(LogTemp, Warning, TEXT("[TS] Skipping already frozen affectable: %s"), *GetNameSafe(A));
			continue;
		}

		SavedDilations.Add(A, A->CustomTimeDilation);
		A->CustomTimeDilation = 0.001f;
	}
}

void UTimeStopBuff::OnBuffExpired()
{
	RestoreTimeStop();

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Blue, TEXT("TimeStop Expired!"));
	}

	UE_LOG(LogTemp, Warning, TEXT("TimeStop Expired (instigator was %s)"), *GetNameSafe(CachedPlayer));

	Super::OnBuffExpired();
}

void UTimeStopBuff::RestoreTimeStop()
{
	for(auto& Pair : SavedDilations)
	{
		AActor* A = Pair.Key.Get();
		if(!A) continue;

		float RestoredValue = Pair.Value;

		if (FMath::IsNearlyZero(RestoredValue))
		{
			RestoredValue = 1.0f;
		}

		A->CustomTimeDilation = RestoredValue;

		UE_LOG(LogTemp, Warning, TEXT("[TS] Restored %s to %f"),
			*GetNameSafe(A), RestoredValue);
	}

	SavedDilations.Empty();
}