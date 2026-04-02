#include "IgnoreObstacleBuff.h"

#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "STR_RacerPawn.h"

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

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Blue, TEXT("Ignore obstacle activated!"));
	}

	UE_LOG(LogTemp, Warning, TEXT("[BUFF] IgnoreObstacle activated on %s | IgnoredActors=%d"), *GetNameSafe(Player), IgnoredActors.Num());

	Super::Activate(Player);
}

void UIgnoreObstacleBuff::OnBuffExpired()
{
	if (ASTR_RacerPawn* Racer = CachedRacer.Get())
	{
		Racer->SetIgnoreObstacleHits(false);

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

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Blue, TEXT("Ignore obstacle expired!"));
	}

	UE_LOG(LogTemp, Warning, TEXT("[BUFF] IgnoreObstacle expired on %s"), *GetNameSafe(CachedPlayer));

	IgnoredActors.Reset();
	CachedRacer.Reset();
	bHasSavedCollisionState = false;
	Super::OnBuffExpired();
}
