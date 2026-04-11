#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainMenuWidget.generated.h"

UCLASS(Blueprintable, BlueprintType)
class PROTOGAMELAB_API UMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	// Appel de la création du widget
	virtual bool Initialize() override;
	virtual void NativeConstruct() override;


	UPROPERTY(meta = (BindWidget), BlueprintReadOnly)
	class UButton* PlayButton;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Navigation")
	TSubclassOf<class UVehicleSelectionWidget> VehicleSelectionWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Navigation")
	FName VehicleSelectionTargetLevel = FName(TEXT("Lvl_Test_2Players"));

	UPROPERTY(Transient)
	TObjectPtr<class UVehicleSelectionWidget> VehicleSelectionWidget = nullptr;

private:
	UFUNCTION()
	void OnPlayClicked();
};
