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
#include "Engine/Blueprint.h"
#include "Engine/StaticMesh.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerInput.h"
#include "Input/Events.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Math/BoxSphereBounds.h"
#include "Materials/MaterialInterface.h"
#include "ProtoGameLabGameInstance.h"
#include "STR_RacerPawn.h"
#include "Styling/CoreStyle.h"
#include "VehicleSelectionPreviewActor.h"
#include "UObject/SoftObjectPtr.h"
#include "Widgets/SWidget.h"

namespace VehicleSelectionWidgetPrivate
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

	FSoftObjectPath GetVehicleSourcePath(const FVehicleSelectionOption& Option)
	{
		if (!Option.VehiclePawnClass.IsNull())
		{
			return Option.VehiclePawnClass.ToSoftObjectPath();
		}

		if (!Option.VehicleSourceAsset.IsNull())
		{
			return Option.VehicleSourceAsset;
		}

		return Option.VehicleMesh.ToSoftObjectPath();
	}

	struct FResolvedVehiclePreviewData
	{
		UStaticMesh* Mesh = nullptr;
		TArray<UMaterialInterface*> Materials;
		FVector RelativeLocation = FVector::ZeroVector;
		FRotator RelativeRotation = FRotator::ZeroRotator;
		FVector RelativeScale = FVector(1.f, 1.f, 1.f);

		bool IsValid() const
		{
			return Mesh != nullptr;
		}
	};

	FResolvedVehiclePreviewData ResolveVehiclePreviewDataFromClass(UClass* VehicleClass)
	{
		FResolvedVehiclePreviewData PreviewData;

		if (!VehicleClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("[VehicleSelect] ResolveVehiclePreviewDataFromClass: VehicleClass is null"));
			return PreviewData;
		}

		if (!VehicleClass->IsChildOf(ASTR_RacerPawn::StaticClass()))
		{
			UE_LOG(LogTemp, Warning, TEXT("[VehicleSelect] ResolveVehiclePreviewDataFromClass: %s is not a STR_RacerPawn class"), *GetNameSafe(VehicleClass));
			return PreviewData;
		}

		const ASTR_RacerPawn* DefaultPawn = Cast<ASTR_RacerPawn>(VehicleClass->GetDefaultObject());
		if (!DefaultPawn)
		{
			UE_LOG(LogTemp, Warning, TEXT("[VehicleSelect] ResolveVehiclePreviewDataFromClass: Default object missing for %s"), *GetNameSafe(VehicleClass));
			return PreviewData;
		}

		if (!DefaultPawn->CarMesh)
		{
			UE_LOG(LogTemp, Warning, TEXT("[VehicleSelect] ResolveVehiclePreviewDataFromClass: CarMesh missing on %s"), *GetNameSafe(VehicleClass));
			return PreviewData;
		}

		PreviewData.Mesh = DefaultPawn->CarMesh->GetStaticMesh();
		PreviewData.RelativeLocation = DefaultPawn->CarMesh->GetRelativeLocation();
		PreviewData.RelativeRotation = DefaultPawn->CarMesh->GetRelativeRotation();
		PreviewData.RelativeScale = DefaultPawn->CarMesh->GetRelativeScale3D();

		const int32 NumMaterials = DefaultPawn->CarMesh->GetNumMaterials();
		PreviewData.Materials.Reserve(NumMaterials);
		for (int32 MaterialIndex = 0; MaterialIndex < NumMaterials; ++MaterialIndex)
		{
			PreviewData.Materials.Add(DefaultPawn->CarMesh->GetMaterial(MaterialIndex));
		}

		UE_LOG(LogTemp, Warning, TEXT("[VehicleSelect] Class=%s Mesh=%s Materials=%d Loc=%s Rot=%s Scale=%s"),
			*GetNameSafe(VehicleClass),
			DefaultPawn->CarMesh ? *GetNameSafe(DefaultPawn->CarMesh->GetStaticMesh()) : TEXT("NULL"),
			DefaultPawn->CarMesh ? DefaultPawn->CarMesh->GetNumMaterials() : -1,
			DefaultPawn->CarMesh ? *DefaultPawn->CarMesh->GetRelativeLocation().ToString() : TEXT("NO_CARMESH"),
			DefaultPawn->CarMesh ? *DefaultPawn->CarMesh->GetRelativeRotation().ToString() : TEXT("NO_CARMESH"),
			DefaultPawn->CarMesh ? *DefaultPawn->CarMesh->GetRelativeScale3D().ToString() : TEXT("NO_CARMESH"));

		return PreviewData;
	}

	FResolvedVehiclePreviewData ResolveVehiclePreviewDataFromPath(const FSoftObjectPath& VehiclePath)
	{
		FResolvedVehiclePreviewData PreviewData;

		if (VehiclePath.IsNull())
		{
			return PreviewData;
		}

		if (UObject* LoadedObject = VehiclePath.TryLoad())
		{
			if (UStaticMesh* StaticMesh = Cast<UStaticMesh>(LoadedObject))
			{
				PreviewData.Mesh = StaticMesh;
				return PreviewData;
			}

			if (UBlueprint* Blueprint = Cast<UBlueprint>(LoadedObject))
			{
				return ResolveVehiclePreviewDataFromClass(Blueprint->GeneratedClass);
			}

			if (UClass* LoadedClass = Cast<UClass>(LoadedObject))
			{
				return ResolveVehiclePreviewDataFromClass(LoadedClass);
			}
		}

		return PreviewData;
	}

	FVector GetPreviewFocusPoint(const FBoxSphereBounds& Bounds)
	{
		return Bounds.Origin + FVector(0.f, 0.f, Bounds.BoxExtent.Z * 0.08f);
	}

	void FramePreviewViewport(
		UViewport* PreviewViewport,
		const FBoxSphereBounds& Bounds,
		const FRotator& OrbitRotation,
		const float DistanceMultiplier,
		const float MinimumDistance)
	{
		if (!PreviewViewport || Bounds.SphereRadius <= KINDA_SMALL_NUMBER)
		{
			return;
		}

		const FVector FocusPoint = GetPreviewFocusPoint(Bounds);
		const FVector ViewDirection = OrbitRotation.Vector().GetSafeNormal();
		const float CameraDistance = FMath::Max(MinimumDistance, Bounds.SphereRadius * DistanceMultiplier);
		const FVector ViewLocation = FocusPoint - (ViewDirection * CameraDistance);
		const FRotator ViewRotation = (FocusPoint - ViewLocation).Rotation();

		PreviewViewport->SetViewLocation(ViewLocation);
		PreviewViewport->SetViewRotation(ViewRotation);
	}
}

void UVehicleSelectionWidget::SetTargetLevelName(const FName InTargetLevelName)
{
	TargetLevelName = InTargetLevelName;
}

int32 UVehicleSelectionWidget::GetSelectedVehicleIndex(const int32 PlayerIndex) const
{
	if (!PlayerStates.IsValidIndex(PlayerIndex))
	{
		return INDEX_NONE;
	}

	return PlayerStates[PlayerIndex].SelectedIndex;
}

FText UVehicleSelectionWidget::GetSelectedVehicleDisplayName(const int32 PlayerIndex) const
{
	if (!PlayerStates.IsValidIndex(PlayerIndex))
	{
		return FText::GetEmpty();
	}

	const int32 SelectedIndex = PlayerStates[PlayerIndex].SelectedIndex;
	if (!VehicleOptions.IsValidIndex(SelectedIndex))
	{
		return FText::GetEmpty();
	}

	return FText::FromString(VehicleSelectionWidgetPrivate::GetVehicleDisplayName(VehicleOptions[SelectedIndex], SelectedIndex));
}

bool UVehicleSelectionWidget::IsPlayerSelectionConfirmed(const int32 PlayerIndex) const
{
	return PlayerStates.IsValidIndex(PlayerIndex) && PlayerStates[PlayerIndex].bConfirmed;
}

FText UVehicleSelectionWidget::GetPlayerStatusDisplayText(const int32 PlayerIndex) const
{
	if (!PlayerStates.IsValidIndex(PlayerIndex))
	{
		return FText::GetEmpty();
	}

	return PlayerStates[PlayerIndex].bConfirmed ? ReadyStatusText : WaitingStatusText;
}

FText UVehicleSelectionWidget::GetPlayerConfirmDisplayText(const int32 PlayerIndex) const
{
	if (!PlayerStates.IsValidIndex(PlayerIndex))
	{
		return FText::GetEmpty();
	}

	return PlayerStates[PlayerIndex].bConfirmed ? UnlockPromptText : ConfirmPromptText;
}

bool UVehicleSelectionWidget::AreAllPlayersReadyForTransition() const
{
	return AreAllPlayersConfirmed();
}

TSharedRef<SWidget> UVehicleSelectionWidget::RebuildWidget()
{
	InitializeDefaultVehicleOptions();
	CacheThemeFont();
	BuildRuntimeWidget();
	//UE_LOG(LogTemp, Warning, TEXT("[VEHICLE SELECT] RebuildWidget built runtime tree with %d options"), VehicleOptions.Num());

	return Super::RebuildWidget();
}

void UVehicleSelectionWidget::NativeConstruct()
{
	Super::NativeConstruct();
	//UE_LOG(LogTemp, Warning, TEXT("[VEHICLE SELECT] NativeConstruct"));

	SetIsFocusable(true);
	EnsureTwoLocalPlayers();
	bPendingInitialPreviewSetup = true;
	//UE_LOG(LogTemp, Warning, TEXT("[VEHICLE SELECT] Runtime widget ready with %d options"), VehicleOptions.Num());

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

	if (bTransitionToTargetLevelPending)
	{
		TransitionDelayRemaining = FMath::Max(0.f, TransitionDelayRemaining - InDeltaTime);
		if (TransitionDelayRemaining <= 0.f)
		{
			CompleteAdvanceToTargetLevel();
			return;
		}
	}

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
	auto HasVehicleOption = [this](const TCHAR* AssetPath)
	{
		const FSoftObjectPath TargetPath(AssetPath);

		for (const FVehicleSelectionOption& ExistingOption : VehicleOptions)
		{
			if (ExistingOption.VehiclePawnClass.ToSoftObjectPath() == TargetPath
				|| ExistingOption.VehicleSourceAsset == TargetPath)
			{
				return true;
			}
		}

		return false;
	};

	auto AddDefaultVehicle = [this, &HasVehicleOption](const TCHAR* Name, const TCHAR* AssetPath)
	{
		if (HasVehicleOption(AssetPath))
		{
			return;
		}

		FVehicleSelectionOption& NewOption = VehicleOptions.AddDefaulted_GetRef();
		NewOption.DisplayName = FText::FromString(Name);
		NewOption.VehiclePawnClass = TSoftClassPtr<ASTR_RacerPawn>(FSoftObjectPath(AssetPath));
		NewOption.VehicleSourceAsset = FSoftObjectPath(AssetPath);
		NewOption.PreviewScale = FVector(1.0f, 1.0f, 1.0f);
	};

	AddDefaultVehicle(TEXT("CAR 1"), TEXT("/Game/Cars/Car1.Car1_C"));
	AddDefaultVehicle(TEXT("CAR 2"), TEXT("/Game/Cars/Car2.Car2_C"));
	AddDefaultVehicle(TEXT("CAR 3"), TEXT("/Game/Cars/Car3.Car3_C"));
	AddDefaultVehicle(TEXT("CAR 4"), TEXT("/Game/Cars/Car4.Car4_C"));
	AddDefaultVehicle(TEXT("CAR 5"), TEXT("/Game/Cars/Car5.Car5_C"));
	AddDefaultVehicle(TEXT("CAR 6"), TEXT("/Game/Cars/Car6.Car6_C"));
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

	UTextBlock* Title = VehicleSelectionWidgetPrivate::CreateTextBlock(
		WidgetTree,
		TEXT("VehicleSelectionTitle"),
		TitleText,
		MakeThemeFont(48),
		VehicleSelectionWidgetPrivate::DefaultTextColor,
		ETextJustify::Center
	);
	if (UVerticalBoxSlot* TitleSlot = RootBox->AddChildToVerticalBox(Title))
	{
		TitleSlot->SetHorizontalAlignment(HAlign_Center);
		TitleSlot->SetPadding(TitlePadding);
	}

	UTextBlock* Subtitle = VehicleSelectionWidgetPrivate::CreateTextBlock(
		WidgetTree,
		TEXT("VehicleSelectionSubtitle"),
		SubtitleText,
		MakeThemeFont(18),
		VehicleSelectionWidgetPrivate::MutedTextColor,
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
		const FLinearColor AccentColor = VehicleSelectionWidgetPrivate::GetAccentColor(PlayerIndex);

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

		PlayerState.PlayerLabel = VehicleSelectionWidgetPrivate::CreateTextBlock(
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
		PlayerState.LeftButton = VehicleSelectionWidgetPrivate::CreateStyledButton(WidgetTree, FString::Printf(TEXT("LeftButton_%d"), PlayerIndex), LeftButtonText, MakeThemeFont(26), LeftLabel);
		if (UHorizontalBoxSlot* LeftSlot = NavRow->AddChildToHorizontalBox(PlayerState.LeftButton))
		{
			LeftSlot->SetPadding(FMargin(0.f, 0.f, NavigationButtonSpacing, 0.f));
		}

		UTextBlock* RightLabel = nullptr;
		PlayerState.RightButton = VehicleSelectionWidgetPrivate::CreateStyledButton(WidgetTree, FString::Printf(TEXT("RightButton_%d"), PlayerIndex), RightButtonText, MakeThemeFont(26), RightLabel);
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

		PlayerState.VehicleLabel = VehicleSelectionWidgetPrivate::CreateTextBlock(
			WidgetTree,
			FString::Printf(TEXT("VehicleLabel_%d"), PlayerIndex),
			FText::FromString(TEXT("---")),
			MakeThemeFont(30),
			VehicleSelectionWidgetPrivate::DefaultTextColor,
			ETextJustify::Center
		);
		if (UVerticalBoxSlot* VehicleLabelSlot = CardBox->AddChildToVerticalBox(PlayerState.VehicleLabel))
		{
			VehicleLabelSlot->SetHorizontalAlignment(HAlign_Center);
			VehicleLabelSlot->SetPadding(VehicleNamePadding);
		}

		PlayerState.StatusLabel = VehicleSelectionWidgetPrivate::CreateTextBlock(
			WidgetTree,
			FString::Printf(TEXT("StatusLabel_%d"), PlayerIndex),
			WaitingStatusText,
			MakeThemeFont(18),
			VehicleSelectionWidgetPrivate::MutedTextColor,
			ETextJustify::Center
		);
		if (UVerticalBoxSlot* StatusSlot = CardBox->AddChildToVerticalBox(PlayerState.StatusLabel))
		{
			StatusSlot->SetHorizontalAlignment(HAlign_Center);
			StatusSlot->SetPadding(StatusPadding);
		}

		UTextBlock* ConfirmLabel = nullptr;
		PlayerState.ConfirmButton = VehicleSelectionWidgetPrivate::CreateStyledButton(
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

	BottomInstructionText = VehicleSelectionWidgetPrivate::CreateTextBlock(
		WidgetTree,
		TEXT("VehicleSelectionInstruction"),
		BottomInstructionTextContent,
		MakeThemeFont(16),
		VehicleSelectionWidgetPrivate::MutedTextColor,
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
			SelectedMeshes.Add(VehicleSelectionWidgetPrivate::GetVehicleSourcePath(VehicleOptions[PlayerState.SelectedIndex]));
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
	if (!AreAllPlayersConfirmed())
	{
		return;
	}

	bTransitionToTargetLevelPending = true;
	TransitionDelayRemaining = FMath::Max(0.f, TransitionDelaySeconds);
	RefreshBottomInstruction();
	OnAllPlayersReady();

	if (TransitionDelayRemaining <= 0.f)
	{
		CompleteAdvanceToTargetLevel();
	}
}

void UVehicleSelectionWidget::CompleteAdvanceToTargetLevel()
{
	bTransitionToTargetLevelPending = false;
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

	RefreshBottomInstruction();
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
	PreviewViewport->SetLightIntensity(8.0f);
	PreviewViewport->SetSkyIntensity(1.2f);
	PreviewViewport->SetShowFlag(TEXT("SkyAtmosphere"), false);
	PreviewViewport->SetShowFlag(TEXT("Atmosphere"), false);
	PreviewViewport->SetShowFlag(TEXT("Fog"), false);
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
	const bool bWasConfirmed = PlayerState.bConfirmed;
	PlayerState.SelectedIndex = (PlayerState.SelectedIndex + Direction + NumVehicles) % NumVehicles;
	PlayerState.bConfirmed = false;
	bTransitionToTargetLevelPending = false;
	TransitionDelayRemaining = 0.f;

	UpdatePlayerCard(PlayerIndex);
	RefreshPreview(PlayerIndex);
	if (bWasConfirmed)
	{
		RefreshBottomInstruction();
	}
	OnSelectionChanged(PlayerIndex, PlayerState.SelectedIndex);
}

void UVehicleSelectionWidget::HandleConfirm(const int32 PlayerIndex)
{
	if (!PlayerStates.IsValidIndex(PlayerIndex) || VehicleOptions.IsEmpty() || !bAcceptInputArmed)
	{
		return;
	}

	FVehicleSelectionPlayerState& PlayerState = PlayerStates[PlayerIndex];
	PlayerState.bConfirmed = !PlayerState.bConfirmed;
	if (PlayerState.bConfirmed)
	{
		UpdatePlayerCard(PlayerIndex);
		TryAdvanceToTargetLevel();

		//blueprint event
		OnPlayerConfirmed(PlayerIndex);
	}
	else
	{
		bTransitionToTargetLevelPending = false;
		TransitionDelayRemaining = 0.f;
		UpdatePlayerCard(PlayerIndex);
		RefreshBottomInstruction();

		//blueprint event
		OnPlayerUnconfirmed(PlayerIndex);
	}
}

void UVehicleSelectionWidget::UpdatePlayerCard(const int32 PlayerIndex)
{
	if (!PlayerStates.IsValidIndex(PlayerIndex))
	{
		return;
	}

	FVehicleSelectionPlayerState& PlayerState = PlayerStates[PlayerIndex];
	const FLinearColor AccentColor = VehicleSelectionWidgetPrivate::GetAccentColor(PlayerIndex);
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
			bHasVehicle ? VehicleSelectionWidgetPrivate::GetVehicleDisplayName(VehicleOptions[PlayerState.SelectedIndex], PlayerState.SelectedIndex) : FString(TEXT("NO VEHICLE"))
		));
		PlayerState.VehicleLabel->SetColorAndOpacity(FSlateColor(PlayerState.bConfirmed ? AccentColor : VehicleSelectionWidgetPrivate::DefaultTextColor));
	}

	if (PlayerState.StatusLabel)
	{
		PlayerState.StatusLabel->SetText(PlayerState.bConfirmed ? LockedInText : WaitingStatusText);
		PlayerState.StatusLabel->SetColorAndOpacity(FSlateColor(PlayerState.bConfirmed ? VehicleSelectionWidgetPrivate::ReadyTextColor : VehicleSelectionWidgetPrivate::MutedTextColor));
	}

	if (PlayerState.ConfirmLabel)
	{
		PlayerState.ConfirmLabel->SetText(PlayerState.bConfirmed ? UnlockPromptText : ConfirmPromptText);
		PlayerState.ConfirmLabel->SetColorAndOpacity(FSlateColor(PlayerState.bConfirmed ? VehicleSelectionWidgetPrivate::ReadyTextColor : VehicleSelectionWidgetPrivate::DefaultTextColor));
	}
}

void UVehicleSelectionWidget::RefreshBottomInstruction()
{
	if (!BottomInstructionText)
	{
		return;
	}

	if (bTransitionToTargetLevelPending)
	{
		BottomInstructionText->SetText(StartingTutorialText);
		return;
	}

	BottomInstructionText->SetText(AreAllPlayersConfirmed() ? ReadyStatusText : BottomInstructionTextContent);
}

bool UVehicleSelectionWidget::AreAllPlayersConfirmed() const
{
	if (PlayerStates.Num() < 2)
	{
		return false;
	}

	for (const FVehicleSelectionPlayerState& PlayerState : PlayerStates)
	{
		if (!PlayerState.bConfirmed)
		{
			return false;
		}
	}

	return true;
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

	VehicleSelectionWidgetPrivate::FResolvedVehiclePreviewData PreviewData;

	if (!VehicleOption.VehiclePawnClass.IsNull())
	{
		if (UClass* VehicleClass = VehicleOption.VehiclePawnClass.LoadSynchronous())
		{
			PreviewData = VehicleSelectionWidgetPrivate::ResolveVehiclePreviewDataFromClass(VehicleClass);
		}
	}
	else if (!VehicleOption.VehicleMesh.IsNull())
	{
		if (UStaticMesh* Mesh = VehicleOption.VehicleMesh.LoadSynchronous())
		{
			PreviewData.Mesh = Mesh;
		}
	}

	TSubclassOf<AVehicleSelectionPreviewActor> PreviewClass = VehicleOption.PreviewActorClass;
	if (!PreviewClass)
	{
		PreviewClass = AVehicleSelectionPreviewActor::StaticClass();
	}

	if (AVehicleSelectionPreviewActor* PreviewActor =
		Cast<AVehicleSelectionPreviewActor>(PlayerState.PreviewViewport->Spawn(PreviewClass)))
	{
		if (PreviewData.IsValid())
		{
			PreviewActor->SetPreviewMesh(PreviewData.Mesh);
			//PreviewActor->SetPreviewMaterials(PreviewData.Materials);
			PreviewActor->SetPreviewRelativeTransform(
				PreviewData.RelativeLocation,
				PreviewData.RelativeRotation,
				PreviewData.RelativeScale
			);
		}

		PreviewActor->SetActorLocation(VehicleOption.PreviewLocation);
		PreviewActor->SetActorRotation(VehicleOption.PreviewRotation);
		PreviewActor->SetActorScale3D(VehicleOption.PreviewScale);
		PlayerState.PreviewActor = PreviewActor;

		if (PreviewData.IsValid())
		{
			VehicleSelectionWidgetPrivate::FramePreviewViewport(
				PlayerState.PreviewViewport,
				PreviewActor->GetPreviewBounds(),
				PreviewCameraRotation,
				PreviewCameraDistanceMultiplier,
				PreviewMinimumCameraDistance
			);
		}
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
