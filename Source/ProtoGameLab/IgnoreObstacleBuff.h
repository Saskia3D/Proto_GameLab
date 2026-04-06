#pragma once

#include "CoreMinimal.h"
#include "BuffBase.h"
#include "IgnoreObstacleBuff.generated.h"

class ASTR_RacerPawn;

UCLASS()
class PROTOGAMELAB_API UIgnoreObstacleBuff : public UBuffBase
{
	GENERATED_BODY()

public:
	virtual void Activate(APawn* Player) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Audio")
	void OnIgnoreObsatcleActivated();

	UFUNCTION(BlueprintImplementableEvent, Category = "Audio")
	void OnIgnoreObstacleExpired();

protected:
	virtual void OnBuffExpired() override;

private:
	TWeakObjectPtr<ASTR_RacerPawn> CachedRacer;
	TArray<TWeakObjectPtr<AActor>> IgnoredActors;
	FCollisionResponseContainer PreviousResponses;
	bool bPreviousNotifyRigidBodyCollision = true;
	bool bHasSavedCollisionState = false;
};
