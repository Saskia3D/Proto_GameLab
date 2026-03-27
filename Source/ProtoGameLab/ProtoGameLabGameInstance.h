#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "RaceLeaderboardEntry.h"
#include "ProtoGameLabGameInstance.generated.h"

UCLASS()
class PROTOGAMELAB_API UProtoGameLabGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Leaderboard")
	void SaveLastRaceLeaderboard(const TArray<FRaceLeaderboardEntry>& InEntries);

	UFUNCTION(BlueprintCallable, Category = "Leaderboard")
	void ClearLastRaceLeaderboard();

	UFUNCTION(BlueprintPure, Category = "Leaderboard")
	TArray<FRaceLeaderboardEntry> GetLastRaceLeaderboard() const { return LastRaceLeaderboard; }

	const TArray<FRaceLeaderboardEntry>& GetLastRaceLeaderboardNative() const { return LastRaceLeaderboard; }

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Leaderboard", meta = (AllowPrivateAccess = "true"))
	TArray<FRaceLeaderboardEntry> LastRaceLeaderboard;
};
