// Fill out your copyright notice in the Description page of Project Settings.



#include "RaceEndWidget.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"

void URaceEndWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// On relie l'action "Cliquer" de nos boutons à nos fonctions C++
	if (Btn_Restart)
	{
		Btn_Restart->OnClicked.AddDynamic(this, &URaceEndWidget::OnRestartClicked);
	}

	if (Btn_MainMenu)
	{
		Btn_MainMenu->OnClicked.AddDynamic(this, &URaceEndWidget::OnMainMenuClicked);
	}
}

void URaceEndWidget::OnRestartClicked()
{
	// On relance la carte de la course à 2 joueurs
	UGameplayStatics::OpenLevel(this, FName("Lvl_Test_2Players"));
}

void URaceEndWidget::OnMainMenuClicked()
{
	// On retourne au menu principal
	UGameplayStatics::OpenLevel(this, FName("MainMenu"));
}