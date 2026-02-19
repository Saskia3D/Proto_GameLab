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

/**
 * 
 */
UENUM(BlueprintType)
enum class ERaceState : uint8
{
	Waiting,
	Running,
	Finished
};

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
	ERaceState GetRaceState() const { return RaceState; } // Permet aux joueurs de connaître l'état actuel de la course

	UFUNCTION(BlueprintCallable, Category = "Race")
	float GetRaceTimeSeconds() const;

	UFUNCTION(BlueprintCallable, Category = "Race")
	const TArray<FRaceFinishEntry>& GetFinishOrder() const { return FinishOrder; } // Permet aux joueurs de connaître l'ordre d'arrivée

	UFUNCTION(BlueprintCallable, Category = "Race")
	AActor* GetWinner() const; // Obtenir le gagnant de la course

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Race")
	ERaceState RaceState = ERaceState::Waiting; // L'état actuel de la course

	UPROPERTY()
	TArray<FRaceFinishEntry> FinishOrder; // L'ordre d'arrivée des joueurs

	UPROPERTY(EditAnywhere, Category = "Race|Camera")
	TObjectPtr<AActor> FinishedViewCameraActor = nullptr; // Caméra à utiliser pour les joueurs qui ont terminé la course

	UPROPERTY(EditAnywhere, Category = "Race|Camera")
	int32 NumPlayersToFinish = 1; // Le nombre de joueurs qui doivent terminer la course avant de la considérer comme terminée

	double StartTimeSeconds = 0.0; // Le temps auquel la course a commencé, en secondes
	void FreezeFinishedPlayer(AActor* PlayerActor); // Gèle le joueur qui a terminé la course pour éviter qu'il puisse continuer à jouer après avoir fini
	bool HasPlayerFinishedAlready(AActor* PlayerActor) const; // Vérifie si un joueur a déjà terminé la course

};
