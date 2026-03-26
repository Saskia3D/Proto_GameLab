// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BuffComponent.generated.h"

class UBuffBase;


UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PROTOGAMELAB_API UBuffComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UBuffComponent();

	void AddBuff(TSubclassOf<UBuffBase> BuffClass);
	void UseBuff();
	void NotifyBuffExpired(UBuffBase* ExpiredBuff);

public:
	UPROPERTY()
	UBuffBase* CurrentBuff = nullptr;

	UPROPERTY()
	TArray<UBuffBase*> ActiveBuffs;

	UFUNCTION(BlueprintPure, Category = "UI")
	UTexture2D* GetCurrentBuffIcon() const;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBuffChanged);

	UPROPERTY(BlueprintAssignable)
	FOnBuffChanged OnBuffChanged;

private:
	UBuffBase* FindActiveBuffByClass(UClass* BuffClass) const;
};
