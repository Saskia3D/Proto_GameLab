// RaceGameMode.cpp - Implémentation de la classe ARaceGameMode, qui gère la logique de la course, y compris le démarrage de la course, la notification des joueurs qui terminent, et le suivi de l'ordre d'arrivée. 

#include "RaceGameMode.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Components/PrimitiveComponent.h"

ARaceGameMode::ARaceGameMode() // Constructeur par défaut, peut être utilisé pour initialiser des variables ou des paramètres de jeu
{
}

void ARaceGameMode::BeginPlay()
{
	Super::BeginPlay();
	StartRace(); // Démarre la course dès que le jeu commence
	UE_LOG(LogTemp, Warning, TEXT("RaceGameMode BeginPlay (ACTIVE)")); // Log pour vérifier que le BeginPlay est appelé et que la course démarre correctement
}

void ARaceGameMode::StartRace()
{
	FinishOrder.Reset(); // Réinitialise l'ordre d'arrivée
	StartTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0; // Enregistre le temps de début de la course
	RaceState = ERaceState::Running; // Met à jour l'état de la course
}

float ARaceGameMode::GetRaceTimeSeconds() const
{
	if (RaceState == ERaceState::Running && GetWorld())
	{
		return GetWorld()->GetTimeSeconds() - StartTimeSeconds; // Retourne le temps écoulé depuis le début de la course
	}
	return 0.f; // Si la course n'est pas en cours, retourne 0
}

bool ARaceGameMode::HasPlayerFinishedAlready(AActor* PlayerActor) const
{
	for (const FRaceFinishEntry& Entry : FinishOrder)
	{
		if (Entry.PlayerActor == PlayerActor)
		{
			return true; // Le joueur a déjà terminé la course
		}
	}
	return false; // Le joueur n'a pas encore terminé la course
}

void ARaceGameMode::NotifyPlayerFinished(AActor* PlayerActor)
{
	if (RaceState != ERaceState::Running || !PlayerActor || HasPlayerFinishedAlready(PlayerActor) || !GetWorld())
	{
		return; // Ignore si la course n'est pas en cours, si le joueur est invalide ou s'il a déjà terminé
	}

	if (HasPlayerFinishedAlready(PlayerActor))
	{
		return; // Ignore si le joueur a déjà terminé la course
	}

	FRaceFinishEntry NewEntry; // Crée une nouvelle entrée pour l'ordre d'arrivée
	NewEntry.PlayerActor = PlayerActor; // Associe le joueur à l'entrée
	NewEntry.FinishTime = GetRaceTimeSeconds(); // Enregistre le temps de course du joueur
	FinishOrder.Add(NewEntry); // Ajoute l'entrée à l'ordre d'arrivée
	float FinishTime = GetRaceTimeSeconds(); // Récupère le temps de course du joueur

	UE_LOG(LogTemp, Log, TEXT("Finish: %s at time %.2f seconds"), *PlayerActor->GetName(), FinishTime); // Affiche un message de log avec le nom du joueur et son temps de course

	/*
	* Si au moins un joueur a terminé, on peut considérer que la course est terminée
	  (Si on veut attendre que tous les joueurs terminent, il faudrait ajouter une condition pour vérifier le nombre total de joueurs)
	*/
	if (FinishOrder.Num() >= 1)
	{
		RaceState = ERaceState::Finished; // Met à jour l'état de la course
	}

	const int32 Position = FinishOrder.Num(); // Position du joueur dans l'ordre d'arrivée
	const TCHAR* Suffix = Position == 1 ? TEXT("1st") : TEXT("2nd"); // Suffixe pour indiquer la position (1st, 2nd, 3rd, etc.)

	if (GEngine)
	{
		// Affiche un message à l'écran pour le joueur qui a terminé, indiquant sa position et son temps de course
		const FString Message = FString::Printf(TEXT("%s has finished in position %s with a time of %.2f seconds!"), *PlayerActor->GetName(), Suffix, NewEntry.FinishTime);

		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, Message);
	}

	if(APawn* Pawn = Cast<APawn>(PlayerActor))
	{
		if(APlayerController* PC = Cast<APlayerController>(Pawn->GetController()))
		{
			Pawn->DisableInput(PC); // Désactive les entrées du joueur qui a terminé la course pour éviter qu'il puisse continuer à jouer après avoir fini

			// Stoppe la physique
			if (UPrimitiveComponent* PrimitiveComp = Cast<UPrimitiveComponent>(Pawn->GetRootComponent()))
			{
				PC->UnPossess(); // Détache le contrôleur du joueur de son véhicule pour éviter toute interaction après la fin de la course
				PrimitiveComp->SetPhysicsLinearVelocity(FVector::ZeroVector); // Arrête la simulation physique du véhicule du joueur qui a terminé
				PrimitiveComp->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector); // Arrête la rotation du véhicule
			}
		}
	}

	if (FinishOrder.Num() >= 1) //Mettre a 2 si on veut attendre que les 2 joueurs terminent la course
	{
		RaceState = ERaceState::Finished; // Met à jour l'état de la course si les 2 joueurs ont terminés

		if (GEngine)
		{
			const FString MessageFin = FString::Printf(TEXT("Race Finished! Winner: %s with a time of %.2f seconds!"), *FinishOrder[0].PlayerActor->GetName(), FinishOrder[0].FinishTime);
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, MessageFin); // Affiche un message à l'écran indiquant le gagnant de la course et son temps
		}
	}
}

AActor* ARaceGameMode::GetWinner() const
{
	if (FinishOrder.Num() > 0)
	{
		return FinishOrder[0].PlayerActor; // Retourne le joueur qui a terminé en premier
	}
	return nullptr; // Si personne n'a terminé, retourne nullptr
}

