#include "MainMenuWidget.h"
#include "../VehicleSelectionWidget.h"
#include "../ProtoGameLabGameInstance.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/WidgetTree.h"

bool UMainMenuWidget::Initialize()
{
	if (!Super::Initialize()) return false;
	return true;
}

void UMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!PlayButton && WidgetTree)
	{
		PlayButton = Cast<UButton>(WidgetTree->FindWidget(TEXT("PlayButton")));
	}

	if (PlayButton)
	{
		// WBP_MainMenu2 ajoute encore son propre binding Blueprint sur Play.
		// On le remplace ici, après la construction du widget, pour garder
		// uniquement la nouvelle navigation vers la sélection d'auto.
		PlayButton->OnClicked.Clear();
		PlayButton->OnPressed.Clear();
		PlayButton->OnReleased.Clear();
		PlayButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnPlayClicked);
		UE_LOG(LogTemp, Warning, TEXT("[MAIN MENU] PlayButton rebound to native selection flow"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[MAIN MENU] PlayButton not found in widget tree"));
	}
}

void UMainMenuWidget::OnPlayClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("[MAIN MENU] OnPlayClicked called"));

	if (!GetWorld())
	{
		UE_LOG(LogTemp, Error, TEXT("[MAIN MENU] OnPlayClicked aborted: no world"));
		return;
	}

	if (UProtoGameLabGameInstance* GameInstance = Cast<UProtoGameLabGameInstance>(GetWorld()->GetGameInstance()))
	{
		GameInstance->ClearSelectedVehicleMeshes();
	}

	if (!VehicleSelectionWidget)
	{
		for (int32 PlayerIndex = GetWorld()->GetGameInstance()->GetNumLocalPlayers(); PlayerIndex < 2; ++PlayerIndex)
		{
			UGameplayStatics::CreatePlayer(this, PlayerIndex, true);
		}

		TSubclassOf<UVehicleSelectionWidget> WidgetClass = VehicleSelectionWidgetClass;
		if (!WidgetClass)
		{
			WidgetClass = UVehicleSelectionWidget::StaticClass();
		}

		if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
		{
			VehicleSelectionWidget = CreateWidget<UVehicleSelectionWidget>(PC, WidgetClass);
			if (VehicleSelectionWidget)
			{
				UE_LOG(LogTemp, Warning, TEXT("[MAIN MENU] Vehicle selection widget created: %s"), *GetNameSafe(VehicleSelectionWidget->GetClass()));
				VehicleSelectionWidget->SetTargetLevelName(VehicleSelectionTargetLevel);
				UWidgetLayoutLibrary::RemoveAllWidgets(this);
				VehicleSelectionWidget->AddToViewport(100);
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("[MAIN MENU] Failed to create vehicle selection widget"));
			}
		}
	}
}
