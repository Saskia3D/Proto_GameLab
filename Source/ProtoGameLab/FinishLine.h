/*
* Ce fichier définit une classe d'acteur pour une ligne d'arrivée dans Unreal Engine.
*/

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FinishLine.generated.h"

class UBoxComponent;

UCLASS()
class PROTOGAMELAB_API AFinishLine : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AFinishLine();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere)
	UBoxComponent* TriggerBox; // Le composant de collision pour détecter les overlaps

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult); // Fonction pour gérer les overlaps

	bool IsPlayerVehicle(AActor* Actor) const; // Fonction pour vérifier si l'acteur est un véhicule du joueur
};
