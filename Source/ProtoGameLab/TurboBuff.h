// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BuffBase.h"
#include "TurboBuff.generated.h"

/**
 * 
 */
UCLASS()
class PROTOGAMELAB_API UTurboBuff : public UBuffBase
{
	GENERATED_BODY()
	
public:
	virtual void Activate(AMyVehiclePawn* Player) override;

protected:
	virtual void OnBuffExpired() override;

private:
	float OriginalTorqueMultiplier;
};
