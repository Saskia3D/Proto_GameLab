// RaceGameMode.cpp - Implémentation de la classe ARaceGameMode, qui gère la logique de la course, y compris le démarrage de la course, la notification des joueurs qui terminent, et le suivi de l'ordre d'arrivée. 

#include "RaceGameMode.h"
#include "TrackManager.h"
#include "Checkpoint.h"
#include "Kismet/GameplayStatics.h"
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

	// Si une classe de TrackManager est assignée dans l'éditeur, crée une instance de TrackManager pour gérer les checkpoints et la progression de la course
	if (TrackManagerClass)
	{
		TrackManager = GetWorld()->SpawnActor<ATrackManager>(TrackManagerClass);
	}

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

	FRaceFinishEntry NewEntry; // Crée une nouvelle entrée pour l'ordre d'arrivée
	NewEntry.PlayerActor = PlayerActor; // Associe le joueur à l'entrée
	NewEntry.FinishTime = GetRaceTimeSeconds(); // Enregistre le temps de course du joueur
	FinishOrder.Add(NewEntry); // Ajoute l'entrée à l'ordre d'arrivée
	float FinishTime = GetRaceTimeSeconds(); // Récupère le temps de course du joueur

	UE_LOG(LogTemp, Log, TEXT("Finish: %s at time %.2f seconds"), *PlayerActor->GetName(), FinishTime); // Affiche un message de log avec le nom du joueur et son temps de course

	const int32 Position = FinishOrder.Num(); // Position du joueur dans l'ordre d'arrivée
	const TCHAR* Suffix = Position == 1 ? TEXT("1st") : TEXT("2nd"); // Suffixe pour indiquer la position (1st, 2nd, 3rd, etc.)

	if (GEngine)
	{
		// Affiche un message à l'écran pour le joueur qui a terminé, indiquant sa position et son temps de course
		const FString Message = FString::Printf(TEXT("%s has finished in position %s with a time of %.2f seconds!"), *PlayerActor->GetName(), Suffix, NewEntry.FinishTime);

		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, Message);
	}

	FreezeFinishedPlayer(PlayerActor); // Gèle le joueur qui a terminé la course pour éviter qu'il puisse continuer à jouer après avoir fini

	/*
	* Si au moins un joueur a terminé, on peut considérer que la course est terminée
	  (Si on veut attendre que tous les joueurs terminent, il faudrait ajouter une condition pour vérifier le nombre total de joueurs)
	*/
	if (FinishOrder.Num() >= NumPlayersToFinish && !bHasTriggeredEndMenu)
	{
		bHasTriggeredEndMenu = true; // Empêche de déclencher le menu de fin de course plusieurs fois
		RaceState = ERaceState::Finished;
		UGameplayStatics::OpenLevel(GetWorld(), FName("RaceEndMenu"));

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

void ARaceGameMode::FreezeFinishedPlayer(AActor* PlayerActor)
{
	if (!PlayerActor) return; // Vérifie que l'acteur est valide

	// Cast l'acteur en APawn pour accéder à son contrôleur
	APawn* Pawn = Cast<APawn>(PlayerActor);
	if (!Pawn) return;

	// Cast le contrôleur en APlayerController pour accéder aux fonctions d'input
	APlayerController* PC = Cast<APlayerController>(Pawn->GetController());
	if (!PC) return;

	PC->SetViewTargetWithBlend(FinishedViewCameraActor, 0.5f); // Change la vue du joueur pour la caméra de fin
	PC->UnPossess(); // Désassocie le contrôleur du pawn pour empêcher toute interaction future

	// Stop la physique si active
	if (UPrimitiveComponent* PrimitiveComp = Cast<UPrimitiveComponent>(Pawn->GetRootComponent()))
	{
		if (PrimitiveComp->IsSimulatingPhysics())
		{
			PrimitiveComp->SetPhysicsLinearVelocity(FVector::ZeroVector);
			PrimitiveComp->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
		}
	}

	// Passer à une caméra de fin
	if (FinishedViewCameraActor)
	{
		PC->SetViewTargetWithBlend(FinishedViewCameraActor, 0.5f);
	}
}

void ARaceGameMode::NotifyCheckpointPassed(APawn* PlayerPawn, int32 CheckpointIndex)
{
	// Log pour vérifier que la fonction est appelée correctement et pour suivre les checkpoints franchis par les joueurs
	UE_LOG(LogTemp, Warning, TEXT("NotifyCheckpointPassed: Pawn=%s CP=%d"),
		*GetNameSafe(PlayerPawn), CheckpointIndex);
    if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green,
			FString::Printf(TEXT("CP %d"), CheckpointIndex));
	}

	if (!PlayerPawn) return; // Vérifie que le pawn est valide
	AController* Controller = PlayerPawn->GetController(); // Récupère le contrôleur du joueur
	if (!Controller) return; // Vérifie que le contrôleur est valide

	FPlayerRaceProgress& Progress = ProgressByController.FindOrAdd(Controller); // Récupère ou crée une entrée de progression pour ce contrôleur

	const int32 NextExpectedCheckpoint = (Progress.LastCheckpoint < 0) ? 0 : (Progress.LastCheckpoint + 1); // Détermine l'index du prochain checkpoint attendu

	// A modifier plus tard?
	if(CheckpointIndex != NextExpectedCheckpoint)
	{
		UE_LOG(LogTemp, Warning, TEXT("Player %s passed checkpoint %d but expected %d"), *Controller->GetName(), CheckpointIndex, NextExpectedCheckpoint);
		return; // Ignore si le checkpoint franchi n'est pas celui attendu
	}

	Progress.LastCheckpoint = CheckpointIndex; // Met à jour le dernier checkpoint franchi

	Progress.DistanceToNext = ComputeDistanceToNextCheckpoint(PlayerPawn, Progress.LastCheckpoint); // Met à jour la distance au prochain checkpoint pour ce joueur, utilisée pour déterminer sa position relative dans la course

	UpdatePositions(); // Met à jour les positions des joueurs dans la course en fonction de leur progression et de leur distance au prochain checkpoint
}

float ARaceGameMode::ComputeDistanceToNextCheckpoint(APawn* PlayerPawn, int32 LastCheckpoint) const
{
	if (!TrackManager || !PlayerPawn) return 0.f; // Vérifie que le TrackManager et le Pawn sont valides

	const int32 NextIndex = LastCheckpoint + 1; // Détermine l'index du prochain checkpoint
	ACheckpoint* NextCheckpoint = TrackManager->GetCheckpoint(NextIndex); // Récupère le prochain checkpoint à partir du TrackManager
	if (!NextCheckpoint) return 999999999999.f;

	return FVector::Dist(PlayerPawn->GetActorLocation(), NextCheckpoint->GetActorLocation()); // Calcule et retourne la distance entre le joueur et le prochain checkpoint
}

int32 ARaceGameMode::CompareControllers(AController* A, AController* B) const
{
	const FPlayerRaceProgress* ProgressA = ProgressByController.Find(A); // Récupère la progression du joueur A
	const FPlayerRaceProgress* ProgressB = ProgressByController.Find(B); // Récupère la progression du joueur B

	if (!ProgressA || !ProgressB) return 0; // Si l'un des joueurs n'a pas de progression enregistrée, les considérer comme égaux

	if (ProgressA->Lap != ProgressB->Lap) return (ProgressA->Lap > ProgressB->Lap) ? 1 : -1; // Le joueur avec le tour le plus élevé est en avance

	if (ProgressA->LastCheckpoint != ProgressB->LastCheckpoint) return (ProgressA->LastCheckpoint > ProgressB->LastCheckpoint) ? 1 : -1; // Le joueur avec le checkpoint le plus élevé est en avance

	if (ProgressA->DistanceToNext != ProgressB->DistanceToNext) return (ProgressA->DistanceToNext < ProgressB->DistanceToNext) ? 1 : -1; // Le joueur avec la distance au prochain checkpoint la plus faible est en avance

	return 0; // Si les deux joueurs ont la même progression, les considérer comme égaux
}

void ARaceGameMode::UpdatePositions()
{
	// Log pour vérifier que la fonction est appelée correctement et pour suivre les mises à jour de position des joueurs
	UE_LOG(LogTemp, Warning, TEXT("UpdatePositions called"));
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Yellow, TEXT("UpdatePositions()"));

	TArray<AActor*> PlayerControllers; // Tableau pour stocker les acteurs des joueurs
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerController::StaticClass(), PlayerControllers); // Récupère tous les acteurs de type APlayerController dans le monde

	// Log pour vérifier le nombre de joueurs trouvés
	UE_LOG(LogTemp, Warning, TEXT("Player count = %d"), PlayerControllers.Num());
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow,
		FString::Printf(TEXT("Player count = %d"), PlayerControllers.Num()));

	if (PlayerControllers.Num() < 2) return; // Si il n'y a pas au moins 2 joueurs, pas besoin de mettre à jour les positions

	AController* ControllerA = Cast<AController>(PlayerControllers[0]); // Récupère le contrôleur du premier joueur
	AController* ControllerB = Cast<AController>(PlayerControllers[1]); // Récupère le contrôleur du deuxième joueur

	if (!ControllerA || !ControllerB) return; // Vérifie que les contrôleurs sont valides

	if (APawn* P1 = ControllerA->GetPawn())
		ProgressByController.FindOrAdd(ControllerA).DistanceToNext = ComputeDistanceToNextCheckpoint(P1, ProgressByController.FindOrAdd(ControllerA).LastCheckpoint);
	if (APawn* P2 = ControllerB->GetPawn())
		ProgressByController.FindOrAdd(ControllerB).DistanceToNext = ComputeDistanceToNextCheckpoint(P2, ProgressByController.FindOrAdd(ControllerB).LastCheckpoint);

	const int32 Result = CompareControllers(ControllerA, ControllerB);

	if (GEngine)
	{
		const TCHAR* Lead = (Result >= 0) ? TEXT("P1 est devant!") : TEXT("P2 est devant!");
		GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Cyan, Lead);
	}
}


