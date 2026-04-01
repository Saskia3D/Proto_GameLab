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


	UPROPERTY(meta = (BindWidget), BlueprintReadOnly)
	class UButton* PlayButton;

private:
	UFUNCTION()
	void OnPlayClicked();
};