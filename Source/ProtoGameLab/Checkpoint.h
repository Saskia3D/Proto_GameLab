// Checkpoint.h
// Cette classe représente un checkpoint dans le jeu, utilisé pour détecter lorsque les véhicules passent à travers les checkpoints sur la piste.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Checkpoint.generated.h"

class UBoxComponent;

UCLASS()
class PROTOGAMELAB_API ACheckpoint : public AActor
{
	GENERATED_BODY()

public:
	ACheckpoint();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Checkpoint")
	int32 CheckpointIndex = 0; // Index du checkpoint, utilisé pour identifier l'ordre des checkpoints dans la course

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Checkpoint")
	TObjectPtr<UBoxComponent> Trigger; // Composant de collision pour détecter les overlaps avec les véhicules

protected:
	virtual void BeginPlay() override;

	// Fonction appelée lorsqu'un autre acteur commence à chevaucher le composant Trigger
	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Sweep);
};
