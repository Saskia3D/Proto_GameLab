/*
* FinishLine.h
* Ce fichier définit une classe d'acteur pour une ligne d'arrivée dans Unreal Engine.
*/

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/ArrowComponent.h"
#include "FinishLine.generated.h"

class UBoxComponent;

USTRUCT()
struct FLapData
{
	GENERATED_BODY()

	int32 LapNumber = 0; // Le numéro du tour
	float LastCrossTime = -99999999.f; // Le temps auquel le joueur a franchi la ligne d'arrivée pour ce tour, initialisé à une valeur très basse pour indiquer que le joueur n'a pas encore franchi la ligne d'arrivée pour ce tour
	bool bArmed = true;
};

UCLASS()
class PROTOGAMELAB_API AFinishLine : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AFinishLine();

	UFUNCTION(BlueprintCallable, Category = "Finish|Lap")
	void ArmForController(AController* Controller); // Arme la ligne d'arrivée pour un contrôleur spécifique, lui permettant de déclencher la ligne d'arrivée pour le prochain tour

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere)
	UBoxComponent* TriggerBox; // Le composant de collision pour détecter les overlaps

	UPROPERTY()
	TSet<TObjectPtr<AActor>> AlreadyTriggered; // Un ensemble pour suivre les acteurs qui ont déjà déclenché la ligne d'arrivée

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Finish", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UArrowComponent> ArrowComponent; // Un composant de flèche pour indiquer la direction de la ligne d'arrivée

	UPROPERTY(EditAnywhere, Category = "Finish|Direction")
	float MinForwardDot = 0.2f; // Le seuil de dot product pour vérifier si le véhicule est orienté dans la bonne direction

	UPROPERTY(EditAnywhere, Category = "Finish|Direction")
	float MinSpeed = 10.0f; // La vitesse minimale pour que le véhicule puisse déclencher la ligne d'arrivée

	UPROPERTY(EditAnywhere, Category = "Finish|Lap")
	int32 TotalLaps = 3; // Le nombre total de tours dans la course

	UPROPERTY(EditAnywhere, Category = "Finish|Lap")
	float LapCooldownSeconds = 0.75f; // Le temps de cooldown entre les tours pour éviter les déclenchements multiples

	UPROPERTY()
	TMap<TObjectPtr<AController>, FLapData> LapByController; // Un mapping pour suivre les données de tour de chaque contrôleur

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult); // Fonction pour gérer les overlaps

	bool IsPlayerVehicle(AActor* Actor) const; // Fonction pour vérifier si l'acteur est un véhicule du joueur
};
