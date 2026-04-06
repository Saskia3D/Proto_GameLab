// RaceGameMode.cpp - Impl�mentation de la classe ARaceGameMode, qui g�re la logique de la course, y compris le d�marrage de la course, la notification des joueurs qui terminent, et le suivi de l'ordre d'arriv�e. 

#include "RaceGameMode.h"
#include "Checkpoint.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "ProtoGameLabGameInstance.h"
#include "Components/PrimitiveComponent.h"
#include "TrackSplineActor.h"
#include "Components/SplineComponent.h"
#include "FinishLine.h"
#include "TimerManager.h"
#include "TrackManager.h"

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

	if (!TrackSplineActor)
	{
		TrackSplineActor = Cast<ATrackSplineActor>(
			UGameplayStatics::GetActorOfClass(GetWorld(), ATrackSplineActor::StaticClass())
		);
	}

	if (!TrackSplineActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RACE] No TrackSplineActor found in level"));
	}

	RefreshAllPlayerProgress();

	if (!TrackManager && TrackManagerClass)
	{
		TrackManager = GetWorld()->SpawnActor<ATrackManager>(TrackManagerClass);
	}

	UE_LOG(LogTemp, Warning, TEXT("RaceGameMode TrackManager = %s (CPCount=%d)"),
		*GetNameSafe(TrackManager),
		TrackManager ? TrackManager->GetCheckpointCount() : -1);

	if (bAutoStartRaceOnBeginPlay)
	{
		StartRace();
		UE_LOG(LogTemp, Warning, TEXT("RaceGameMode BeginPlay (ACTIVE)"));
	}
	else
	{
		RaceState = ERaceState::Waiting;
		UE_LOG(LogTemp, Warning, TEXT("RaceGameMode BeginPlay (WAITING FOR TUTORIAL)"));
	}
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

	if (GetWorld())
	{
		if (UProtoGameLabGameInstance* GameInstance = Cast<UProtoGameLabGameInstance>(GetWorld()->GetGameInstance()))
		{
			GameInstance->ClearLastRaceLeaderboard();
			GameInstance->SetLastRaceMapName(FName(*UGameplayStatics::GetCurrentLevelName(GetWorld(), true)));
		}
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

int32 ARaceGameMode::FindFinishOrderIndex(AController* Controller) const
{
	if (!Controller)
	{
		return INDEX_NONE;
	}

	for (int32 Index = 0; Index < FinishOrder.Num(); ++Index)
	{
		if (FinishOrder[Index].Controller == Controller)
		{
			return Index;
		}
	}

	return INDEX_NONE;
}

float ARaceGameMode::GetFinishTimeForController(AController* Controller) const
{
	if (!Controller)
	{
		return -1.f;
	}

	for (const FRaceFinishEntry& Entry : FinishOrder)
	{
		if (Entry.Controller == Controller)
		{
			return Entry.FinishTime;
		}
	}

	return -1.f;
}

void ARaceGameMode::GatherRaceControllers(TArray<AController*>& OutControllers) const
{
	OutControllers.Reset();

	TSet<TObjectPtr<AController>> UniqueControllers;

	for (const TPair<TObjectPtr<AController>, FPlayerRaceProgress>& Pair : ProgressByController)
	{
		if (Pair.Key)
		{
			UniqueControllers.Add(Pair.Key);
		}
	}

	for (const FRaceFinishEntry& Entry : FinishOrder)
	{
		if (Entry.Controller)
		{
			UniqueControllers.Add(Entry.Controller);
		}
	}

	if (GetWorld())
	{
		TArray<AActor*> PawnActors;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), APawn::StaticClass(), PawnActors);

		for (AActor* Actor : PawnActors)
		{
			if (APawn* Pawn = Cast<APawn>(Actor))
			{
				if (AController* Controller = Pawn->GetController())
				{
					UniqueControllers.Add(Controller);
				}
			}
		}
	}

	OutControllers.Reserve(UniqueControllers.Num());
	for (AController* Controller : UniqueControllers)
	{
		if (Controller)
		{
			OutControllers.Add(Controller);
		}
	}
}

TArray<FRaceLeaderboardEntry> ARaceGameMode::BuildLeaderboardSnapshot() const
{
	TArray<AController*> Controllers;
	GatherRaceControllers(Controllers);

	Controllers.Sort([this](const AController& Left, const AController& Right)
	{
		if (&Left == &Right)
		{
			return false;
		}

		const int32 LeftFinishIndex = FindFinishOrderIndex(const_cast<AController*>(&Left));
		const int32 RightFinishIndex = FindFinishOrderIndex(const_cast<AController*>(&Right));

		const bool bLeftFinished = LeftFinishIndex != INDEX_NONE;
		const bool bRightFinished = RightFinishIndex != INDEX_NONE;

		if (bLeftFinished != bRightFinished)
		{
			return bLeftFinished;
		}

		if (bLeftFinished && bRightFinished)
		{
			return LeftFinishIndex < RightFinishIndex;
		}

		const int32 CompareResult = CompareControllers(const_cast<AController*>(&Left), const_cast<AController*>(&Right));
		if (CompareResult != 0)
		{
			return CompareResult > 0;
		}

		return GetNameSafe(&Left) < GetNameSafe(&Right);
	});

	TMap<TObjectPtr<AController>, FString> DisplayNames;
	int32 HumanFallbackIndex = 1;
	int32 AIIndex = 1;

	for (AController* Controller : Controllers)
	{
		if (APlayerController* PC = Cast<APlayerController>(Controller))
		{
			int32 PlayerNumber = INDEX_NONE;

			if (GetWorld())
			{
				for (int32 LocalIndex = 0; LocalIndex < 8; ++LocalIndex)
				{
					if (UGameplayStatics::GetPlayerController(GetWorld(), LocalIndex) == PC)
					{
						PlayerNumber = LocalIndex + 1;
						break;
					}
				}
			}

			if (PlayerNumber == INDEX_NONE)
			{
				PlayerNumber = HumanFallbackIndex;
			}

			DisplayNames.Add(Controller, FString::Printf(TEXT("Player %d"), PlayerNumber));
			++HumanFallbackIndex;
		}
	}

	for (AController* Controller : Controllers)
	{
		if (!DisplayNames.Contains(Controller))
		{
			DisplayNames.Add(Controller, FString::Printf(TEXT("AI %d"), AIIndex++));
		}
	}

	TArray<FRaceLeaderboardEntry> Entries;
	Entries.Reserve(Controllers.Num());

	for (int32 Index = 0; Index < Controllers.Num(); ++Index)
	{
		AController* Controller = Controllers[Index];
		const FPlayerRaceProgress* Progress = GetPlayerProgress(Controller);
		const int32 FinishIndex = FindFinishOrderIndex(Controller);

		FRaceLeaderboardEntry Entry;
		Entry.Position = Index + 1;
		Entry.PlayerName = DisplayNames.FindRef(Controller);
		Entry.FinishTimeSeconds = GetFinishTimeForController(Controller);
		Entry.Score = GetPlayerScore(Controller);
		Entry.LapsCompleted = GetPlayerLapCount(Controller);
		Entry.CheckpointsPassed = GetPlayerCheckpointCount(Controller);
		Entry.CurrentLap = Progress ? Progress->Lap : 0;
		Entry.LastCheckpointIndex = Progress ? Progress->LastCheckpoint : -1;
		Entry.bFinishedRace = FinishIndex != INDEX_NONE;

		Entries.Add(Entry);
	}

	return Entries;
}

void ARaceGameMode::CacheLeaderboardForEndMenu()
{
	if (GetWorld())
	{
		if (UProtoGameLabGameInstance* GameInstance = Cast<UProtoGameLabGameInstance>(GetWorld()->GetGameInstance()))
		{
			const TArray<FRaceLeaderboardEntry> Snapshot = BuildLeaderboardSnapshot();
			GameInstance->SaveLastRaceLeaderboard(Snapshot);

			UE_LOG(LogTemp, Warning, TEXT("[LEADERBOARD] Stored %d entries for RaceEndMenu"), Snapshot.Num());
		}
	}
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

	// On d�marre le compte � rebours seulement quand le premier joueur finit
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

	// Si tous les joueurs attendus ont fini, fin imm�diate
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
	CacheLeaderboardForEndMenu();

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
	if (RaceState != ERaceState::Running)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CHECKPOINT] Ignored because race is not running yet"));
		return;
	}

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

	if (APawn* Pawn = Controller->GetPawn())
	{
		Progress.DistanceToNext = ComputeDistanceToNextCheckpoint(Pawn, Progress.LastCheckpoint);
		Progress.SplineDistance = ComputeSplineDistance(Pawn);
		Progress.SplineAlpha = ComputeSplineAlpha(Pawn);
	}

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

	if (!ProgressA || !ProgressB)
	{
		return 0;
	}

	// 1) joueur fini est toujours devant
	if (ProgressA->bFinishedRace != ProgressB->bFinishedRace)
	{
		return ProgressA->bFinishedRace ? 1 : -1;
	}

	// 2) le lap reste le crit�re principal
	if (ProgressA->Lap != ProgressB->Lap)
	{
		return (ProgressA->Lap > ProgressB->Lap) ? 1 : -1;
	}

	// 3) le dernier checkpoint valid�
	if (ProgressA->LastCheckpoint != ProgressB->LastCheckpoint)
	{
		return (ProgressA->LastCheckpoint > ProgressB->LastCheckpoint) ? 1 : -1;
	}

	// 4) la progression spline locale sur le segment courant
	if (!FMath::IsNearlyEqual(ProgressA->SplineDistance, ProgressB->SplineDistance, 1.0f))
	{
		return (ProgressA->SplineDistance > ProgressB->SplineDistance) ? 1 : -1;
	}

	// 5) Fallback
	if (!FMath::IsNearlyEqual(ProgressA->DistanceToNext, ProgressB->DistanceToNext, 1.0f))
	{
		return (ProgressA->DistanceToNext < ProgressB->DistanceToNext) ? 1 : -1;
	}

	return 0;
}

void ARaceGameMode::UpdatePositions()
{
	RefreshAllPlayerProgress();

	const TArray<AController*> Controllers = GetRaceControllers();
	if (Controllers.Num() < 2)
	{
		return;
	}

	AController* ControllerA = Controllers[0];
	AController* ControllerB = Controllers[1];

	if (!ControllerA || !ControllerB)
	{
		return;
	}

	const FPlayerRaceProgress* ProgressA = ProgressByController.Find(ControllerA);
	const FPlayerRaceProgress* ProgressB = ProgressByController.Find(ControllerB);

	if (!ProgressA || !ProgressB || !TrackManager)
	{
		return;
	}

	const int32 Result = CompareControllers(ControllerA, ControllerB);

	const int32 CPCount = TrackManager->GetCheckpointCount();
	if (CPCount <= 0)
	{
		return;
	}

	const int32 SafeCPA = FMath::Max(ProgressA->LastCheckpoint, 0);
	const int32 SafeCPB = FMath::Max(ProgressB->LastCheckpoint, 0);

	const int32 ProgA = ProgressA->Lap * CPCount + SafeCPA;
	const int32 ProgB = ProgressB->Lap * CPCount + SafeCPB;

	AController* LeaderCtrl = (Result >= 0) ? ControllerA : ControllerB;
	AController* TrailerCtrl = (Result >= 0) ? ControllerB : ControllerA;

	const FPlayerRaceProgress* LeaderP = (Result >= 0) ? ProgressA : ProgressB;
	const FPlayerRaceProgress* TrailerP = (Result >= 0) ? ProgressB : ProgressA;

	const int32 LeadByCP = FMath::Abs(ProgA - ProgB);

	UE_LOG(LogTemp, Warning,
		TEXT("[LEAD] %s leads %s | CPGap=%d | Leader(Lap=%d CP=%d Spline=%.1f) Trailer(Lap=%d CP=%d Spline=%.1f)"),
		*GetNameSafe(LeaderCtrl),
		*GetNameSafe(TrailerCtrl),
		LeadByCP,
		LeaderP->Lap,
		LeaderP->LastCheckpoint,
		LeaderP->SplineDistance,
		TrailerP->Lap,
		TrailerP->LastCheckpoint,
		TrailerP->SplineDistance
	);

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
			LeadMessage = FString::Printf(
				TEXT("P1 est devant P2 ! | Ecart CP: %d | Spline %.0f vs %.0f"),
				LeadByCP,
				ProgressA->SplineDistance,
				ProgressB->SplineDistance
			);
		}
		else if (Result < 0)
		{
			LeadMessage = FString::Printf(
				TEXT("P2 est devant P1 ! | Ecart CP: %d | Spline %.0f vs %.0f"),
				LeadByCP,
				ProgressB->SplineDistance,
				ProgressA->SplineDistance
			);
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
	if (RaceState != ERaceState::Running)
	{
		UE_LOG(LogTemp, Warning, TEXT("[LAP] Ignored because race is not running yet"));
		return;
	}

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
		Progress.SplineDistance = ComputeSplineDistance(Pawn);
		Progress.SplineAlpha = ComputeSplineAlpha(Pawn);
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

TArray<AController*> ARaceGameMode::GetRaceControllers() const
{
	TArray<AController*> Result;

	if (!GetWorld())
	{
		return Result;
	}

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (PC)
		{
			Result.Add(PC);
		}
	}

	return Result;
}

float ARaceGameMode::ComputeSplineDistance(APawn* Pawn) const
{
	if (!Pawn || !TrackSplineActor)
	{
		return 0.f;
	}

	return TrackSplineActor->GetClosestDistanceAlongSpline(Pawn->GetActorLocation());
}

float ARaceGameMode::ComputeSplineAlpha(APawn* Pawn) const
{
	if (!Pawn || !TrackSplineActor || !TrackSplineActor->Spline)
	{
		return 0.f;
	}

	const float SplineLength = TrackSplineActor->Spline->GetSplineLength();
	if (SplineLength <= KINDA_SMALL_NUMBER)
	{
		return 0.f;
	}

	const float Distance = ComputeSplineDistance(Pawn);
	return FMath::Clamp(Distance / SplineLength, 0.f, 1.f);
}

void ARaceGameMode::RefreshControllerProgress(AController* Controller)
{
	if (!Controller)
	{
		return;
	}

	FPlayerRaceProgress& Progress = ProgressByController.FindOrAdd(Controller);

	if (Progress.bFinishedRace)
	{
		return;
	}

	APawn* Pawn = Controller->GetPawn();
	if (!Pawn)
	{
		return;
	}

	// debug helper
	Progress.DistanceToNext = ComputeDistanceToNextCheckpoint(Pawn, Progress.LastCheckpoint);

	// progression
	Progress.SplineDistance = ComputeSplineDistance(Pawn);
	Progress.SplineAlpha = ComputeSplineAlpha(Pawn);
}

void ARaceGameMode::RefreshAllPlayerProgress()
{
	const TArray<AController*> Controllers = GetRaceControllers();

	for (AController* Controller : Controllers)
	{
		RefreshControllerProgress(Controller);
	}
}

int32 ARaceGameMode::GetPlayerRacePosition(AController* Controller) const
{
	if (!Controller)
	{
		return 0;
	}

	const TArray<AController*> Controllers = GetRaceControllers();
	if (Controllers.Num() == 0)
	{
		return 0;
	}

	int32 Position = 1;

	for (AController* Other : Controllers)
	{
		if (!Other || Other == Controller)
		{
			continue;
		}

		if (CompareControllers(Other, Controller) > 0)
		{
			Position++;
		}
	}

	return Position;
}

AController* ARaceGameMode::GetCurrentLeader() const
{
	const TArray<AController*> Controllers = GetRaceControllers();
	if (Controllers.Num() == 0)
	{
		return nullptr;
	}

	AController* Best = Controllers[0];

	for (int32 i = 1; i < Controllers.Num(); ++i)
	{
		AController* Candidate = Controllers[i];
		if (Candidate && CompareControllers(Candidate, Best) > 0)
		{
			Best = Candidate;
		}
	}

	return Best;
}

bool ARaceGameMode::IsControllerAheadOf(AController* A, AController* B) const
{
	if (!A || !B)
	{
		return false;
	}

	return CompareControllers(A, B) > 0;
}

int32 ARaceGameMode::GetRaceTotalLaps() const
{
	TArray<AActor*> FinishLines;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AFinishLine::StaticClass(), FinishLines);

	if (FinishLines.Num() > 0)
	{
		const AFinishLine* FinishLine = Cast<AFinishLine>(FinishLines[0]);
		if (FinishLine)
		{
			return FinishLine->TotalLaps;
		}
	}

	return 3;
}

bool ARaceGameMode::HasControllerFinishedRace(AController* Controller) const
{
	if (!Controller)
	{
		return false;
	}

	const FPlayerRaceProgress* Progress = ProgressByController.Find(Controller);
	if (!Progress)
	{
		return false;
	}

	return Progress->bFinishedRace;
}

int32 ARaceGameMode::GetDisplayedLapForController(AController* Controller) const
{
	if (!Controller)
	{
		return 1;
	}

	const FPlayerRaceProgress* Progress = ProgressByController.Find(Controller);
	if (!Progress)
	{
		return 1;
	}

	const int32 TotalLaps = GetRaceTotalLaps();

	if (Progress->bFinishedRace)
	{
		return TotalLaps;
	}

	return FMath::Clamp(Progress->Lap + 1, 1, TotalLaps);
}

bool ARaceGameMode::IsFinishCountdownActive() const
{
	if (!bUseFinishCountdown || !bFinishCountdownStarted || RaceState != ERaceState::Running || bHasTriggeredEndMenu || !GetWorld())
	{
		return false;
	}

	return GetWorld()->GetTimerManager().IsTimerActive(FinishCountdownHandle);
}

float ARaceGameMode::GetFinishCountdownRemaining() const
{
	if (!IsFinishCountdownActive() || !GetWorld())
	{
		return 0.f;
	}

	return GetWorld()->GetTimerManager().GetTimerRemaining(FinishCountdownHandle);
}

int32 ARaceGameMode::GetFinishCountdownRemainingSeconds() const
{
	return FMath::CeilToInt(GetFinishCountdownRemaining());
}

