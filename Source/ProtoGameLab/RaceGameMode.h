/*
* RaceGameMode.h - D�claration de la classe ARaceGameMode, qui g�re la logique d'une course dans Unreal Engine.
*   Ce fichier d�finit une classe de mode de jeu pour une course dans Unreal Engine.
*  Il inclut des �num�rations pour l'�tat de la course, une structure pour enregistrer les joueurs qui ont termin� la course et leur temps,
*  ainsi que des fonctions pour d�marrer la course, notifier quand un joueur termine, obtenir l'�tat de la course, le temps de la course,
*  l'ordre d'arriv�e et le gagnant.La classe h�rite de AGameModeBase et utilise des macros Unreal pour l'int�gration avec le moteur.
*/

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RaceLeaderboardEntry.h"
#include "RaceGameMode.generated.h"

class ATrackSplineActor;

// �num�ration pour repr�senter l'�tat de la course
UENUM(BlueprintType)
enum class ERaceState : uint8
{
	Waiting,
	Running,
	Finished
};

// Structure pour enregistrer les joueurs qui ont termin� la course et leur temps
USTRUCT(BlueprintType)
struct FRaceFinishEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<AActor> PlayerActor = nullptr; // Le joueur qui a termin� la course

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<AController> Controller = nullptr; // Le contr�leur du joueur, peut �tre utilis� pour acc�der � des informations suppl�mentaires sur le joueur ou pour lui envoyer des messages

	UPROPERTY(BlueprintReadOnly)
	float FinishTime = 0.f; // Le temps de course du joueur, en secondes
};

// Structure pour suivre la progression de chaque joueur dans la course
USTRUCT(BlueprintType)
struct FPlayerRaceProgress
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Race")
	int32 Lap = 0; // Le tour actuel du joueur

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Race")
	int32 LastCheckpoint = -1; // L'index du dernier checkpoint que le joueur a franchi, initialis� � -1 pour indiquer qu'il n'a pas encore franchi de checkpoint

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Race")
	float DistanceToNext = 999999999999.f; // La distance actuelle du joueur au prochain checkpoint, utilis�e pour d�terminer la position relative des joueurs dans la course, initialis�e � une valeur tr�s �lev�e pour indiquer que le joueur n'est pas encore proche du prochain checkpoint

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Race")
	float SplineDistance = 0.f; //distance projetee du joueur (tie break)

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Race")
	float SplineAlpha = 0.f; //utils UI

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Race")
	int32 Score = 0; //Score total du joueur

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Race")
	int32 CheckpointsPassedCount = 0; //Nombre de checkpoints passes

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Race")
	int32 LapsCompletedCount = 0; //Nombre de tours completes

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Race")
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
	void StartRace(); // D�marre la course, appel�e par le serveur

	UFUNCTION(BlueprintCallable, Category = "Race|Start")
	bool IsRaceRunning() const { return RaceState == ERaceState::Running; }

	UFUNCTION(BlueprintCallable, Category = "Race")
	void NotifyPlayerFinished(AActor* PlayerActor); // Appel�e par les joueurs lorsqu'ils terminent la course

	UFUNCTION(BlueprintCallable, Category = "Race")
	const TArray<FRaceFinishEntry>& GetFinishOrder() const { return FinishOrder; } // Permet aux joueurs de conna�tre l'ordre d'arriv�e

	UFUNCTION(BlueprintCallable, Category = "Race")
	void NotifyCheckpointPassed(APawn* PlayerPawn, int32 CheckpointIndex); // Appel�e par les joueurs lorsqu'ils passent un checkpoint, utilis�e pour suivre leur progression dans la course

	UFUNCTION(BlueprintCallable, Category = "Race")
	void UpdatePositions(); // Met � jour les positions des joueurs dans la course en fonction de leur progression et de leur distance au prochain checkpoint

	UFUNCTION(BlueprintCallable, Category = "Race")
	void NotifyLapCompleted(AController* Controller, int32 NewLapNumber);

	UFUNCTION(BlueprintCallable, Category = "Race")
	AActor* GetWinner() const; // Obtenir le gagnant de la course

	UFUNCTION(BlueprintCallable, Category = "Race")
	ERaceState GetRaceState() const { return RaceState; } // Permet aux joueurs de conna�tre l'�tat actuel de la course

	UFUNCTION(BlueprintCallable, Category = "Race")
	float GetRaceTimeSeconds() const;

	//Getters utiles pour systeme pointage
	UFUNCTION(BlueprintCallable, Category = "Race|Score")
	int32 GetPlayerScore(AController* Controller) const;

	UFUNCTION(BlueprintCallable, Category = "Race|Score")
	int32 GetPlayerCheckpointCount(AController* Controller) const;

	UFUNCTION(BlueprintCallable, Category = "Race|Score")
	int32 GetPlayerLapCount(AController* Controller) const;

	UFUNCTION(BlueprintCallable, Category = "Race")
	bool IsControllerFinished(AController* Controller) const;

	UFUNCTION(BlueprintCallable, Category = "Race|Position")
	int32 GetPlayerRacePosition(AController* Controller) const;

	UFUNCTION(BlueprintCallable, Category = "Race|Position")
	AController* GetCurrentLeader() const;

	UFUNCTION(BlueprintCallable, Category = "Race|Position")
	bool IsControllerAheadOf(AController* A, AController* B) const;

	UFUNCTION(BlueprintCallable, Category = "Race|UI")
	int32 GetDisplayedLapForController(AController* Controller) const;

	UFUNCTION(BlueprintCallable, Category = "Race|UI")
	int32 GetRaceTotalLaps() const;

	UFUNCTION(BlueprintCallable, Category = "Race|UI")
	bool HasControllerFinishedRace(AController* Controller) const;

	const FPlayerRaceProgress* GetPlayerProgress(AController* Controller) const; //Recupere toute la progression
	TArray<FRaceLeaderboardEntry> BuildLeaderboardSnapshot() const;

protected:
	virtual void BeginPlay() override;

	// Map pour suivre la progression de chaque joueur dans la course, associant chaque acteur de joueur � sa progression (tour actuel, dernier checkpoint franchi, distance au prochain checkpoint)
	UPROPERTY(EditAnywhere, Category = "Race")
	TObjectPtr<class ATrackManager> TrackManager = nullptr;

	// Classe de TrackManager � utiliser, assign�e dans l'�diteur pour permettre au GameMode de cr�er une instance du TrackManager au d�but de la course
	UPROPERTY(EditDefaultsOnly, Category = "Race")
	TSubclassOf<ATrackManager> TrackManagerClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Race|Start")
	bool bAutoStartRaceOnBeginPlay = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Race|Score")
	int32 PointsPerCheckpoint = 100; //Nombre de points par checkpoint passe

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Race|Score")
	int32 PointsPerLap = 500; //Nombre de points par tour complete

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Race|End")
	bool bUseFinishCountdown = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Race|End")
	float FinishCountdownSeconds = 15.f;

private:
	UPROPERTY(VisibleAnywhere, Category = "Race")
	ERaceState RaceState = ERaceState::Waiting; // L'�tat actuel de la course

	UPROPERTY(VisibleAnywhere, Category = "Race")
	TMap<TObjectPtr<AController>, FPlayerRaceProgress> ProgressByController;

	UPROPERTY()
	TArray<FRaceFinishEntry> FinishOrder; // L'ordre d'arriv�e des joueurs

	UPROPERTY(EditAnywhere, Category = "Race|Camera")
	TObjectPtr<AActor> FinishedViewCameraActor = nullptr; // Cam�ra � utiliser pour les joueurs qui ont termin� la course

	UPROPERTY(EditAnywhere, Category = "Race|Camera")
	int32 NumPlayersToFinish = 2; // Le nombre de joueurs qui doivent terminer la course avant de la consid�rer comme termin�e

	UPROPERTY(EditAnywhere, Category = "Race")
	TObjectPtr<ATrackSplineActor> TrackSplineActor = nullptr;

	void RefreshControllerProgress(AController* Controller);
	void RefreshAllPlayerProgress();
	float ComputeSplineDistance(APawn* Pawn) const;
	float ComputeSplineAlpha(APawn* Pawn) const;
	TArray<AController*> GetRaceControllers() const;

	double StartTimeSeconds = 0.0; // Le temps auquel la course a commenc�, en secondes
	void FreezeFinishedPlayer(AActor* PlayerActor); // G�le le joueur qui a termin� la course pour �viter qu'il puisse continuer � jouer apr�s avoir fini
	bool HasPlayerFinishedAlready(AActor* PlayerActor) const; // V�rifie si un joueur a d�j� termin� la course
	bool bHasTriggeredEndMenu = false; // Indique si le menu de fin de course a d�j� �t� d�clench� pour �viter de le d�clencher plusieurs fois
	bool bFinishCountdownStarted = false;
	void EndRace();
	void CacheLeaderboardForEndMenu();
	void GatherRaceControllers(TArray<AController*>& OutControllers) const;
	int32 FindFinishOrderIndex(AController* Controller) const;
	float GetFinishTimeForController(AController* Controller) const;
	FTimerHandle FinishCountdownHandle;
	int32 CompareControllers(AController* A, AController* B) const; // Compare deux contr�leurs pour d�terminer leur ordre dans la course, en fonction de leur progression et de leur distance au prochain checkpoint
	float ComputeDistanceToNextCheckpoint(APawn* Pawn, int32 LastCheckpoint) const; // Calcule la distance d'un joueur au prochain checkpoint, utilis�e pour d�terminer sa position relative dans la course
};


