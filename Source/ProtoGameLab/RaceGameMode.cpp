// RaceGameMode.cpp - Implémentation de la classe ARaceGameMode, qui gère la logique de la course, y compris le démarrage de la course, la notification des joueurs qui terminent, et le suivi de l'ordre d'arrivée. 

#include "RaceGameMode.h"
#include "TrackManager.h"
#include "Checkpoint.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Components/PrimitiveComponent.h"
#include "TimerManager.h"

ARaceGameMode::ARaceGameMode()
{
}

void ARaceGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (!TrackManager)
	{
		TrackManager = Cast<ATrackManager>(
			UGameplayStatics::GetActorOfClass(GetWorld(), ATrackManager::StaticClass())
		);
	}

	if (!TrackManager && TrackManagerClass)
	{
		TrackManager = GetWorld()->SpawnActor<ATrackManager>(TrackManagerClass);
	}

	UE_LOG(LogTemp, Warning, TEXT("RaceGameMode TrackManager = %s (CPCount=%d)"),
		*GetNameSafe(TrackManager),
		TrackManager ? TrackManager->GetCheckpointCount() : -1);

	StartRace();
	UE_LOG(LogTemp, Warning, TEXT("RaceGameMode BeginPlay (ACTIVE)"));
}

void ARaceGameMode::StartRace()
{
	FinishOrder.Reset();
	ProgressByController.Reset();

	StartTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	RaceState = ERaceState::Running;

	bHasTriggeredEndMenu = false;
	bFinishCountdownStarted = false;

	if (GetWorld())
	{
		GetWorldTimerManager().ClearTimer(FinishCountdownHandle);
	}
}

float ARaceGameMode::GetRaceTimeSeconds() const
{
	if ((RaceState == ERaceState::Running || RaceState == ERaceState::Finished) && GetWorld())
	{
		return GetWorld()->GetTimeSeconds() - StartTimeSeconds;
	}
	return 0.f;
}

bool ARaceGameMode::HasPlayerFinishedAlready(AActor* PlayerActor) const
{
	for (const FRaceFinishEntry& Entry : FinishOrder)
	{
		if (Entry.PlayerActor == PlayerActor)
		{
			return true;
		}
	}
	return false;
}

bool ARaceGameMode::IsControllerFinished(AController* Controller) const
{
	if (!Controller) return false;

	const FPlayerRaceProgress* Progress = ProgressByController.Find(Controller);
	return Progress ? Progress->bFinishedRace : false;
}

void ARaceGameMode::NotifyPlayerFinished(AActor* PlayerActor)
{
	if (RaceState != ERaceState::Running || !PlayerActor || HasPlayerFinishedAlready(PlayerActor) || !GetWorld())
	{
		return;
	}

	AController* Controller = nullptr;
	if (APawn* Pawn = Cast<APawn>(PlayerActor))
	{
		Controller = Pawn->GetController();
	}

	if (!Controller)
	{
		UE_LOG(LogTemp, Warning, TEXT("[FINISH] NotifyPlayerFinished ignored: no controller"));
		return;
	}

	FPlayerRaceProgress& Progress = ProgressByController.FindOrAdd(Controller);
	if (Progress.bFinishedRace)
	{
		return;
	}

	Progress.bFinishedRace = true;

	FRaceFinishEntry NewEntry;
	NewEntry.PlayerActor = PlayerActor;
	NewEntry.Controller = Controller;
	NewEntry.FinishTime = GetRaceTimeSeconds();
	FinishOrder.Add(NewEntry);

	const int32 FinalScore = GetPlayerScore(Controller);

	UE_LOG(LogTemp, Warning,
		TEXT("[FINISH SCORE] Player=%s | Controller=%s | FinalScore=%d | Position=%d | Time=%.2f"),
		*GetNameSafe(PlayerActor),
		*GetNameSafe(Controller),
		FinalScore,
		FinishOrder.Num(),
		NewEntry.FinishTime);

	const int32 Position = FinishOrder.Num();
	FString PositionText = TEXT("th");
	if (Position == 1) PositionText = TEXT("1st");
	else if (Position == 2) PositionText = TEXT("2nd");
	else if (Position == 3) PositionText = TEXT("3rd");
	else PositionText = FString::Printf(TEXT("%dth"), Position);

	if (GEngine)
	{
		const FString Message = FString::Printf(
			TEXT("%s FINISHED! Position: %s | Time: %.2fs | Score: %d"),
			*GetNameSafe(PlayerActor),
			*PositionText,
			NewEntry.FinishTime,
			FinalScore
		);

		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, Message);
	}

	// On démarre le compte à rebours seulement quand le premier joueur finit
	if (!bFinishCountdownStarted && bUseFinishCountdown)
	{
		bFinishCountdownStarted = true;

		UE_LOG(LogTemp, Warning, TEXT("[RACE END] Finish countdown started: %.2fs"), FinishCountdownSeconds);

		GetWorldTimerManager().SetTimer(
			FinishCountdownHandle,
			this,
			&ARaceGameMode::EndRace,
			FinishCountdownSeconds,
			false
		);

		if (GEngine)
		{
			const FString CountdownMsg = FString::Printf(
				TEXT("Final countdown started! Race ends in %.0f seconds."),
				FinishCountdownSeconds
			);
			GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Yellow, CountdownMsg);
		}
	}

	// Si tous les joueurs attendus ont fini, fin immédiate
	if (FinishOrder.Num() >= NumPlayersToFinish)
	{
		EndRace();
	}
}

void ARaceGameMode::EndRace()
{
	if (bHasTriggeredEndMenu || !GetWorld())
	{
		return;
	}

	bHasTriggeredEndMenu = true;
	RaceState = ERaceState::Finished;

	GetWorldTimerManager().ClearTimer(FinishCountdownHandle);

	if (GEngine && FinishOrder.Num() > 0)
	{
		const FString MessageFin = FString::Printf(
			TEXT("Race Finished! Winner: %s | Time: %.2fs"),
			*GetNameSafe(FinishOrder[0].PlayerActor),
			FinishOrder[0].FinishTime
		);
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, MessageFin);
	}

	UE_LOG(LogTemp, Warning, TEXT("[RACE END] Opening RaceEndMenu"));
	UGameplayStatics::OpenLevel(GetWorld(), FName("RaceEndMenu"));
}

AActor* ARaceGameMode::GetWinner() const
{
	if (FinishOrder.Num() > 0)
	{
		return FinishOrder[0].PlayerActor;
	}
	return nullptr;
}

void ARaceGameMode::NotifyCheckpointPassed(APawn* PlayerPawn, int32 CheckpointIndex)
{
	UE_LOG(LogTemp, Warning, TEXT("NotifyCheckpointPassed: Pawn=%s CP=%d"),
		*GetNameSafe(PlayerPawn), CheckpointIndex);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green,
			FString::Printf(TEXT("CP %d"), CheckpointIndex));
	}

	if (!PlayerPawn) return;

	AController* Controller = PlayerPawn->GetController();
	if (!Controller) return;

	FPlayerRaceProgress& Progress = ProgressByController.FindOrAdd(Controller);

	if (Progress.bFinishedRace)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CHECKPOINT] Ignored because player already finished: %s"), *GetNameSafe(Controller));
		return;
	}

	const int32 NextExpectedCheckpoint = (Progress.LastCheckpoint < 0) ? 0 : (Progress.LastCheckpoint + 1);

	if (CheckpointIndex != NextExpectedCheckpoint)
	{
		UE_LOG(LogTemp, Warning, TEXT("Player %s passed checkpoint %d but expected %d"),
			*Controller->GetName(), CheckpointIndex, NextExpectedCheckpoint);
		return;
	}

	Progress.LastCheckpoint = CheckpointIndex;
	Progress.CheckpointsPassedCount++;
	Progress.Score += PointsPerCheckpoint;
	Progress.DistanceToNext = ComputeDistanceToNextCheckpoint(PlayerPawn, Progress.LastCheckpoint);

	UE_LOG(LogTemp, Warning,
		TEXT("[SCORE] %s earned %d checkpoint points | TotalScore=%d | TotalCP=%d"),
		*GetNameSafe(Controller),
		PointsPerCheckpoint,
		Progress.Score,
		Progress.CheckpointsPassedCount);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Cyan,
			FString::Printf(TEXT("%s: +%d CP points | Score=%d"),
				*GetNameSafe(Controller),
				PointsPerCheckpoint,
				Progress.Score));
	}

	UpdatePositions();
}

float ARaceGameMode::ComputeDistanceToNextCheckpoint(APawn* PlayerPawn, int32 LastCheckpoint) const
{
	if (!TrackManager || !PlayerPawn) return 0.f;

	const int32 Count = TrackManager->GetCheckpointCount();
	if (Count <= 0) return 99999999999.f;

	const int32 NextIndex = (LastCheckpoint < 0)
		? 0
		: (LastCheckpoint + 1) % Count;

	ACheckpoint* NextCheckpoint = TrackManager->GetCheckpoint(NextIndex);
	if (!NextCheckpoint) return 999999999999.f;

	return FVector::Dist(PlayerPawn->GetActorLocation(), NextCheckpoint->GetActorLocation());
}

int32 ARaceGameMode::CompareControllers(AController* A, AController* B) const
{
	const FPlayerRaceProgress* ProgressA = ProgressByController.Find(A);
	const FPlayerRaceProgress* ProgressB = ProgressByController.Find(B);

	if (!ProgressA || !ProgressB) return 0;

	// Un joueur fini est toujours devant un joueur non fini
	if (ProgressA->bFinishedRace != ProgressB->bFinishedRace)
	{
		return ProgressA->bFinishedRace ? 1 : -1;
	}

	if (ProgressA->Lap != ProgressB->Lap) return (ProgressA->Lap > ProgressB->Lap) ? 1 : -1;
	if (ProgressA->LastCheckpoint != ProgressB->LastCheckpoint) return (ProgressA->LastCheckpoint > ProgressB->LastCheckpoint) ? 1 : -1;
	if (ProgressA->DistanceToNext != ProgressB->DistanceToNext) return (ProgressA->DistanceToNext < ProgressB->DistanceToNext) ? 1 : -1;

	return 0;
}

void ARaceGameMode::UpdatePositions()
{
	UE_LOG(LogTemp, Warning, TEXT("UpdatePositions called"));
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Yellow, TEXT("UpdatePositions()"));

	TArray<AActor*> PlayerControllers;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerController::StaticClass(), PlayerControllers);

	UE_LOG(LogTemp, Warning, TEXT("Player count = %d"), PlayerControllers.Num());
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow,
			FString::Printf(TEXT("Player count = %d"), PlayerControllers.Num()));
	}

	if (PlayerControllers.Num() < 2) return;

	AController* ControllerA = Cast<AController>(PlayerControllers[0]);
	AController* ControllerB = Cast<AController>(PlayerControllers[1]);

	if (!ControllerA || !ControllerB) return;

	if (APawn* P1 = ControllerA->GetPawn())
	{
		FPlayerRaceProgress& ProgA = ProgressByController.FindOrAdd(ControllerA);
		if (!ProgA.bFinishedRace)
		{
			ProgA.DistanceToNext = ComputeDistanceToNextCheckpoint(P1, ProgA.LastCheckpoint);
		}
	}

	if (APawn* P2 = ControllerB->GetPawn())
	{
		FPlayerRaceProgress& ProgB = ProgressByController.FindOrAdd(ControllerB);
		if (!ProgB.bFinishedRace)
		{
			ProgB.DistanceToNext = ComputeDistanceToNextCheckpoint(P2, ProgB.LastCheckpoint);
		}
	}

	const int32 Result = CompareControllers(ControllerA, ControllerB);

	const FPlayerRaceProgress* ProgressA = ProgressByController.Find(ControllerA);
	const FPlayerRaceProgress* ProgressB = ProgressByController.Find(ControllerB);

	if (!ProgressA || !ProgressB || !TrackManager)
	{
		return;
	}

	const int32 CPCount = TrackManager->GetCheckpointCount();
	if (CPCount <= 0) return;

	const int32 SafeCPA = FMath::Max(ProgressA->LastCheckpoint, 0);
	const int32 SafeCPB = FMath::Max(ProgressB->LastCheckpoint, 0);

	const int32 ProgA = ProgressA->Lap * CPCount + SafeCPA;
	const int32 ProgB = ProgressB->Lap * CPCount + SafeCPB;

	AController* LeaderCtrl = (ProgA >= ProgB) ? ControllerA : ControllerB;
	AController* TrailerCtrl = (ProgA >= ProgB) ? ControllerB : ControllerA;

	const FPlayerRaceProgress* LeaderP = (ProgA >= ProgB) ? ProgressA : ProgressB;
	const FPlayerRaceProgress* TrailerP = (ProgA >= ProgB) ? ProgressB : ProgressA;

	const int32 LeadByCP = FMath::Abs(ProgA - ProgB);

	UE_LOG(LogTemp, Warning, TEXT("[LEAD] %s leads %s by %d checkpoint(s) | Leader(Lap=%d CP=%d) Trailer(Lap=%d CP=%d)"),
		*GetNameSafe(LeaderCtrl), *GetNameSafe(TrailerCtrl), LeadByCP,
		LeaderP->Lap, LeaderP->LastCheckpoint,
		TrailerP->Lap, TrailerP->LastCheckpoint);

	if (GEngine)
	{
		FString LeadMessage;

		if (ProgressA->bFinishedRace && !ProgressB->bFinishedRace)
		{
			LeadMessage = TEXT("P1 a termine la course !");
		}
		else if (!ProgressA->bFinishedRace && ProgressB->bFinishedRace)
		{
			LeadMessage = TEXT("P2 a termine la course !");
		}
		else if (ProgressA->bFinishedRace && ProgressB->bFinishedRace)
		{
			LeadMessage = TEXT("P1 et P2 ont termine !");
		}
		else if (Result > 0)
		{
			LeadMessage = FString::Printf(TEXT("P1 est devant P2 ! | Ecart CP: %d"), LeadByCP);
		}
		else if (Result < 0)
		{
			LeadMessage = FString::Printf(TEXT("P2 est devant P1 ! | Ecart CP: %d"), LeadByCP);
		}
		else
		{
			LeadMessage = TEXT("P1 et P2 sont a egalite !");
		}

		GEngine->AddOnScreenDebugMessage(100, 1.0f, FColor::Cyan, LeadMessage);
	}
}

void ARaceGameMode::NotifyLapCompleted(AController* Controller, int32 NewLapNumber)
{
	if (!Controller) return;

	FPlayerRaceProgress& Progress = ProgressByController.FindOrAdd(Controller);

	if (Progress.bFinishedRace)
	{
		UE_LOG(LogTemp, Warning, TEXT("[LAP] Ignored because player already finished: %s"), *GetNameSafe(Controller));
		return;
	}

	Progress.Lap = NewLapNumber;
	Progress.LapsCompletedCount++;
	Progress.Score += PointsPerLap;

	UE_LOG(LogTemp, Warning,
		TEXT("[SCORE] %s earned %d lap points | TotalScore=%d | TotalLaps=%d"),
		*GetNameSafe(Controller),
		PointsPerLap,
		Progress.Score,
		Progress.LapsCompletedCount);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Magenta,
			FString::Printf(TEXT("%s: +%d LAP points | Score=%d"),
				*GetNameSafe(Controller),
				PointsPerLap,
				Progress.Score));
	}

	Progress.LastCheckpoint = -1;

	if (APawn* Pawn = Controller->GetPawn())
	{
		Progress.DistanceToNext = ComputeDistanceToNextCheckpoint(Pawn, Progress.LastCheckpoint);
	}

	UpdatePositions();
}

const FPlayerRaceProgress* ARaceGameMode::GetPlayerProgress(AController* Controller) const
{
	if (!Controller) return nullptr;
	return ProgressByController.Find(Controller);
}

int32 ARaceGameMode::GetPlayerScore(AController* Controller) const
{
	const FPlayerRaceProgress* Progress = GetPlayerProgress(Controller);
	return Progress ? Progress->Score : 0;
}

int32 ARaceGameMode::GetPlayerCheckpointCount(AController* Controller) const
{
	const FPlayerRaceProgress* Progress = GetPlayerProgress(Controller);
	return Progress ? Progress->CheckpointsPassedCount : 0;
}

int32 ARaceGameMode::GetPlayerLapCount(AController* Controller) const
{
	const FPlayerRaceProgress* Progress = GetPlayerProgress(Controller);
	return Progress ? Progress->LapsCompletedCount : 0;
}