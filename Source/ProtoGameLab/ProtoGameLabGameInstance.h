#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "UObject/SoftObjectPath.h"
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

	UFUNCTION(BlueprintCallable, Category = "Race")
	void SetLastRaceMapName(FName InMapName);

	UFUNCTION(BlueprintPure, Category = "Race")
	FName GetLastRaceMapName() const { return LastRaceMapName; }

	UFUNCTION(BlueprintPure, Category = "Leaderboard")
	TArray<FRaceLeaderboardEntry> GetLastRaceLeaderboard() const { return LastRaceLeaderboard; }

	const TArray<FRaceLeaderboardEntry>& GetLastRaceLeaderboardNative() const { return LastRaceLeaderboard; }

	UFUNCTION(BlueprintCallable, Category = "Vehicle Selection")
	void SetSelectedVehicleMeshes(const TArray<FSoftObjectPath>& InSelectedVehicleMeshes);

	UFUNCTION(BlueprintCallable, Category = "Vehicle Selection")
	void SetSelectedVehicleMesh(int32 PlayerIndex, const FSoftObjectPath& InMeshPath);

	UFUNCTION(BlueprintCallable, Category = "Vehicle Selection")
	void ClearSelectedVehicleMeshes();

	UFUNCTION(BlueprintPure, Category = "Vehicle Selection")
	FSoftObjectPath GetSelectedVehicleMesh(int32 PlayerIndex) const;

	UFUNCTION(BlueprintPure, Category = "Vehicle Selection")
	TArray<FSoftObjectPath> GetSelectedVehicleMeshes() const { return SelectedVehicleMeshes; }

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Leaderboard", meta = (AllowPrivateAccess = "true"))
	TArray<FRaceLeaderboardEntry> LastRaceLeaderboard;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Race", meta = (AllowPrivateAccess = "true"))
	FName LastRaceMapName = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle Selection", meta = (AllowPrivateAccess = "true"))
	TArray<FSoftObjectPath> SelectedVehicleMeshes;
};
