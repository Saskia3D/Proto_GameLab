#include "MainMenuWidget.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"

bool UMainMenuWidget::Initialize()
{
	if (!Super::Initialize()) return false;

	if (PlayButton)
	{
		//liaison du clic sur le bouton à la fonction 
		PlayButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnPlayClicked);
	}
	return true;
}

void UMainMenuWidget::OnPlayClicked()
{
	UGameplayStatics::OpenLevel(this, FName("Lvl_Test_2Players"));
}