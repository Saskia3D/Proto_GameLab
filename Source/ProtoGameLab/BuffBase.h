//BuffBase.h

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "BuffBase.generated.h"

class APawn;

UCLASS(Blueprintable, Abstract)
class PROTOGAMELAB_API UBuffBase : public UObject
{
	GENERATED_BODY()

public:
	virtual void Activate(APawn* Player);

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float Duration = 3.0f;

	UPROPERTY()
	TObjectPtr<APawn> CachedPlayer = nullptr;

	FTimerHandle DurationHandle;

	virtual void OnBuffExpired();
};