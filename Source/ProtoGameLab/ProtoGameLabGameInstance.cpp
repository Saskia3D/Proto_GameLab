#include "ProtoGameLabGameInstance.h"

namespace
{
	void EnsureSelectionSize(TArray<FSoftObjectPath>& MeshPaths, const int32 DesiredSize)
	{
		while (MeshPaths.Num() < DesiredSize)
		{
			MeshPaths.Add(FSoftObjectPath());
		}
	}
}

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

void UProtoGameLabGameInstance::SetSelectedVehicleMeshes(const TArray<FSoftObjectPath>& InSelectedVehicleMeshes)
{
	SelectedVehicleMeshes = InSelectedVehicleMeshes;
}

void UProtoGameLabGameInstance::SetSelectedVehicleMesh(const int32 PlayerIndex, const FSoftObjectPath& InMeshPath)
{
	if (PlayerIndex < 0)
	{
		return;
	}

	EnsureSelectionSize(SelectedVehicleMeshes, PlayerIndex + 1);
	SelectedVehicleMeshes[PlayerIndex] = InMeshPath;
}

void UProtoGameLabGameInstance::ClearSelectedVehicleMeshes()
{
	SelectedVehicleMeshes.Reset();
}

FSoftObjectPath UProtoGameLabGameInstance::GetSelectedVehicleMesh(const int32 PlayerIndex) const
{
	if (!SelectedVehicleMeshes.IsValidIndex(PlayerIndex))
	{
		return FSoftObjectPath();
	}

	return SelectedVehicleMeshes[PlayerIndex];
}
