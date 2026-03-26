#include "ProtoGameLabGameInstance.h"

void UProtoGameLabGameInstance::SaveLastRaceLeaderboard(const TArray<FRaceLeaderboardEntry>& InEntries)
{
	LastRaceLeaderboard = InEntries;
}

void UProtoGameLabGameInstance::ClearLastRaceLeaderboard()
{
	LastRaceLeaderboard.Reset();
}
