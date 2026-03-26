#pragma once

#include "CoreMinimal.h"
#include "RaceLeaderboardEntry.generated.h"

USTRUCT(BlueprintType)
struct FRaceLeaderboardEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Leaderboard")
	int32 Position = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Leaderboard")
	FString PlayerName;

	UPROPERTY(BlueprintReadOnly, Category = "Leaderboard")
	float FinishTimeSeconds = -1.f;

	UPROPERTY(BlueprintReadOnly, Category = "Leaderboard")
	int32 Score = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Leaderboard")
	int32 LapsCompleted = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Leaderboard")
	int32 CheckpointsPassed = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Leaderboard")
	int32 CurrentLap = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Leaderboard")
	int32 LastCheckpointIndex = -1;

	UPROPERTY(BlueprintReadOnly, Category = "Leaderboard")
	bool bFinishedRace = false;
};
