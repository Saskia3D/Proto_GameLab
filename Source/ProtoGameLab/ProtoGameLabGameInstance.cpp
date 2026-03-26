#include "ProtoGameLabGameInstance.h"

void UProtoGameLabGameInstance::SaveLastRaceLeaderboard(const TArray<FRaceLeaderboardEntry>& InEntries)
{
	LastRaceLeaderboard = InEntries;
}

void UProtoGameLabGameInstance::ClearLastRaceLeaderboard()
{
	LastRaceLeaderboard.Reset();
}

void UProtoGameLabGameInstance::SetLastRaceMapName(const FName InMapName)
{
	LastRaceMapName = InMapName;
}
