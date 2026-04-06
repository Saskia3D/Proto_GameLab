// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BuffBase.h"
#include "NiagaraComponent.h"
#include "TurboBuff.generated.h"

/**
 * 
 */
UCLASS()
class PROTOGAMELAB_API UTurboBuff : public UBuffBase
{
	GENERATED_BODY()
	
public:
	virtual void Activate(APawn* Player) override;

protected:
	virtual void OnBuffExpired() override;

private:
	float OriginalTorqueMultiplier;

	UPROPERTY(EditDefaultsOnly, Category="Turbo")
	float TurboMultiplier = 1.5f;

	float OriginalMaxSpeed = 0.f;
	TWeakObjectPtr<class ASTR_RacerPawn> CachedRacer;

	UPROPERTY()
	UNiagaraComponent* ActiveBoostFX_Left;

	UPROPERTY()
	UNiagaraComponent* ActiveBoostFX_Right;
};
