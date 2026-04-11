#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RaceLeaderboardEntry.h"
#include "RaceEndWidget.generated.h"

class UButton;
class UCanvasPanel;
class UPanelWidget;
class UTextBlock;
class UTexture2D;
class UVerticalBox;
class UWidget;
struct FGeometry;
class FReply;
struct FKeyEvent;

UENUM()
enum class ERaceEndPage : uint8
{
	Scores,
	Actions
};

USTRUCT()
struct FAnimatedScoreRow
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> RankText = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> NameText = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ScoreText = nullptr;

	FLinearColor RowColor = FLinearColor::White;
	FString PlayerName;
	int32 Position = 0;
	int32 TargetScore = 0;
	int32 DisplayedScore = 0;
};

UCLASS()
class PROTOGAMELAB_API URaceEndWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "RaceEnd|State")
	int32 GetSelectedActionIndex() const;

	UFUNCTION(BlueprintPure, Category = "RaceEnd|State")
	bool IsShowingActionPage() const;

	UFUNCTION(BlueprintImplementableEvent, Category = "RaceEnd|Events")
	void OnActionSelectionChanged(int32 NewIndex);

	UFUNCTION(BlueprintImplementableEvent, Category = "RaceEnd|Events")
	void OnRestartActivated();

	UFUNCTION(BlueprintImplementableEvent, Category = "RaceEnd|Events")
	void OnMainMenuActivated();

	UFUNCTION(BlueprintImplementableEvent, Category = "RaceEnd|Events")
	void OnNewMapActivated();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual bool NativeSupportsKeyboardFocus() const override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnKeyUp(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	UPROPERTY(BlueprintReadOnly, Category = "Leaderboard")
	TArray<FRaceLeaderboardEntry> LeaderboardEntries;

	UPROPERTY(meta = (BindWidget))
	UButton* Btn_Restart = nullptr;

	UPROPERTY(meta = (BindWidget))
	UButton* Btn_MainMenu = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UPanelWidget* LeaderboardHost = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Theme", meta = (AllowedClasses = "/Script/Engine.Font,/Script/Engine.FontFace"))
	FSoftObjectPath ThemeFontAsset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Theme")
	TSoftObjectPtr<UTexture2D> MenuLogoTexture;

	UFUNCTION()
	void OnRestartClicked();

	UFUNCTION()
	void OnMainMenuClicked();

	UFUNCTION()
	void OnTryNewMapClicked();

private:
	UPROPERTY(Transient)
	TObjectPtr<UWidget> RuntimeMenuChromeWidget = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> ScorePageWidget = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> ScoreRowsWidget = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> ActionPageWidget = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ContinuePromptText = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UButton> RuntimeRestartButton = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UButton> RuntimeMainMenuButton = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UButton> RuntimeNewMapButton = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> RuntimeRestartLabel = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> RuntimeMainMenuLabel = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> RuntimeNewMapLabel = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UObject> LoadedThemeFontObject = nullptr;

	UPROPERTY(Transient)
	TArray<FAnimatedScoreRow> AnimatedScoreRows;

	FSlateFontInfo ThemeFontInfo;
	ERaceEndPage CurrentPage = ERaceEndPage::Scores;
	int32 AnimatedRowIndex = 0;
	int32 SelectedActionIndex = 0;
	float RowSettleDelayRemaining = 0.f;
	float ContinuePromptPulseTime = 0.f;
	float AcceptInputArmDelayRemaining = 0.f;
	bool bCanAdvanceFromScores = false;
	bool bAcceptInputArmed = false;
	bool bAcceptPressedDuringGate = false;

	void LoadLeaderboardEntries();
	void CacheThemeFont();
	void BuildRuntimeMenuChrome();
	void BuildRuntimeLeaderboard();
	UCanvasPanel* ResolveCanvasHost() const;
	FSlateFontInfo MakeThemeFont(int32 Size) const;
	void StyleExistingMenuWidgets();
	void ResetScoreAnimation();
	void UpdateScoreAnimation(float DeltaTime);
	void CompleteScoreAnimation();
	void FinishScoreAnimation();
	void ShowActionPage();
	void ApplyActionSelectionVisuals();
	void UpdateContinuePrompt(float DeltaTime);
	void UpdateAcceptInputGate(float DeltaTime);
	void ResetAcceptInputGate(float ArmDelaySeconds);
	void OpenLevelWithCleanInput(FName LevelName, bool bPrepareGameInput);
};
