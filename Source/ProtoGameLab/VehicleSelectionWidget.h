#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Fonts/SlateFontInfo.h"
#include "Input/Reply.h"
#include "UObject/SoftObjectPath.h"
#include "VehicleSelectionWidget.generated.h"

class AVehicleSelectionPreviewActor;
class UBorder;
class UButton;
class UCanvasPanel;
class UTextBlock;
class UViewport;
class ASTR_RacerPawn;
struct FGeometry;
struct FKeyEvent;
class UStaticMesh;

USTRUCT(BlueprintType)
struct FVehicleSelectionOption
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vehicle")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vehicle", meta = (AllowedClasses = "/Script/Engine.StaticMesh"))
	TSoftObjectPtr<UStaticMesh> VehicleMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vehicle")
	TSubclassOf<AVehicleSelectionPreviewActor> PreviewActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vehicle")
	FVector PreviewLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vehicle")
	FRotator PreviewRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vehicle")
	FVector PreviewScale = FVector(1.f, 1.f, 1.f);
};

USTRUCT()
struct FVehicleSelectionPlayerState
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TObjectPtr<UBorder> CardBorder = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UViewport> PreviewViewport = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> PlayerLabel = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> VehicleLabel = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StatusLabel = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ConfirmLabel = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UButton> LeftButton = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UButton> RightButton = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UButton> ConfirmButton = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<AVehicleSelectionPreviewActor> PreviewActor = nullptr;

	int32 SelectedIndex = 0;
	bool bConfirmed = false;
};

UCLASS()
class PROTOGAMELAB_API UVehicleSelectionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Navigation")
	void SetTargetLevelName(FName InTargetLevelName);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual bool NativeSupportsKeyboardFocus() const override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnKeyUp(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vehicles")
	TArray<FVehicleSelectionOption> VehicleOptions;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Navigation")
	FName TargetLevelName = FName(TEXT("Lvl_Test_2Players"));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Theme", meta = (AllowedClasses = "/Script/Engine.Font,/Script/Engine.FontFace"))
	FSoftObjectPath ThemeFontAsset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Theme")
	FLinearColor BackgroundTint = FLinearColor::Black;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Theme")
	FLinearColor PanelTint = FLinearColor(0.08f, 0.12f, 0.18f, 0.96f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Theme")
	FLinearColor PreviewBackgroundTint = FLinearColor::Black;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Copy")
	FText TitleText = FText::FromString(TEXT("SELECT YOUR RIDE"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Copy")
	FText SubtitleText = FText::FromString(TEXT("BOTH PLAYERS MUST LOCK THEIR CAR BEFORE THE TUTORIAL STARTS"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Copy")
	FText PlayerOneTitleText = FText::FromString(TEXT("PLAYER 1"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Copy")
	FText PlayerTwoTitleText = FText::FromString(TEXT("PLAYER 2"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Copy")
	FText WaitingStatusText = FText::FromString(TEXT("CHOOSE A CAR AND PRESS A"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Copy")
	FText ReadyStatusText = FText::FromString(TEXT("READY FOR TUTORIAL"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Copy")
	FText ConfirmPromptText = FText::FromString(TEXT("PRESS A TO CONFIRM"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Copy")
	FText LockedInText = FText::FromString(TEXT("LOCKED IN"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Copy")
	FText BottomInstructionTextContent = FText::FromString(TEXT("P1: Q / D / SPACE   |   P2: LEFT / RIGHT / ENTER   |   GAMEPAD: DPAD + A"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Copy")
	FText LeftButtonText = FText::FromString(TEXT("<"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Copy")
	FText RightButtonText = FText::FromString(TEXT(">"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout")
	FMargin RootPadding = FMargin(90.f, 72.f, 90.f, 36.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout")
	FMargin TitlePadding = FMargin(0.f, 0.f, 0.f, 8.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout")
	FMargin SubtitlePadding = FMargin(0.f, 0.f, 0.f, 18.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout")
	FMargin PlayersRowPadding = FMargin(0.f, 18.f, 0.f, 18.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout")
	float CardSpacing = 12.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout")
	FMargin CardPadding = FMargin(24.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout")
	FMargin PlayerLabelPadding = FMargin(0.f, 0.f, 0.f, 18.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout")
	FMargin PreviewPadding = FMargin(0.f, 6.f, 0.f, 34.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout")
	FMargin NavigationPadding = FMargin(0.f, 0.f, 0.f, 24.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout")
	float NavigationButtonSpacing = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout")
	FMargin VehicleNamePadding = FMargin(0.f, 0.f, 0.f, 16.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout")
	FMargin StatusPadding = FMargin(0.f, 0.f, 0.f, 28.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout")
	FMargin BottomInstructionPadding = FMargin(0.f, 10.f, 0.f, 0.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout")
	FVector2D PreviewSize = FVector2D(520.f, 340.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview")
	FVector PreviewCameraLocation = FVector(340.f, 0.f, 110.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview")
	FRotator PreviewCameraRotation = FRotator(-10.f, 180.f, 0.f);

private:
	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> RuntimeRootCanvas = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> BottomInstructionText = nullptr;

	UPROPERTY(Transient)
	TArray<FVehicleSelectionPlayerState> PlayerStates;

	UPROPERTY(Transient)
	TObjectPtr<UObject> LoadedThemeFontObject = nullptr;

	FSlateFontInfo ThemeFontInfo;
	float AcceptInputArmDelayRemaining = 0.f;
	bool bAcceptInputArmed = false;
	bool bAcceptPressedDuringGate = false;
	bool bPendingInitialPreviewSetup = false;

	UFUNCTION()
	void OnPlayerOneLeftClicked();

	UFUNCTION()
	void OnPlayerOneRightClicked();

	UFUNCTION()
	void OnPlayerOneConfirmClicked();

	UFUNCTION()
	void OnPlayerTwoLeftClicked();

	UFUNCTION()
	void OnPlayerTwoRightClicked();

	UFUNCTION()
	void OnPlayerTwoConfirmClicked();

	void InitializeDefaultVehicleOptions();
	void CacheThemeFont();
	FSlateFontInfo MakeThemeFont(int32 Size) const;
	void BuildRuntimeWidget();
	void EnsureTwoLocalPlayers();
	void ConfigureSelectionInput() const;
	void FocusAllUsers();
	void SaveSelectionsToGameInstance() const;
	void TryAdvanceToTargetLevel();
	void FinalizeInitialPreviewSetup();
	void ConfigurePreviewViewport(int32 PlayerIndex);
	void SilenceMainMenuWorldActors();
	void HandleSelectionChange(int32 PlayerIndex, int32 Direction);
	void HandleConfirm(int32 PlayerIndex);
	void UpdatePlayerCard(int32 PlayerIndex);
	void RefreshPreview(int32 PlayerIndex);
	void ResetAcceptInputGate(float ArmDelaySeconds);
	void UpdateAcceptInputGate(float DeltaTime);
	int32 ResolvePlayerIndexFromKey(const FKeyEvent& InKeyEvent) const;
	bool IsConfirmKeyForPlayer(int32 PlayerIndex, const FKey& Key) const;
	bool IsLeftKeyForPlayer(int32 PlayerIndex, const FKey& Key) const;
	bool IsRightKeyForPlayer(int32 PlayerIndex, const FKey& Key) const;
	void DestroyPreviewActor(int32 PlayerIndex);
};
