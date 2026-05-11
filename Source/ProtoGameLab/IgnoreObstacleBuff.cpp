#include "IgnoreObstacleBuff.h"

#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "STR_RacerPawn.h"
#include "NiagaraFunctionLibrary.h"

void UIgnoreObstacleBuff::Activate(APawn* Player)
{
	ASTR_RacerPawn* Racer = Cast<ASTR_RacerPawn>(Player);
	if (!Racer || !Racer->BoxComp)
	{
		return;
	}

	CachedRacer = Racer;
	IgnoredActors.Reset();
	PreviousResponses = Racer->BoxComp->GetCollisionResponseToChannels();
	bPreviousNotifyRigidBodyCollision = Racer->BoxComp->BodyInstance.bNotifyRigidBodyCollision;
	bHasSavedCollisionState = true;

	auto IgnoreActorsWithTag = [&](const FName Tag)
	{
		TArray<AActor*> FoundActors;
		UGameplayStatics::GetAllActorsWithTag(Player, Tag, FoundActors);

		for (AActor* Actor : FoundActors)
		{
			if (!Actor || Actor == Racer)
			{
				continue;
			}

			Racer->BoxComp->IgnoreActorWhenMoving(Actor, true);
			IgnoredActors.AddUnique(Actor);
		}
	};

	IgnoreActorsWithTag(TEXT("TimeStopAffectable"));
	IgnoreActorsWithTag(TEXT("Obstacle"));

	Racer->SetIgnoreObstacleHits(true);
	Racer->BoxComp->SetCollisionResponseToAllChannels(ECR_Overlap);
	Racer->BoxComp->SetNotifyRigidBodyCollision(false);

	OnIgnoreObsatcleActivated();

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Blue, TEXT("Ignore obstacle activated!"));
	}
	if (IgnoreObstacleEffect && Racer)
	{
		ActiveEffect = UNiagaraFunctionLibrary::SpawnSystemAttached(
			IgnoreObstacleEffect,
			Racer->GetRootComponent(),
			NAME_None,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget,
			false // IMPORTANT: pas auto destroy
		);

		if (ActiveEffect)
		{
			ActiveEffect->SetAutoDestroy(false);
			ActiveEffect->Activate(true);
		}
	}

	// Event Blueprint (UI, sons, FX additionnels)
	OnIgnoreObstacleVFXActivated();

	UE_LOG(LogTemp, Warning, TEXT("[BUFF] IgnoreObstacle activated on %s | IgnoredActors=%d"), *GetNameSafe(Player), IgnoredActors.Num());

	Super::Activate(Player);
}

void UIgnoreObstacleBuff::OnBuffExpired()
{
    if (ASTR_RacerPawn* Racer = CachedRacer.Get())
    {
        Racer->SetIgnoreObstacleHits(false);

        // --- NOUVEAU : dépénétration avant de restaurer les collisions ---
        if (Racer->BoxComp && bHasSavedCollisionState)
        {
            UWorld* World = Racer->GetWorld();
            if (World)
            {
                FVector SafeLocation = Racer->GetActorLocation();
                FCollisionShape Shape = Racer->BoxComp->GetCollisionShape();
                FQuat Rotation = Racer->BoxComp->GetComponentQuat();

                // On cherche une position libre autour du racer (plusieurs directions)
                const TArray<FVector> EjectDirections = {
                    Racer->GetActorForwardVector(),
                    -Racer->GetActorForwardVector(),
                    Racer->GetActorRightVector(),
                    -Racer->GetActorRightVector()
                };

                FCollisionQueryParams QueryParams;
                QueryParams.AddIgnoredActor(Racer);

                bool bFoundSafeSpot = false;
                const float EjectStep = 50.f;
                const int32 MaxSteps = 10;

                for (const FVector& Dir : EjectDirections)
                {
                    for (int32 Step = 1; Step <= MaxSteps; ++Step)
                    {
                        FVector TestLocation = Racer->GetActorLocation() + Dir * EjectStep * Step;

                        // On vérifie qu'il n'y a pas de chevauchement à cette position
                        bool bOverlapping = World->OverlapBlockingTestByChannel(
                            TestLocation,
                            Rotation,
                            ECC_Pawn,
                            Shape,
                            QueryParams
                        );

                        if (!bOverlapping)
                        {
                            SafeLocation = TestLocation;
                            bFoundSafeSpot = true;
                            break;
                        }
                    }
                    if (bFoundSafeSpot) break;
                }

                if (bFoundSafeSpot)
                {
                    Racer->SetActorLocation(SafeLocation, false, nullptr, ETeleportType::TeleportPhysics);
                    UE_LOG(LogTemp, Warning, TEXT("[BUFF] IgnoreObstacle : racer éjecté vers position sûre."));
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("[BUFF] IgnoreObstacle : aucune position sûre trouvée, téléport de secours."));
                    // Fallback : utilise le dernier point sûr du racer s'il en a un
                    if (Racer->bHasSafeRecoveryPoint)
                    {
                        Racer->SetActorLocation(Racer->LastSafeLocation, false, nullptr, ETeleportType::TeleportPhysics);
                    }
                }
            }
        }
        // --- FIN dépénétration ---

        for (const TWeakObjectPtr<AActor>& IgnoredActor : IgnoredActors)
        {
            if (AActor* Actor = IgnoredActor.Get())
            {
                Racer->BoxComp->IgnoreActorWhenMoving(Actor, false);
            }
        }

        if (Racer->BoxComp && bHasSavedCollisionState)
        {
            Racer->BoxComp->SetCollisionResponseToChannels(PreviousResponses);
            Racer->BoxComp->SetNotifyRigidBodyCollision(bPreviousNotifyRigidBodyCollision);
        }
    }

	OnIgnoreObstacleExpired();

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Blue, TEXT("Ignore obstacle expired!"));
	}
	if (ActiveEffect)
	{
		ActiveEffect->DeactivateImmediate();
		ActiveEffect->DestroyComponent();
		ActiveEffect = nullptr;
	}

	UE_LOG(LogTemp, Warning, TEXT("[BUFF] IgnoreObstacle expired on %s"), *GetNameSafe(CachedRacer.Get()));

	IgnoredActors.Reset();
	CachedRacer.Reset();
	bHasSavedCollisionState = false;
	Super::OnBuffExpired();
}
