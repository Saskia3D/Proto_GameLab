#include "VHSBuff.h"
#include "STR_RacerPawn.h"
#include "Camera/CameraComponent.h"
#include "Kismet/GameplayStatics.h"

void UVHSBuff::Activate(APawn* Player)
{
	if (!Player) return;

	CachedPlayer = Player;

	ApplyVHS();

	UE_LOG(LogTemp, Warning, TEXT("VHS Activated"));

	Super::Activate(Player);
}

void UVHSBuff::ApplyVHS()
{
	UWorld* World = CachedPlayer ? CachedPlayer->GetWorld() : nullptr;
	if (!World || !VHSMaterial) return;

	TArray<AActor*> Pawns;
	UGameplayStatics::GetAllActorsOfClass(World, APawn::StaticClass(), Pawns);

	for (AActor* A : Pawns)
	{
		if (!A || A == CachedPlayer) continue;

		ASTR_RacerPawn* Racer = Cast<ASTR_RacerPawn>(A);
		if (!Racer || !Racer->CameraComp) continue;

		Racer->CameraComp->PostProcessSettings.WeightedBlendables.Array.Add(
			FWeightedBlendable(1.0f, VHSMaterial)
		);

		AffectedPlayers.Add(Racer);
	}
}

void UVHSBuff::OnBuffExpired()
{
	RemoveVHS();

	UE_LOG(LogTemp, Warning, TEXT("VHS Expired"));

	Super::OnBuffExpired();
}

void UVHSBuff::RemoveVHS()
{
	for (auto& WeakRacer : AffectedPlayers)
	{
		ASTR_RacerPawn* Racer = WeakRacer.Get();
		if (!Racer || !Racer->CameraComp) continue;

		auto& Array = Racer->CameraComp->PostProcessSettings.WeightedBlendables.Array;

		Array.RemoveAll([this](const FWeightedBlendable& WB)
			{
				return WB.Object == VHSMaterial;
			});
	}

	AffectedPlayers.Empty();
}
