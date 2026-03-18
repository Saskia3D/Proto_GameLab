// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RaceEndWidget.generated.h"

class UButton;

UCLASS()
class PROTOGAMELAB_API URaceEndWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	UButton* Btn_Restart;

	UPROPERTY(meta = (BindWidget))
	UButton* Btn_MainMenu;

	UFUNCTION()
	void OnRestartClicked();

	UFUNCTION()
	void OnMainMenuClicked();
};