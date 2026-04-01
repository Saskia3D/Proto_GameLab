#include "RaceEndWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerInput.h"
#include "Input/Events.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "ProtoGameLabGameInstance.h"
#include "Styling/CoreStyle.h"

namespace
{
	constexpr float ScoreTopPadding = 72.f;
	constexpr float ContinueBottomPadding = 110.f;
	constexpr float ActionTopPadding = 220.f;
	constexpr float ScoreRowSpacing = 30.f;
	constexpr float ScoreSettleDelay = 0.18f;
	constexpr float ContinuePulseSpeed = 3.6f;

	const FLinearColor PlayerHighlightColor(1.0f, 0.83f, 0.20f, 1.0f);
	const FLinearColor DefaultTextColor = FLinearColor::White;

	FString GetRetroRankText(const int32 Position)
	{
		switch (Position)
		{
		case 1:
			return TEXT("1ST");
		case 2:
			return TEXT("2ND");
		case 3:
			return TEXT("3RD");
		default:
			return FString::Printf(TEXT("%dTH"), Position);
		}
	}

	FString FormatArcadeScore(const int32 Score)
	{
		return FString::Printf(TEXT("%06d"), FMath::Max(Score, 0));
	}

	bool IsAcceptKey(const FKey& Key)
	{
		return Key == EKeys::Gamepad_FaceButton_Bottom
			|| Key == EKeys::Virtual_Gamepad_Accept.GetVirtualKey()
			|| Key == EKeys::Enter
			|| Key == EKeys::SpaceBar;
	}

	bool IsNextActionKey(const FKey& Key)
	{
		return Key == EKeys::Down
			|| Key == EKeys::S
			|| Key == EKeys::Gamepad_DPad_Down
			|| Key == EKeys::Right;
	}

	bool IsPreviousActionKey(const FKey& Key)
	{
		return Key == EKeys::Up
			|| Key == EKeys::W
			|| Key == EKeys::Gamepad_DPad_Up
			|| Key == EKeys::Left;
	}

	FLinearColor GetEntryColor(const FRaceLeaderboardEntry& Entry)
	{
		return Entry.PlayerName.StartsWith(TEXT("Joueur")) ? PlayerHighlightColor : DefaultTextColor;
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
		TextBlock->SetShadowOffset(FVector2D(1.f, 1.f));
		TextBlock->SetShadowColorAndOpacity(FLinearColor::Black);
	}

	UTextBlock* CreateTextBlock(UWidgetTree* WidgetTree, const FString& Name, const FString& Value, const FSlateFontInfo& FontInfo, const FLinearColor& Color, const ETextJustify::Type Justification)
	{
		UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *Name);
		TextBlock->SetText(FText::FromString(Value));
		//ajout de la police rétro
		FSlateFontInfo ForceRetroFont = FontInfo;
		if (UObject* FontObj = LoadObject<UObject>(nullptr, TEXT("/Game/EndMenu/fonts/PressStart2P-Regular_Font.PressStart2P-Regular_Font")))
		{
			ForceRetroFont.FontObject = FontObj;
		}
		StyleTextBlock(TextBlock, ForceRetroFont, Color, Justification);
		return TextBlock;
	}

	UButton* CreateInvisibleButton(UWidgetTree* WidgetTree, const TCHAR* ButtonName)
	{
		UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), ButtonName);
		FButtonStyle ButtonStyle = Button->GetStyle();
		ButtonStyle.Normal.DrawAs = ESlateBrushDrawType::NoDrawType;
		ButtonStyle.Hovered.DrawAs = ESlateBrushDrawType::NoDrawType;
		ButtonStyle.Pressed.DrawAs = ESlateBrushDrawType::NoDrawType;
		ButtonStyle.Disabled.DrawAs = ESlateBrushDrawType::NoDrawType;
		Button->SetStyle(ButtonStyle);
		Button->SetBackgroundColor(FLinearColor::Transparent);
		return Button;
	}
}

void URaceEndWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetIsFocusable(true);

	if (Btn_Restart)
	{
		Btn_Restart->OnClicked.RemoveDynamic(this, &URaceEndWidget::OnRestartClicked);
		Btn_Restart->OnClicked.AddDynamic(this, &URaceEndWidget::OnRestartClicked);
	}

	if (Btn_MainMenu)
	{
		Btn_MainMenu->OnClicked.RemoveDynamic(this, &URaceEndWidget::OnMainMenuClicked);
		Btn_MainMenu->OnClicked.AddDynamic(this, &URaceEndWidget::OnMainMenuClicked);
	}

	CacheThemeFont();
	LoadLeaderboardEntries();
	StyleExistingMenuWidgets();
	BuildRuntimeMenuChrome();
	BuildRuntimeLeaderboard();
	ResetAcceptInputGate(0.25f);
	SetKeyboardFocus();
}

void URaceEndWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	UpdateAcceptInputGate(InDeltaTime);
	UpdateScoreAnimation(InDeltaTime);
	UpdateContinuePrompt(InDeltaTime);
}

bool URaceEndWidget::NativeSupportsKeyboardFocus() const
{
	return true;
}

FReply URaceEndWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();

	if (IsAcceptKey(Key) && !bAcceptInputArmed)
	{
		bAcceptPressedDuringGate = true;
		return FReply::Handled();
	}

	if (CurrentPage == ERaceEndPage::Scores)
	{
		if (IsAcceptKey(Key))
		{
			if (!bCanAdvanceFromScores)
			{
				CompleteScoreAnimation();
			}
			else
			{
				ShowActionPage();
			}

			return FReply::Handled();
		}
	}
	else if (CurrentPage == ERaceEndPage::Actions)
	{
		if (IsNextActionKey(Key))
		{
			SelectedActionIndex = 1;
			ApplyActionSelectionVisuals();
			return FReply::Handled();
		}

		if (IsPreviousActionKey(Key))
		{
			SelectedActionIndex = 0;
			ApplyActionSelectionVisuals();
			return FReply::Handled();
		}

		if (IsAcceptKey(Key))
		{
			if (SelectedActionIndex == 0)
			{
				OnRestartClicked();
			}
			else
			{
				OnMainMenuClicked();
			}

			return FReply::Handled();
		}
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply URaceEndWidget::NativeOnKeyUp(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();

	if (IsAcceptKey(Key) && !bAcceptInputArmed)
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

void URaceEndWidget::OnRestartClicked()
{
	FName RestartLevel = FName("Lvl_Test_2Players");

	if (const UWorld* World = GetWorld())
	{
		if (const UProtoGameLabGameInstance* GameInstance = Cast<UProtoGameLabGameInstance>(World->GetGameInstance()))
		{
			if (!GameInstance->GetLastRaceMapName().IsNone())
			{
				RestartLevel = GameInstance->GetLastRaceMapName();
			}
		}
	}

	OpenLevelWithCleanInput(RestartLevel, true);
}

void URaceEndWidget::OnMainMenuClicked()
{
	OpenLevelWithCleanInput(FName("MainMenu"), false);
}

void URaceEndWidget::LoadLeaderboardEntries()
{
	LeaderboardEntries.Reset();

	if (const UWorld* World = GetWorld())
	{
		if (const UProtoGameLabGameInstance* GameInstance = Cast<UProtoGameLabGameInstance>(World->GetGameInstance()))
		{
			LeaderboardEntries = GameInstance->GetLastRaceLeaderboardNative();
		}
	}
}

void URaceEndWidget::CacheThemeFont()
{
	if (ThemeFontAsset.IsValid())
	{
		LoadedThemeFontObject = ThemeFontAsset.TryLoad();
	}

	if (LoadedThemeFontObject)
	{
		ThemeFontInfo = FCoreStyle::GetDefaultFontStyle("Regular", 24);
		ThemeFontInfo.FontObject = LoadedThemeFontObject;
		return;
	}

	if (WidgetTree)
	{
		WidgetTree->ForEachWidget([this](UWidget* Widget)
		{
			if (LoadedThemeFontObject)
			{
				return;
			}

			if (UTextBlock* TextBlock = Cast<UTextBlock>(Widget))
			{
				ThemeFontInfo = TextBlock->GetFont();
				LoadedThemeFontObject = const_cast<UObject*>(ThemeFontInfo.FontObject.Get());
			}
		});
	}

	if (ThemeFontInfo.Size <= 0 && ThemeFontInfo.FontObject == nullptr)
	{
		if (UObject* MonoFont = LoadObject<UObject>(nullptr, TEXT("/Game/EndMenu/fonts/PressStart2P-Regular_Font.PressStart2P-Regular_Font")))
		{
			ThemeFontInfo = FCoreStyle::GetDefaultFontStyle("Regular", 24);
			ThemeFontInfo.FontObject = MonoFont;
		}
		else
		{
			ThemeFontInfo = FCoreStyle::GetDefaultFontStyle("Regular", 24);
		}
	}
}

UCanvasPanel* URaceEndWidget::ResolveCanvasHost() const
{
	if (UCanvasPanel* CanvasHost = Cast<UCanvasPanel>(LeaderboardHost))
	{
		return CanvasHost;
	}

	if (!WidgetTree)
	{
		return nullptr;
	}

	UCanvasPanel* CanvasFallback = nullptr;
	WidgetTree->ForEachWidget([&CanvasFallback](UWidget* Widget)
	{
		if (!CanvasFallback)
		{
			CanvasFallback = Cast<UCanvasPanel>(Widget);
		}
	});

	return CanvasFallback;
}

FSlateFontInfo URaceEndWidget::MakeThemeFont(const int32 Size) const
{
	FSlateFontInfo FontInfo = ThemeFontInfo;
	if (FontInfo.Size <= 0 && FontInfo.FontObject == nullptr)
	{
		FontInfo = FCoreStyle::GetDefaultFontStyle("Regular", Size);
	}

	FontInfo.Size = Size;
	return FontInfo;
}

void URaceEndWidget::StyleExistingMenuWidgets()
{
	if (Btn_Restart)
	{
		Btn_Restart->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (Btn_MainMenu)
	{
		Btn_MainMenu->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void URaceEndWidget::BuildRuntimeMenuChrome()
{
	if (!WidgetTree)
	{
		return;
	}

	if (RuntimeMenuChromeWidget)
	{
		RuntimeMenuChromeWidget->RemoveFromParent();
		RuntimeMenuChromeWidget = nullptr;
	}

	ScorePageWidget = nullptr;
	ScoreRowsWidget = nullptr;
	ActionPageWidget = nullptr;
	ContinuePromptText = nullptr;
	RuntimeRestartButton = nullptr;
	RuntimeMainMenuButton = nullptr;
	RuntimeRestartLabel = nullptr;
	RuntimeMainMenuLabel = nullptr;

	UCanvasPanel* CanvasHost = ResolveCanvasHost();
	if (!CanvasHost)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RACE END] No canvas host found in RaceEndWidget."));
		return;
	}

	UOverlay* RootOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("RuntimeMenuChrome"));

	UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RuntimeMenuBackdrop"));
	Backdrop->SetBrushColor(FLinearColor::Black);
	if (UOverlaySlot* BackdropSlot = RootOverlay->AddChildToOverlay(Backdrop))
	{
		BackdropSlot->SetHorizontalAlignment(HAlign_Fill);
		BackdropSlot->SetVerticalAlignment(VAlign_Fill);
	}

	ScorePageWidget = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RuntimeScorePage"));
	if (UOverlaySlot* ScorePageSlot = RootOverlay->AddChildToOverlay(ScorePageWidget))
	{
		ScorePageSlot->SetHorizontalAlignment(HAlign_Center);
		ScorePageSlot->SetVerticalAlignment(VAlign_Top);
		ScorePageSlot->SetPadding(FMargin(0.f, ScoreTopPadding, 0.f, 0.f));
	}

	ContinuePromptText = CreateTextBlock(
		WidgetTree,
		TEXT("ContinuePrompt"),
		TEXT("PRESS A TO CONTINUE"),
		MakeThemeFont(30),
		PlayerHighlightColor,
		ETextJustify::Center
	);
	ContinuePromptText->SetVisibility(ESlateVisibility::Collapsed);
	if (UOverlaySlot* ContinueSlot = RootOverlay->AddChildToOverlay(ContinuePromptText))
	{
		ContinueSlot->SetHorizontalAlignment(HAlign_Center);
		ContinueSlot->SetVerticalAlignment(VAlign_Bottom);
		ContinueSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 150.f));
	}

	ActionPageWidget = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RuntimeActionPage"));
	ActionPageWidget->SetVisibility(ESlateVisibility::Collapsed);

	UTextBlock* ActionTitle = CreateTextBlock(
		WidgetTree,
		TEXT("ActionTitle"),
		TEXT("WHAT NEXT ?"),
		MakeThemeFont(50),
		DefaultTextColor,
		ETextJustify::Center
	);
	if (UVerticalBoxSlot* ActionTitleSlot = ActionPageWidget->AddChildToVerticalBox(ActionTitle))
	{
		ActionTitleSlot->SetHorizontalAlignment(HAlign_Center);
		ActionTitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 28.f));
	}

	RuntimeRestartButton = CreateInvisibleButton(WidgetTree, TEXT("RuntimeRestartButton"));
	RuntimeRestartButton->OnClicked.AddDynamic(this, &URaceEndWidget::OnRestartClicked);
	RuntimeRestartLabel = CreateTextBlock(WidgetTree, TEXT("RuntimeRestartLabel"), TEXT("RESTART"), MakeThemeFont(40), PlayerHighlightColor, ETextJustify::Center);
	RuntimeRestartButton->AddChild(RuntimeRestartLabel);
	if (UVerticalBoxSlot* RestartSlot = ActionPageWidget->AddChildToVerticalBox(RuntimeRestartButton))
	{
		RestartSlot->SetHorizontalAlignment(HAlign_Center);
		RestartSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 16.f));
	}

	RuntimeMainMenuButton = CreateInvisibleButton(WidgetTree, TEXT("RuntimeMainMenuButton"));
	RuntimeMainMenuButton->OnClicked.AddDynamic(this, &URaceEndWidget::OnMainMenuClicked);
	RuntimeMainMenuLabel = CreateTextBlock(WidgetTree, TEXT("RuntimeMainMenuLabel"), TEXT("MAIN MENU"), MakeThemeFont(40), DefaultTextColor, ETextJustify::Center);
	RuntimeMainMenuButton->AddChild(RuntimeMainMenuLabel);
	if (UVerticalBoxSlot* MainMenuSlot = ActionPageWidget->AddChildToVerticalBox(RuntimeMainMenuButton))
	{
		MainMenuSlot->SetHorizontalAlignment(HAlign_Center);
	}

	if (UOverlaySlot* ActionSlot = RootOverlay->AddChildToOverlay(ActionPageWidget))
	{
		ActionSlot->SetHorizontalAlignment(HAlign_Center);
		ActionSlot->SetVerticalAlignment(VAlign_Center);
		ActionSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 0.f));
	}

	RuntimeMenuChromeWidget = RootOverlay;

	if (UCanvasPanelSlot* ChromeSlot = CanvasHost->AddChildToCanvas(RootOverlay))
	{
		ChromeSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		ChromeSlot->SetOffsets(FMargin(0.f, 0.f, 0.f, 0.f));
		ChromeSlot->SetZOrder(100);
	}

	ApplyActionSelectionVisuals();
}

void URaceEndWidget::BuildRuntimeLeaderboard()
{
	if (!WidgetTree || !ScorePageWidget)
	{
		return;
	}

	ScorePageWidget->ClearChildren();
	AnimatedScoreRows.Reset();
	ScoreRowsWidget = nullptr;

	UTextBlock* Title = CreateTextBlock(
		WidgetTree,
		TEXT("LeaderboardTitle"),
		TEXT("HIGH SCORES"),
		MakeThemeFont(60),
		DefaultTextColor,
		ETextJustify::Center
	);
	if (UVerticalBoxSlot* TitleSlot = ScorePageWidget->AddChildToVerticalBox(Title))
	{
		TitleSlot->SetHorizontalAlignment(HAlign_Center);
		TitleSlot->SetPadding(FMargin(0.f, 100.f, 0.f, 50.f));
	}

	UHorizontalBox* HeaderRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("LeaderboardHeaderRow"));

	UTextBlock* RankHeader = CreateTextBlock(WidgetTree, TEXT("RankHeader"), TEXT("RANK"), MakeThemeFont(25), DefaultTextColor, ETextJustify::Center);
	if (UHorizontalBoxSlot* RankHeaderSlot = HeaderRow->AddChildToHorizontalBox(RankHeader))
	{
		FSlateChildSize Size;
		Size.SizeRule = ESlateSizeRule::Fill;
		Size.Value = 0.8f;
		RankHeaderSlot->SetSize(Size);
		RankHeaderSlot->SetPadding(FMargin(10.f, 0.f));
	}

	UTextBlock* NameHeader = CreateTextBlock(WidgetTree, TEXT("NameHeader"), TEXT("NAME"), MakeThemeFont(25), DefaultTextColor, ETextJustify::Center);
	if (UHorizontalBoxSlot* NameHeaderSlot = HeaderRow->AddChildToHorizontalBox(NameHeader))
	{
		FSlateChildSize Size;
		Size.SizeRule = ESlateSizeRule::Fill;
		Size.Value = 1.2f;
		NameHeaderSlot->SetSize(Size);
		NameHeaderSlot->SetPadding(FMargin(10.f, 0.f));
	}

	UTextBlock* ScoreHeader = CreateTextBlock(WidgetTree, TEXT("ScoreHeader"), TEXT("SCORE"), MakeThemeFont(25), DefaultTextColor, ETextJustify::Center);
	if (UHorizontalBoxSlot* ScoreHeaderSlot = HeaderRow->AddChildToHorizontalBox(ScoreHeader))
	{
		FSlateChildSize Size;
		Size.SizeRule = ESlateSizeRule::Fill;
		Size.Value = 1.0f;
		ScoreHeaderSlot->SetSize(Size);
		ScoreHeaderSlot->SetPadding(FMargin(10.f, 0.f));
	}

	if (UVerticalBoxSlot* HeaderSlot = ScorePageWidget->AddChildToVerticalBox(HeaderRow))
	{
		HeaderSlot->SetHorizontalAlignment(HAlign_Center);
		HeaderSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 22.f));
	}

	ScoreRowsWidget = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ScoreRowsWidget"));
	if (UVerticalBoxSlot* RowsSlot = ScorePageWidget->AddChildToVerticalBox(ScoreRowsWidget))
	{
		RowsSlot->SetHorizontalAlignment(HAlign_Center);
	}

	if (LeaderboardEntries.IsEmpty())
	{
		UTextBlock* EmptyState = CreateTextBlock(
			WidgetTree,
			TEXT("LeaderboardEmpty"),
			TEXT("NO SCORES RECORDED"),
			MakeThemeFont(25),
			DefaultTextColor,
			ETextJustify::Center
		);
		if (UVerticalBoxSlot* EmptySlot = ScoreRowsWidget->AddChildToVerticalBox(EmptyState))
		{
			EmptySlot->SetHorizontalAlignment(HAlign_Center);
		}
	}
	else
	{
		for (int32 EntryIndex = 0; EntryIndex < LeaderboardEntries.Num(); ++EntryIndex)
		{
			const FRaceLeaderboardEntry& Entry = LeaderboardEntries[EntryIndex];
			const FLinearColor RowColor = GetEntryColor(Entry);

			UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), *FString::Printf(TEXT("ScoreRow_%d"), EntryIndex));

			UTextBlock* RankText = CreateTextBlock(
				WidgetTree,
				*FString::Printf(TEXT("Rank_%d"), EntryIndex),
				GetRetroRankText(Entry.Position),
				MakeThemeFont(35),
				RowColor,
				ETextJustify::Center
			);
			if (UHorizontalBoxSlot* RankSlot = Row->AddChildToHorizontalBox(RankText))
			{
				FSlateChildSize Size;
				Size.SizeRule = ESlateSizeRule::Fill;
				Size.Value = 0.8f;
				RankSlot->SetSize(Size);
				RankSlot->SetPadding(FMargin(10.f, 0.f));
			}

			UTextBlock* NameText = CreateTextBlock(
				WidgetTree,
				*FString::Printf(TEXT("Name_%d"), EntryIndex),
				Entry.PlayerName.ToUpper(),
				MakeThemeFont(35),
				RowColor,
				ETextJustify::Center
			);
			if (UHorizontalBoxSlot* NameSlot = Row->AddChildToHorizontalBox(NameText))
			{
				FSlateChildSize Size;
				Size.SizeRule = ESlateSizeRule::Fill;
				Size.Value = 1.2f;
				NameSlot->SetSize(Size);
				NameSlot->SetPadding(FMargin(10.f, 0.f));
			}

			UTextBlock* ScoreText = CreateTextBlock(
				WidgetTree,
				*FString::Printf(TEXT("Score_%d"), EntryIndex),
				FormatArcadeScore(0),
				MakeThemeFont(35),
				RowColor,
				ETextJustify::Center
			);
			if (UHorizontalBoxSlot* ScoreSlot = Row->AddChildToHorizontalBox(ScoreText))
			{
				FSlateChildSize Size;
				Size.SizeRule = ESlateSizeRule::Fill;
				Size.Value = 1.0f;
				ScoreSlot->SetSize(Size);
				ScoreSlot->SetPadding(FMargin(10.f, 0.f));
			}

			if (UVerticalBoxSlot* RowSlot = ScoreRowsWidget->AddChildToVerticalBox(Row))
			{
				RowSlot->SetHorizontalAlignment(HAlign_Center);
				RowSlot->SetPadding(FMargin(0.f, 0.f, 0.f, ScoreRowSpacing));
			}

			FAnimatedScoreRow& AnimatedRow = AnimatedScoreRows.AddDefaulted_GetRef();
			AnimatedRow.RankText = RankText;
			AnimatedRow.NameText = NameText;
			AnimatedRow.ScoreText = ScoreText;
			AnimatedRow.RowColor = RowColor;
			AnimatedRow.PlayerName = Entry.PlayerName;
			AnimatedRow.Position = Entry.Position;
			AnimatedRow.TargetScore = Entry.Score;
			AnimatedRow.DisplayedScore = 0;
		}
	}

	ResetScoreAnimation();
}

void URaceEndWidget::ResetScoreAnimation()
{
	CurrentPage = ERaceEndPage::Scores;
	AnimatedRowIndex = 0;
	SelectedActionIndex = 0;
	RowSettleDelayRemaining = 0.f;
	ContinuePromptPulseTime = 0.f;
	bCanAdvanceFromScores = false;

	if (ScorePageWidget)
	{
		ScorePageWidget->SetVisibility(ESlateVisibility::Visible);
	}

	if (ActionPageWidget)
	{
		ActionPageWidget->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (ContinuePromptText)
	{
		ContinuePromptText->SetVisibility(ESlateVisibility::Collapsed);
		ContinuePromptText->SetRenderOpacity(1.f);
	}

	for (FAnimatedScoreRow& Row : AnimatedScoreRows)
	{
		Row.DisplayedScore = 0;
		if (Row.ScoreText)
		{
			Row.ScoreText->SetText(FText::FromString(FormatArcadeScore(0)));
			Row.ScoreText->SetColorAndOpacity(FSlateColor(Row.RowColor));
		}
	}

	if (AnimatedScoreRows.IsEmpty())
	{
		FinishScoreAnimation();
	}

	ApplyActionSelectionVisuals();
}

void URaceEndWidget::UpdateScoreAnimation(const float DeltaTime)
{
	if (CurrentPage != ERaceEndPage::Scores || bCanAdvanceFromScores)
	{
		return;
	}

	if (!AnimatedScoreRows.IsValidIndex(AnimatedRowIndex))
	{
		FinishScoreAnimation();
		return;
	}

	FAnimatedScoreRow& CurrentRow = AnimatedScoreRows[AnimatedRowIndex];

	if (CurrentRow.DisplayedScore < CurrentRow.TargetScore)
	{
		const int32 Remaining = CurrentRow.TargetScore - CurrentRow.DisplayedScore;
		const float DynamicRate = FMath::Max(1600.f, static_cast<float>(CurrentRow.TargetScore) * 2.8f);
		const int32 Step = FMath::Max(1, FMath::CeilToInt(DynamicRate * DeltaTime));
		const int32 CatchUp = FMath::Max(1, FMath::CeilToInt(static_cast<float>(Remaining) * 0.18f));

		CurrentRow.DisplayedScore = FMath::Min(CurrentRow.TargetScore, CurrentRow.DisplayedScore + FMath::Max(Step, CatchUp));
		if (CurrentRow.ScoreText)
		{
			CurrentRow.ScoreText->SetText(FText::FromString(FormatArcadeScore(CurrentRow.DisplayedScore)));
		}

		if (CurrentRow.DisplayedScore >= CurrentRow.TargetScore)
		{
			RowSettleDelayRemaining = ScoreSettleDelay;
		}

		return;
	}

	if (RowSettleDelayRemaining > 0.f)
	{
		RowSettleDelayRemaining = FMath::Max(0.f, RowSettleDelayRemaining - DeltaTime);
		if (RowSettleDelayRemaining > 0.f)
		{
			return;
		}
	}

	++AnimatedRowIndex;
	if (!AnimatedScoreRows.IsValidIndex(AnimatedRowIndex))
	{
		FinishScoreAnimation();
	}
}

void URaceEndWidget::CompleteScoreAnimation()
{
	for (FAnimatedScoreRow& Row : AnimatedScoreRows)
	{
		Row.DisplayedScore = Row.TargetScore;
		if (Row.ScoreText)
		{
			Row.ScoreText->SetText(FText::FromString(FormatArcadeScore(Row.TargetScore)));
		}
	}

	FinishScoreAnimation();
}

void URaceEndWidget::FinishScoreAnimation()
{
	bCanAdvanceFromScores = true;
	AnimatedRowIndex = AnimatedScoreRows.Num();
	RowSettleDelayRemaining = 0.f;
	ContinuePromptPulseTime = 0.f;

	if (ContinuePromptText)
	{
		ContinuePromptText->SetVisibility(ESlateVisibility::HitTestInvisible);
		ContinuePromptText->SetRenderOpacity(1.f);
	}
}

void URaceEndWidget::ShowActionPage()
{
	CurrentPage = ERaceEndPage::Actions;
	SelectedActionIndex = 0;
	ResetAcceptInputGate(0.18f);

	if (ScorePageWidget)
	{
		ScorePageWidget->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (ContinuePromptText)
	{
		ContinuePromptText->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (ActionPageWidget)
	{
		ActionPageWidget->SetVisibility(ESlateVisibility::Visible);
	}

	ApplyActionSelectionVisuals();
	SetKeyboardFocus();
}

void URaceEndWidget::ApplyActionSelectionVisuals()
{
	if (RuntimeRestartLabel)
	{
		RuntimeRestartLabel->SetColorAndOpacity(FSlateColor(SelectedActionIndex == 0 ? PlayerHighlightColor : DefaultTextColor));
	}

	if (RuntimeMainMenuLabel)
	{
		RuntimeMainMenuLabel->SetColorAndOpacity(FSlateColor(SelectedActionIndex == 1 ? PlayerHighlightColor : DefaultTextColor));
	}
}

void URaceEndWidget::UpdateContinuePrompt(const float DeltaTime)
{
	if (!ContinuePromptText || ContinuePromptText->GetVisibility() == ESlateVisibility::Collapsed)
	{
		return;
	}

	if (CurrentPage != ERaceEndPage::Scores || !bCanAdvanceFromScores)
	{
		ContinuePromptText->SetRenderOpacity(1.f);
		return;
	}

	ContinuePromptPulseTime += DeltaTime * ContinuePulseSpeed;
	const float Alpha = 0.45f + (0.55f * ((FMath::Sin(ContinuePromptPulseTime) + 1.f) * 0.5f));
	ContinuePromptText->SetRenderOpacity(Alpha);
}

void URaceEndWidget::UpdateAcceptInputGate(const float DeltaTime)
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

void URaceEndWidget::ResetAcceptInputGate(const float ArmDelaySeconds)
{
	AcceptInputArmDelayRemaining = FMath::Max(0.f, ArmDelaySeconds);
	bAcceptInputArmed = false;
	bAcceptPressedDuringGate = false;
}

void URaceEndWidget::OpenLevelWithCleanInput(const FName LevelName, const bool bPrepareGameInput)
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		PC = UGameplayStatics::GetPlayerController(this, 0);
	}

	if (PC)
	{
		if (PC->PlayerInput)
		{
			PC->PlayerInput->FlushPressedKeys();
		}

		if (bPrepareGameInput)
		{
			FInputModeGameOnly GameOnlyInputMode;
			PC->SetInputMode(GameOnlyInputMode);
		}

		PC->bShowMouseCursor = false;
	}

	UGameplayStatics::OpenLevel(this, LevelName);
}
