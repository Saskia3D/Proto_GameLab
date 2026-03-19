/* 
* RaceGameMode.h - Déclaration de la classe ARaceGameMode, qui gère la logique d'une course dans Unreal Engine.
*   Ce fichier définit une classe de mode de jeu pour une course dans Unreal Engine.
*  Il inclut des énumérations pour l'état de la course, une structure pour enregistrer les joueurs qui ont terminé la course et leur temps,
*  ainsi que des fonctions pour démarrer la course, notifier quand un joueur termine, obtenir l'état de la course, le temps de la course, 
*  l'ordre d'arrivée et le gagnant.La classe hérite de AGameModeBase et utilise des macros Unreal pour l'intégration avec le moteur.
*/

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RaceGameMode.generated.h"

// Énumération pour représenter l'état de la course
UENUM(BlueprintType)
enum class ERaceState : uint8
{
	Waiting,
	Running,
	Finished
};

// Structure pour enregistrer les joueurs qui ont terminé la course et leur temps
USTRUCT(BlueprintType)
struct FRaceFinishEntry
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<AActor> PlayerActor = nullptr; // Le joueur qui a terminé la course

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<AController> Controller = nullptr; // Le contrôleur du joueur, peut être utilisé pour accéder à des informations supplémentaires sur le joueur ou pour lui envoyer des messages

	UPROPERTY(BlueprintReadOnly)
	float FinishTime = 0.f; // Le temps de course du joueur, en secondes
};

// Structure pour suivre la progression de chaque joueur dans la course
USTRUCT(BlueprintType)
struct FPlayerRaceProgress
{
	GENERATED_BODY()

	int32 Lap = 0; // Le tour actuel du joueur
	int32 LastCheckpoint = -1; // L'index du dernier checkpoint que le joueur a franchi, initialisé à -1 pour indiquer qu'il n'a pas encore franchi de checkpoint
	float DistanceToNext = 999999999999.f; // La distance actuelle du joueur au prochain checkpoint, utilisée pour déterminer la position relative des joueurs dans la course, initialisée à une valeur très élevée pour indiquer que le joueur n'est pas encore proche du prochain checkpoint

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Race")
	int32 Score = 0; //Score total du joueur

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Race")
	int32 CheckpointsPassedCount = 0; //Nombre de checkpoints passes

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Race")
	int32 LapsCompletedCount = 0; //Nombre de tours completes

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Race")
	bool bFinishedRace = false;
};

// Classe de mode de jeu pour la course
UCLASS()
class PROTOGAMELAB_API ARaceGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ARaceGameMode();

	// Fonctions de gestion de la course
	UFUNCTION(BlueprintCallable, Category = "Race")
	void StartRace(); // Démarre la course, appelée par le serveur

	UFUNCTION(BlueprintCallable, Category = "Race")
	void NotifyPlayerFinished(AActor* PlayerActor); // Appelée par les joueurs lorsqu'ils terminent la course

	UFUNCTION(BlueprintCallable, Category = "Race")
	const TArray<FRaceFinishEntry>& GetFinishOrder() const { return FinishOrder; } // Permet aux joueurs de connaître l'ordre d'arrivée

	UFUNCTION(BlueprintCallable, Category = "Race")
	void NotifyCheckpointPassed(APawn* PlayerPawn, int32 CheckpointIndex); // Appelée par les joueurs lorsqu'ils passent un checkpoint, utilisée pour suivre leur progression dans la course

	UFUNCTION(BlueprintCallable, Category = "Race")
	void UpdatePositions(); // Met à jour les positions des joueurs dans la course en fonction de leur progression et de leur distance au prochain checkpoint

	UFUNCTION(BlueprintCallable, Category="Race")
    void NotifyLapCompleted(AController* Controller, int32 NewLapNumber);

	UFUNCTION(BlueprintCallable, Category = "Race")
	AActor* GetWinner() const; // Obtenir le gagnant de la course

	UFUNCTION(BlueprintCallable, Category = "Race")
	ERaceState GetRaceState() const { return RaceState; } // Permet aux joueurs de connaître l'état actuel de la course

	UFUNCTION(BlueprintCallable, Category = "Race")
	float GetRaceTimeSeconds() const;

	//Getters utiles pour systeme pointage
	UFUNCTION(BlueprintCallable, Category="Race|Score")
	int32 GetPlayerScore(AController* Controller) const;

	UFUNCTION(BlueprintCallable, Category="Race|Score")
	int32 GetPlayerCheckpointCount(AController* Controller) const;

	UFUNCTION(BlueprintCallable, Category="Race|Score")
	int32 GetPlayerLapCount(AController* Controller) const;

	UFUNCTION(BlueprintCallable, Category="Race")
	bool IsControllerFinished(AController* Controller) const;

	const FPlayerRaceProgress* GetPlayerProgress(AController* Controller) const; //Recupere toute la progression

protected:
	virtual void BeginPlay() override;

	// Map pour suivre la progression de chaque joueur dans la course, associant chaque acteur de joueur à sa progression (tour actuel, dernier checkpoint franchi, distance au prochain checkpoint)
	UPROPERTY(EditAnywhere, Category = "Race")
	TObjectPtr<class ATrackManager> TrackManager = nullptr;

	// Classe de TrackManager à utiliser, assignée dans l'éditeur pour permettre au GameMode de créer une instance du TrackManager au début de la course
	UPROPERTY(EditDefaultsOnly, Category = "Race")
	TSubclassOf<ATrackManager> TrackManagerClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Race|Score")
	int32 PointsPerCheckpoint = 100; //Nombre de points par checkpoint passe

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Race|Score")
	int32 PointsPerLap = 500; //Nombre de points par tour complete

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Race|End")
	bool bUseFinishCountdown = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Race|End")
	float FinishCountdownSeconds = 15.f;

private:
	UPROPERTY(VisibleAnywhere, Category = "Race")
	ERaceState RaceState = ERaceState::Waiting; // L'état actuel de la course

	UPROPERTY(VisibleAnywhere, Category = "Race")
	TMap<TObjectPtr<AController>, FPlayerRaceProgress> ProgressByController;

	UPROPERTY()
	TArray<FRaceFinishEntry> FinishOrder; // L'ordre d'arrivée des joueurs

	UPROPERTY(EditAnywhere, Category = "Race|Camera")
	TObjectPtr<AActor> FinishedViewCameraActor = nullptr; // Caméra à utiliser pour les joueurs qui ont terminé la course

	UPROPERTY(EditAnywhere, Category = "Race|Camera")
	int32 NumPlayersToFinish = 2; // Le nombre de joueurs qui doivent terminer la course avant de la considérer comme terminée

	double StartTimeSeconds = 0.0; // Le temps auquel la course a commencé, en secondes
	void FreezeFinishedPlayer(AActor* PlayerActor); // Gèle le joueur qui a terminé la course pour éviter qu'il puisse continuer à jouer après avoir fini
	bool HasPlayerFinishedAlready(AActor* PlayerActor) const; // Vérifie si un joueur a déjà terminé la course
	bool bHasTriggeredEndMenu = false; // Indique si le menu de fin de course a déjà été déclenché pour éviter de le déclencher plusieurs fois
	bool bFinishCountdownStarted = false;
	void EndRace();
	FTimerHandle FinishCountdownHandle;
	int32 CompareControllers(AController* A, AController* B) const; // Compare deux contrôleurs pour déterminer leur ordre dans la course, en fonction de leur progression et de leur distance au prochain checkpoint
	float ComputeDistanceToNextCheckpoint(APawn* Pawn, int32 LastCheckpoint) const; // Calcule la distance d'un joueur au prochain checkpoint, utilisée pour déterminer sa position relative dans la course
};
