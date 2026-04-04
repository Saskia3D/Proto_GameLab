#include "VehicleSelectionWidget.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Viewport.h"
#include "Engine/StaticMesh.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerInput.h"
#include "Input/Events.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "ProtoGameLabGameInstance.h"
#include "STR_RacerPawn.h"
#include "Styling/CoreStyle.h"
#include "VehicleSelectionPreviewActor.h"
#include "Widgets/SWidget.h"

namespace
{
	const FLinearColor PlayerOneAccent(1.0f, 0.70f, 0.16f, 1.0f);
	const FLinearColor PlayerTwoAccent(0.30f, 0.88f, 1.0f, 1.0f);
	const FLinearColor DefaultTextColor = FLinearColor::White;
	const FLinearColor MutedTextColor(0.68f, 0.73f, 0.81f, 1.0f);
	const FLinearColor ReadyTextColor(0.47f, 0.96f, 0.61f, 1.0f);
	const FLinearColor ButtonColor(0.12f, 0.17f, 0.25f, 1.0f);
	const FLinearColor ButtonHoverColor(0.18f, 0.24f, 0.34f, 1.0f);
	const FLinearColor ButtonPressedColor(0.26f, 0.33f, 0.46f, 1.0f);

	FLinearColor GetAccentColor(const int32 PlayerIndex)
	{
		return PlayerIndex == 0 ? PlayerOneAccent : PlayerTwoAccent;
	}

	void StyleTextBlock(UTextBlock* TextBlock, const FSlateFontInfo& FontInfo, const FLinearColor& Color, const ETextJustify::Type Justification)
	{
		if (!TextBlock)
		{
			return;
		}

		TextBlock->SetFont(FontInfo);
		TextBlock->SetColorAndOpacity(FSlateColor(Color));
		TextBlock->SetJustification(Justification);
		TextBlock->SetShadowOffset(FVector2D(1.0f, 1.0f));
		TextBlock->SetShadowColorAndOpacity(FLinearColor::Black);
	}

	UTextBlock* CreateTextBlock(UWidgetTree* WidgetTree, const FString& Name, const FText& Value, const FSlateFontInfo& FontInfo, const FLinearColor& Color, const ETextJustify::Type Justification)
	{
		UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *Name);
		TextBlock->SetText(Value);
		StyleTextBlock(TextBlock, FontInfo, Color, Justification);
		return TextBlock;
	}

	UButton* CreateStyledButton(UWidgetTree* WidgetTree, const FString& Name, const FText& Label, const FSlateFontInfo& FontInfo, UTextBlock*& OutLabel)
	{
		UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), *Name);
		Button->SetBackgroundColor(FLinearColor::White);
		Button->SetClickMethod(EButtonClickMethod::MouseDown);

		FButtonStyle ButtonStyle = Button->GetStyle();
		ButtonStyle.Normal.TintColor = FSlateColor(ButtonColor);
		ButtonStyle.Hovered.TintColor = FSlateColor(ButtonHoverColor);
		ButtonStyle.Pressed.TintColor = FSlateColor(ButtonPressedColor);
		ButtonStyle.Disabled.TintColor = FSlateColor(FLinearColor(0.18f, 0.18f, 0.18f, 0.8f));
		Button->SetStyle(ButtonStyle);

		OutLabel = CreateTextBlock(WidgetTree, Name + TEXT("_Label"), Label, FontInfo, DefaultTextColor, ETextJustify::Center);
		Button->AddChild(OutLabel);
		return Button;
	}

	FString GetVehicleDisplayName(const FVehicleSelectionOption& Option, const int32 Index)
	{
		if (!Option.DisplayName.IsEmpty())
		{
			return Option.DisplayName.ToString().ToUpper();
		}

		return FString::Printf(TEXT("CAR %d"), Index + 1);
	}
}

void UVehicleSelectionWidget::SetTargetLevelName(const FName InTargetLevelName)
{
	TargetLevelName = InTargetLevelName;
}

TSharedRef<SWidget> UVehicleSelectionWidget::RebuildWidget()
{
	InitializeDefaultVehicleOptions();
	CacheThemeFont();
	BuildRuntimeWidget();
	UE_LOG(LogTemp, Warning, TEXT("[VEHICLE SELECT] RebuildWidget built runtime tree with %d options"), VehicleOptions.Num());

	return Super::RebuildWidget();
}

void UVehicleSelectionWidget::NativeConstruct()
{
	Super::NativeConstruct();
	UE_LOG(LogTemp, Warning, TEXT("[VEHICLE SELECT] NativeConstruct"));

	SetIsFocusable(true);
	EnsureTwoLocalPlayers();
	bPendingInitialPreviewSetup = true;
	UE_LOG(LogTemp, Warning, TEXT("[VEHICLE SELECT] Runtime widget ready with %d options"), VehicleOptions.Num());

	ResetAcceptInputGate(0.25f);
	ConfigureSelectionInput();
	SetKeyboardFocus();
	FocusAllUsers();
}

void UVehicleSelectionWidget::NativeDestruct()
{
	for (int32 PlayerIndex = 0; PlayerIndex < PlayerStates.Num(); ++PlayerIndex)
	{
		DestroyPreviewActor(PlayerIndex);
	}

	Super::NativeDestruct();
}

void UVehicleSelectionWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	UpdateAcceptInputGate(InDeltaTime);

	if (bPendingInitialPreviewSetup)
	{
		FinalizeInitialPreviewSetup();
	}
}

bool UVehicleSelectionWidget::NativeSupportsKeyboardFocus() const
{
	return true;
}

FReply UVehicleSelectionWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	const int32 PlayerIndex = ResolvePlayerIndexFromKey(InKeyEvent);
	if (!PlayerStates.IsValidIndex(PlayerIndex))
	{
		return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
	}

	if (InKeyEvent.IsRepeat())
	{
		return FReply::Handled();
	}

	if (IsConfirmKeyForPlayer(PlayerIndex, Key) && !bAcceptInputArmed)
	{
		bAcceptPressedDuringGate = true;
		return FReply::Handled();
	}

	if (IsLeftKeyForPlayer(PlayerIndex, Key))
	{
		HandleSelectionChange(PlayerIndex, -1);
		return FReply::Handled();
	}

	if (IsRightKeyForPlayer(PlayerIndex, Key))
	{
		HandleSelectionChange(PlayerIndex, +1);
		return FReply::Handled();
	}

	if (IsConfirmKeyForPlayer(PlayerIndex, Key))
	{
		HandleConfirm(PlayerIndex);
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply UVehicleSelectionWidget::NativeOnKeyUp(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const int32 PlayerIndex = ResolvePlayerIndexFromKey(InKeyEvent);
	if (PlayerStates.IsValidIndex(PlayerIndex) && IsConfirmKeyForPlayer(PlayerIndex, InKeyEvent.GetKey()) && !bAcceptInputArmed)
	{
		bAcceptPressedDuringGate = false;
		if (AcceptInputArmDelayRemaining <= 0.f)
		{
			bAcceptInputArmed = true;
		}

		return FReply::Handled();
	}

	return Super::NativeOnKeyUp(InGeometry, InKeyEvent);
}

void UVehicleSelectionWidget::OnPlayerOneLeftClicked()
{
	HandleSelectionChange(0, -1);
}

void UVehicleSelectionWidget::OnPlayerOneRightClicked()
{
	HandleSelectionChange(0, +1);
}

void UVehicleSelectionWidget::OnPlayerOneConfirmClicked()
{
	HandleConfirm(0);
}

void UVehicleSelectionWidget::OnPlayerTwoLeftClicked()
{
	HandleSelectionChange(1, -1);
}

void UVehicleSelectionWidget::OnPlayerTwoRightClicked()
{
	HandleSelectionChange(1, +1);
}

void UVehicleSelectionWidget::OnPlayerTwoConfirmClicked()
{
	HandleConfirm(1);
}

void UVehicleSelectionWidget::InitializeDefaultVehicleOptions()
{
	if (!VehicleOptions.IsEmpty())
	{
		return;
	}

	auto AddDefaultVehicle = [this](const TCHAR* Name, const TCHAR* AssetPath)
	{
		FVehicleSelectionOption& NewOption = VehicleOptions.AddDefaulted_GetRef();
		NewOption.DisplayName = FText::FromString(Name);
		NewOption.VehicleMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(AssetPath));
		NewOption.PreviewScale = FVector(1.0f, 1.0f, 1.0f);
	};

	AddDefaultVehicle(TEXT("CAR 1"), TEXT("/Game/Cars/Car1.Car1"));
	AddDefaultVehicle(TEXT("CAR 2"), TEXT("/Game/Cars/Car2.Car2"));
	AddDefaultVehicle(TEXT("CAR 3"), TEXT("/Game/Cars/Car3.Car3"));
}

void UVehicleSelectionWidget::CacheThemeFont()
{
	LoadedThemeFontObject = nullptr;
	ThemeFontInfo = FCoreStyle::GetDefaultFontStyle("Regular", 24);

	if (!ThemeFontAsset.IsNull())
	{
		if (UObject* FontObject = ThemeFontAsset.TryLoad())
		{
			LoadedThemeFontObject = FontObject;
			ThemeFontInfo.FontObject = FontObject;
		}
	}
}

FSlateFontInfo UVehicleSelectionWidget::MakeThemeFont(const int32 Size) const
{
	FSlateFontInfo FontInfo = ThemeFontInfo;
	if (FontInfo.Size <= 0 && FontInfo.FontObject == nullptr)
	{
		FontInfo = FCoreStyle::GetDefaultFontStyle("Regular", Size);
	}

	FontInfo.Size = Size;
	return FontInfo;
}

void UVehicleSelectionWidget::BuildRuntimeWidget()
{
	if (!WidgetTree)
	{
		return;
	}

	PlayerStates.SetNum(2);

	RuntimeRootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("VehicleSelectionRootCanvas"));
	WidgetTree->RootWidget = RuntimeRootCanvas;

	UBorder* Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("VehicleSelectionBackground"));
	Background->SetBrushColor(BackgroundTint);
	if (UCanvasPanelSlot* BackgroundSlot = RuntimeRootCanvas->AddChildToCanvas(Background))
	{
		BackgroundSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		BackgroundSlot->SetOffsets(FMargin(0.f, 0.f, 0.f, 0.f));
		BackgroundSlot->SetZOrder(0);
	}

	UVerticalBox* RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("VehicleSelectionRootBox"));
	if (UCanvasPanelSlot* RootSlot = RuntimeRootCanvas->AddChildToCanvas(RootBox))
	{
		RootSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		RootSlot->SetOffsets(RootPadding);
		RootSlot->SetZOrder(1);
	}

	UTextBlock* Title = CreateTextBlock(
		WidgetTree,
		TEXT("VehicleSelectionTitle"),
		TitleText,
		MakeThemeFont(48),
		DefaultTextColor,
		ETextJustify::Center
	);
	if (UVerticalBoxSlot* TitleSlot = RootBox->AddChildToVerticalBox(Title))
	{
		TitleSlot->SetHorizontalAlignment(HAlign_Center);
		TitleSlot->SetPadding(TitlePadding);
	}

	UTextBlock* Subtitle = CreateTextBlock(
		WidgetTree,
		TEXT("VehicleSelectionSubtitle"),
		SubtitleText,
		MakeThemeFont(18),
		MutedTextColor,
		ETextJustify::Center
	);
	if (UVerticalBoxSlot* SubtitleSlot = RootBox->AddChildToVerticalBox(Subtitle))
	{
		SubtitleSlot->SetHorizontalAlignment(HAlign_Center);
		SubtitleSlot->SetPadding(SubtitlePadding);
	}

	UHorizontalBox* PlayersRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("VehicleSelectionPlayersRow"));
	if (UVerticalBoxSlot* PlayersSlot = RootBox->AddChildToVerticalBox(PlayersRow))
	{
		PlayersSlot->SetPadding(PlayersRowPadding);
		FSlateChildSize PlayersSize;
		PlayersSize.SizeRule = ESlateSizeRule::Fill;
		PlayersSize.Value = 1.f;
		PlayersSlot->SetSize(PlayersSize);
	}

	for (int32 PlayerIndex = 0; PlayerIndex < PlayerStates.Num(); ++PlayerIndex)
	{
		FVehicleSelectionPlayerState& PlayerState = PlayerStates[PlayerIndex];
		const FLinearColor AccentColor = GetAccentColor(PlayerIndex);

		UBorder* CardBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), *FString::Printf(TEXT("VehicleCard_%d"), PlayerIndex));
		CardBorder->SetBrushColor(PanelTint);
		CardBorder->SetPadding(CardPadding);
		PlayerState.CardBorder = CardBorder;

		if (UHorizontalBoxSlot* CardSlot = PlayersRow->AddChildToHorizontalBox(CardBorder))
		{
			FSlateChildSize CardSize;
			CardSize.SizeRule = ESlateSizeRule::Fill;
			CardSize.Value = 1.f;
			CardSlot->SetSize(CardSize);
			CardSlot->SetPadding(FMargin(PlayerIndex == 0 ? 0.f : CardSpacing, 0.f, PlayerIndex == 0 ? CardSpacing : 0.f, 0.f));
			CardSlot->SetVerticalAlignment(VAlign_Fill);
		}

		UVerticalBox* CardBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), *FString::Printf(TEXT("VehicleCardBox_%d"), PlayerIndex));
		CardBorder->SetContent(CardBox);

		PlayerState.PlayerLabel = CreateTextBlock(
			WidgetTree,
			FString::Printf(TEXT("PlayerLabel_%d"), PlayerIndex),
			PlayerIndex == 0 ? PlayerOneTitleText : PlayerTwoTitleText,
			MakeThemeFont(28),
			AccentColor,
			ETextJustify::Center
		);
		if (UVerticalBoxSlot* PlayerLabelSlot = CardBox->AddChildToVerticalBox(PlayerState.PlayerLabel))
		{
			PlayerLabelSlot->SetHorizontalAlignment(HAlign_Center);
			PlayerLabelSlot->SetPadding(PlayerLabelPadding);
		}

		USizeBox* PreviewSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), *FString::Printf(TEXT("PreviewSizeBox_%d"), PlayerIndex));
		PreviewSizeBox->SetWidthOverride(PreviewSize.X);
		PreviewSizeBox->SetHeightOverride(PreviewSize.Y);

		PlayerState.PreviewViewport = WidgetTree->ConstructWidget<UViewport>(UViewport::StaticClass(), *FString::Printf(TEXT("VehiclePreview_%d"), PlayerIndex));
		PlayerState.PreviewViewport->SetBackgroundColor(PreviewBackgroundTint);
		PreviewSizeBox->SetContent(PlayerState.PreviewViewport);

		if (UVerticalBoxSlot* PreviewSlot = CardBox->AddChildToVerticalBox(PreviewSizeBox))
		{
			PreviewSlot->SetHorizontalAlignment(HAlign_Center);
			PreviewSlot->SetPadding(PreviewPadding);
		}

		UHorizontalBox* NavRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), *FString::Printf(TEXT("NavRow_%d"), PlayerIndex));
		if (UVerticalBoxSlot* NavRowSlot = CardBox->AddChildToVerticalBox(NavRow))
		{
			NavRowSlot->SetHorizontalAlignment(HAlign_Center);
			NavRowSlot->SetPadding(NavigationPadding);
		}

		UTextBlock* LeftLabel = nullptr;
		PlayerState.LeftButton = CreateStyledButton(WidgetTree, FString::Printf(TEXT("LeftButton_%d"), PlayerIndex), LeftButtonText, MakeThemeFont(26), LeftLabel);
		if (UHorizontalBoxSlot* LeftSlot = NavRow->AddChildToHorizontalBox(PlayerState.LeftButton))
		{
			LeftSlot->SetPadding(FMargin(0.f, 0.f, NavigationButtonSpacing, 0.f));
		}

		UTextBlock* RightLabel = nullptr;
		PlayerState.RightButton = CreateStyledButton(WidgetTree, FString::Printf(TEXT("RightButton_%d"), PlayerIndex), RightButtonText, MakeThemeFont(26), RightLabel);
		if (UHorizontalBoxSlot* RightSlot = NavRow->AddChildToHorizontalBox(PlayerState.RightButton))
		{
			RightSlot->SetPadding(FMargin(NavigationButtonSpacing, 0.f, 0.f, 0.f));
		}

		if (PlayerIndex == 0)
		{
			PlayerState.LeftButton->OnClicked.AddDynamic(this, &UVehicleSelectionWidget::OnPlayerOneLeftClicked);
			PlayerState.RightButton->OnClicked.AddDynamic(this, &UVehicleSelectionWidget::OnPlayerOneRightClicked);
		}
		else
		{
			PlayerState.LeftButton->OnClicked.AddDynamic(this, &UVehicleSelectionWidget::OnPlayerTwoLeftClicked);
			PlayerState.RightButton->OnClicked.AddDynamic(this, &UVehicleSelectionWidget::OnPlayerTwoRightClicked);
		}

		PlayerState.VehicleLabel = CreateTextBlock(
			WidgetTree,
			FString::Printf(TEXT("VehicleLabel_%d"), PlayerIndex),
			FText::FromString(TEXT("---")),
			MakeThemeFont(30),
			DefaultTextColor,
			ETextJustify::Center
		);
		if (UVerticalBoxSlot* VehicleLabelSlot = CardBox->AddChildToVerticalBox(PlayerState.VehicleLabel))
		{
			VehicleLabelSlot->SetHorizontalAlignment(HAlign_Center);
			VehicleLabelSlot->SetPadding(VehicleNamePadding);
		}

		PlayerState.StatusLabel = CreateTextBlock(
			WidgetTree,
			FString::Printf(TEXT("StatusLabel_%d"), PlayerIndex),
			WaitingStatusText,
			MakeThemeFont(18),
			MutedTextColor,
			ETextJustify::Center
		);
		if (UVerticalBoxSlot* StatusSlot = CardBox->AddChildToVerticalBox(PlayerState.StatusLabel))
		{
			StatusSlot->SetHorizontalAlignment(HAlign_Center);
			StatusSlot->SetPadding(StatusPadding);
		}

		UTextBlock* ConfirmLabel = nullptr;
		PlayerState.ConfirmButton = CreateStyledButton(
			WidgetTree,
			FString::Printf(TEXT("ConfirmButton_%d"), PlayerIndex),
			ConfirmPromptText,
			MakeThemeFont(20),
			ConfirmLabel
		);
		PlayerState.ConfirmLabel = ConfirmLabel;
		if (UVerticalBoxSlot* ConfirmSlot = CardBox->AddChildToVerticalBox(PlayerState.ConfirmButton))
		{
			ConfirmSlot->SetHorizontalAlignment(HAlign_Center);
		}

		if (PlayerIndex == 0)
		{
			PlayerState.ConfirmButton->OnClicked.AddDynamic(this, &UVehicleSelectionWidget::OnPlayerOneConfirmClicked);
		}
		else
		{
			PlayerState.ConfirmButton->OnClicked.AddDynamic(this, &UVehicleSelectionWidget::OnPlayerTwoConfirmClicked);
		}
	}

	BottomInstructionText = CreateTextBlock(
		WidgetTree,
		TEXT("VehicleSelectionInstruction"),
		BottomInstructionTextContent,
		MakeThemeFont(16),
		MutedTextColor,
		ETextJustify::Center
	);
	if (UVerticalBoxSlot* InstructionSlot = RootBox->AddChildToVerticalBox(BottomInstructionText))
	{
		InstructionSlot->SetHorizontalAlignment(HAlign_Center);
		InstructionSlot->SetPadding(BottomInstructionPadding);
	}
}

void UVehicleSelectionWidget::EnsureTwoLocalPlayers()
{
	if (!GetWorld())
	{
		return;
	}

	UGameInstance* GameInstance = GetWorld()->GetGameInstance();
	if (!GameInstance)
	{
		return;
	}

	for (int32 PlayerIndex = GameInstance->GetNumLocalPlayers(); PlayerIndex < 2; ++PlayerIndex)
	{
		UGameplayStatics::CreatePlayer(this, PlayerIndex, true);
	}
}

void UVehicleSelectionWidget::ConfigureSelectionInput() const
{
	for (int32 PlayerIndex = 0; PlayerIndex < 2; ++PlayerIndex)
	{
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, PlayerIndex))
		{
			if (PC->PlayerInput)
			{
				PC->PlayerInput->FlushPressedKeys();
			}

			PC->bShowMouseCursor = (PlayerIndex == 0);
			UWidgetBlueprintLibrary::SetInputMode_GameAndUIEx(PC, const_cast<UVehicleSelectionWidget*>(this), EMouseLockMode::DoNotLock, false, true);
		}
	}
}

void UVehicleSelectionWidget::FocusAllUsers()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().SetAllUserFocus(TakeWidget(), EFocusCause::SetDirectly);
	}
}

void UVehicleSelectionWidget::SaveSelectionsToGameInstance() const
{
	if (!GetWorld())
	{
		return;
	}

	UProtoGameLabGameInstance* GameInstance = Cast<UProtoGameLabGameInstance>(GetWorld()->GetGameInstance());
	if (!GameInstance)
	{
		return;
	}

	TArray<FSoftObjectPath> SelectedMeshes;
	SelectedMeshes.Reserve(PlayerStates.Num());

	for (const FVehicleSelectionPlayerState& PlayerState : PlayerStates)
	{
		if (VehicleOptions.IsValidIndex(PlayerState.SelectedIndex))
		{
			SelectedMeshes.Add(VehicleOptions[PlayerState.SelectedIndex].VehicleMesh.ToSoftObjectPath());
		}
		else
		{
			SelectedMeshes.Add(FSoftObjectPath());
		}
	}

	GameInstance->SetSelectedVehicleMeshes(SelectedMeshes);
}

void UVehicleSelectionWidget::TryAdvanceToTargetLevel()
{
	if (PlayerStates.Num() < 2)
	{
		return;
	}

	for (const FVehicleSelectionPlayerState& PlayerState : PlayerStates)
	{
		if (!PlayerState.bConfirmed)
		{
			return;
		}
	}

	SaveSelectionsToGameInstance();

	const FName LevelToOpen = TargetLevelName.IsNone() ? FName(TEXT("Lvl_Test_2Players")) : TargetLevelName;

	for (int32 PlayerIndex = 0; PlayerIndex < 2; ++PlayerIndex)
	{
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, PlayerIndex))
		{
			if (PC->PlayerInput)
			{
				PC->PlayerInput->FlushPressedKeys();
			}

			PC->bShowMouseCursor = false;
			UWidgetBlueprintLibrary::SetInputMode_GameOnly(PC, true);
		}
	}

	UGameplayStatics::OpenLevel(this, LevelToOpen);
}

void UVehicleSelectionWidget::FinalizeInitialPreviewSetup()
{
	bPendingInitialPreviewSetup = false;
	UE_LOG(LogTemp, Warning, TEXT("[VEHICLE SELECT] FinalizeInitialPreviewSetup"));
	SilenceMainMenuWorldActors();

	for (int32 PlayerIndex = 0; PlayerIndex < PlayerStates.Num(); ++PlayerIndex)
	{
		ConfigurePreviewViewport(PlayerIndex);
		UpdatePlayerCard(PlayerIndex);
		RefreshPreview(PlayerIndex);
	}
}

void UVehicleSelectionWidget::ConfigurePreviewViewport(const int32 PlayerIndex)
{
	if (!PlayerStates.IsValidIndex(PlayerIndex))
	{
		return;
	}

	UViewport* PreviewViewport = PlayerStates[PlayerIndex].PreviewViewport;
	if (!PreviewViewport)
	{
		return;
	}

	PreviewViewport->SetEnableAdvancedFeatures(true);
	PreviewViewport->SetViewLocation(PreviewCameraLocation);
	PreviewViewport->SetViewRotation(PreviewCameraRotation);
}

void UVehicleSelectionWidget::SilenceMainMenuWorldActors()
{
	TArray<AActor*> RacerPawnActors;
	UGameplayStatics::GetAllActorsOfClass(this, ASTR_RacerPawn::StaticClass(), RacerPawnActors);

	for (AActor* Actor : RacerPawnActors)
	{
		ASTR_RacerPawn* RacerPawn = Cast<ASTR_RacerPawn>(Actor);
		if (!RacerPawn)
		{
			continue;
		}

		RacerPawn->SetActorHiddenInGame(true);
		RacerPawn->SetActorEnableCollision(false);
		RacerPawn->SetActorTickEnabled(false);
	}
}

void UVehicleSelectionWidget::HandleSelectionChange(const int32 PlayerIndex, const int32 Direction)
{
	if (!PlayerStates.IsValidIndex(PlayerIndex) || VehicleOptions.IsEmpty() || Direction == 0)
	{
		return;
	}

	FVehicleSelectionPlayerState& PlayerState = PlayerStates[PlayerIndex];
	const int32 NumVehicles = VehicleOptions.Num();
	PlayerState.SelectedIndex = (PlayerState.SelectedIndex + Direction + NumVehicles) % NumVehicles;
	PlayerState.bConfirmed = false;

	UpdatePlayerCard(PlayerIndex);
	RefreshPreview(PlayerIndex);
}

void UVehicleSelectionWidget::HandleConfirm(const int32 PlayerIndex)
{
	if (!PlayerStates.IsValidIndex(PlayerIndex) || VehicleOptions.IsEmpty() || !bAcceptInputArmed)
	{
		return;
	}

	FVehicleSelectionPlayerState& PlayerState = PlayerStates[PlayerIndex];
	if (!PlayerState.bConfirmed)
	{
		PlayerState.bConfirmed = true;
		UpdatePlayerCard(PlayerIndex);
	}

	TryAdvanceToTargetLevel();
}

void UVehicleSelectionWidget::UpdatePlayerCard(const int32 PlayerIndex)
{
	if (!PlayerStates.IsValidIndex(PlayerIndex))
	{
		return;
	}

	FVehicleSelectionPlayerState& PlayerState = PlayerStates[PlayerIndex];
	const FLinearColor AccentColor = GetAccentColor(PlayerIndex);
	const bool bHasVehicle = VehicleOptions.IsValidIndex(PlayerState.SelectedIndex);

	if (PlayerState.CardBorder)
	{
		PlayerState.CardBorder->SetBrushColor(PlayerState.bConfirmed
			? FLinearColor(AccentColor.R * 0.22f, AccentColor.G * 0.22f, AccentColor.B * 0.22f, 0.96f)
			: PanelTint);
	}

	if (PlayerState.VehicleLabel)
	{
		PlayerState.VehicleLabel->SetText(FText::FromString(
			bHasVehicle ? GetVehicleDisplayName(VehicleOptions[PlayerState.SelectedIndex], PlayerState.SelectedIndex) : FString(TEXT("NO VEHICLE"))
		));
		PlayerState.VehicleLabel->SetColorAndOpacity(FSlateColor(PlayerState.bConfirmed ? AccentColor : DefaultTextColor));
	}

	if (PlayerState.StatusLabel)
	{
		PlayerState.StatusLabel->SetText(PlayerState.bConfirmed ? ReadyStatusText : WaitingStatusText);
		PlayerState.StatusLabel->SetColorAndOpacity(FSlateColor(PlayerState.bConfirmed ? ReadyTextColor : MutedTextColor));
	}

	if (PlayerState.ConfirmLabel)
	{
		PlayerState.ConfirmLabel->SetText(PlayerState.bConfirmed ? LockedInText : ConfirmPromptText);
		PlayerState.ConfirmLabel->SetColorAndOpacity(FSlateColor(PlayerState.bConfirmed ? ReadyTextColor : DefaultTextColor));
	}
}

void UVehicleSelectionWidget::RefreshPreview(const int32 PlayerIndex)
{
	if (!PlayerStates.IsValidIndex(PlayerIndex))
	{
		return;
	}

	FVehicleSelectionPlayerState& PlayerState = PlayerStates[PlayerIndex];
	DestroyPreviewActor(PlayerIndex);

	if (!PlayerState.PreviewViewport || !VehicleOptions.IsValidIndex(PlayerState.SelectedIndex))
	{
		return;
	}

	const FVehicleSelectionOption& VehicleOption = VehicleOptions[PlayerState.SelectedIndex];
	TSubclassOf<AVehicleSelectionPreviewActor> PreviewClass = VehicleOption.PreviewActorClass;
	if (!PreviewClass)
	{
		PreviewClass = AVehicleSelectionPreviewActor::StaticClass();
	}

	if (AVehicleSelectionPreviewActor* PreviewActor = Cast<AVehicleSelectionPreviewActor>(PlayerState.PreviewViewport->Spawn(PreviewClass)))
	{
		if (UStaticMesh* VehicleMesh = VehicleOption.VehicleMesh.LoadSynchronous())
		{
			PreviewActor->SetPreviewMesh(VehicleMesh);
		}

		PreviewActor->SetActorLocation(VehicleOption.PreviewLocation);
		PreviewActor->SetActorRotation(VehicleOption.PreviewRotation);
		PreviewActor->SetActorScale3D(VehicleOption.PreviewScale);
		PlayerState.PreviewActor = PreviewActor;
	}
}

void UVehicleSelectionWidget::ResetAcceptInputGate(const float ArmDelaySeconds)
{
	AcceptInputArmDelayRemaining = FMath::Max(0.f, ArmDelaySeconds);
	bAcceptInputArmed = false;
	bAcceptPressedDuringGate = false;
}

void UVehicleSelectionWidget::UpdateAcceptInputGate(const float DeltaTime)
{
	if (bAcceptInputArmed)
	{
		return;
	}

	AcceptInputArmDelayRemaining = FMath::Max(0.f, AcceptInputArmDelayRemaining - DeltaTime);
	if (AcceptInputArmDelayRemaining <= 0.f && !bAcceptPressedDuringGate)
	{
		bAcceptInputArmed = true;
	}
}

int32 UVehicleSelectionWidget::ResolvePlayerIndexFromKey(const FKeyEvent& InKeyEvent) const
{
	const FKey Key = InKeyEvent.GetKey();

	if (Key == EKeys::Left || Key == EKeys::Right || Key == EKeys::Enter || Key == EKeys::RightShift)
	{
		return 1;
	}

	return InKeyEvent.GetUserIndex() > 0 ? 1 : 0;
}

bool UVehicleSelectionWidget::IsConfirmKeyForPlayer(const int32 PlayerIndex, const FKey& Key) const
{
	if (Key == EKeys::Gamepad_FaceButton_Bottom || Key == EKeys::Virtual_Gamepad_Accept.GetVirtualKey())
	{
		return true;
	}

	if (PlayerIndex == 0)
	{
		return Key == EKeys::SpaceBar;
	}

	return Key == EKeys::Enter || Key == EKeys::RightShift;
}

bool UVehicleSelectionWidget::IsLeftKeyForPlayer(const int32 PlayerIndex, const FKey& Key) const
{
	if (Key == EKeys::Gamepad_DPad_Left)
	{
		return true;
	}

	if (PlayerIndex == 0)
	{
		return Key == EKeys::A || Key == EKeys::Q;
	}

	return Key == EKeys::Left || Key == EKeys::J;
}

bool UVehicleSelectionWidget::IsRightKeyForPlayer(const int32 PlayerIndex, const FKey& Key) const
{
	if (Key == EKeys::Gamepad_DPad_Right)
	{
		return true;
	}

	if (PlayerIndex == 0)
	{
		return Key == EKeys::D;
	}

	return Key == EKeys::Right || Key == EKeys::L;
}

void UVehicleSelectionWidget::DestroyPreviewActor(const int32 PlayerIndex)
{
	if (!PlayerStates.IsValidIndex(PlayerIndex))
	{
		return;
	}

	if (AVehicleSelectionPreviewActor* PreviewActor = PlayerStates[PlayerIndex].PreviewActor)
	{
		PreviewActor->Destroy();
		PlayerStates[PlayerIndex].PreviewActor = nullptr;
	}
}
