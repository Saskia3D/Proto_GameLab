// TrackManager.cpp

#include "TrackManager.h"
#include "Checkpoint.h"

// Sets default values
ATrackManager::ATrackManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ATrackManager::BeginPlay()
{
	Super::BeginPlay();

	for (int32 i = 0; i < Checkpoints.Num(); ++i)
	{
		if (Checkpoints[i])
		{
			Checkpoints[i]->CheckpointIndex = i;

			UE_LOG(LogTemp, Warning, TEXT("TrackManager: %s assigned index %d"),
				*GetNameSafe(Checkpoints[i]), i);
		}
	}
}

// Called every frame
void ATrackManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

