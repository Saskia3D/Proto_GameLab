// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "BuffBase.generated.h"


class AMyVehiclePawn;
/**
 * 
 */
UCLASS(Blueprintable, Abstract)
class PROTOGAMELAB_API UBuffBase : public UObject
{
	GENERATED_BODY()
	
public:
	virtual void Activate(class AMyVehiclePawn* Player);

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float Duration = 3.0f;

	AMyVehiclePawn* CachedPlayer;

	FTimerHandle DurationHandle;

	virtual void OnBuffExpired();
};
