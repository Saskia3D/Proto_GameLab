// TrackManager.h
// Cette classe est responsable de la gestion des pistes dans le jeu

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TrackManager.generated.h"

class ACheckpoint;

UCLASS()
class PROTOGAMELAB_API ATrackManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATrackManager();
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Race")
	TArray<TObjectPtr<ACheckpoint>> Checkpoints; // Liste des checkpoints sur la piste, assignés dans l'éditeur pour définir l'ordre des checkpoints dans la course

	UFUNCTION(BlueprintCallable, Category = "Race")
	int32 GetCheckpointCount() const { return Checkpoints.Num(); } // Fonction pour obtenir le nombre de checkpoints sur la piste

	UFUNCTION(BlueprintCallable, Category = "Race")
	ACheckpoint* GetCheckpoint(int32 Index) const { return Checkpoints.IsValidIndex(Index) ? Checkpoints[Index] : nullptr; }; // Fonction pour obtenir un checkpoint spécifique par son index, retourne nullptr si l'index n'est pas valide

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
