#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BuffType.h"
#include "TutorialManager.generated.h"

class ARaceGameMode;
class ASTR_RacerPawn;
class UUserWidget;
class APlayerController;
class UBuffBase;

UCLASS()
class PROTOGAMELAB_API ATutorialManager : public AActor
{
	GENERATED_BODY()

public:
	ATutorialManager();

	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void CompleteTutorialForPawn(APawn* PlayerPawn);

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void ForceFinishTutorial();

	UFUNCTION(BlueprintPure, Category = "Tutorial")
	bool IsTutorialActive() const { return bTutorialActive; }

	UFUNCTION(BlueprintPure, Category = "Tutorial")
	float GetRemainingTutorialTime() const
	{
		return FMath::Max(0.f, TutorialDuration - ElapsedTutorialTime);
	}

	UFUNCTION(BlueprintPure, Category = "Tutorial|UI")
	bool IsControllerReady(AController* Controller) const;

	UFUNCTION(BlueprintPure, Category = "Tutorial|UI")
	int32 GetReadyPlayerCount() const { return ReadyControllers.Num(); }

	UFUNCTION(BlueprintPure, Category = "Tutorial|UI")
	int32 GetExpectedPlayerCount() const;

	UFUNCTION(BlueprintPure, Category = "Tutorial|UI")
	bool ShouldShowSkipPrompt() const
	{
		return bTutorialActive && !bTutorialFinished;
	}

	UFUNCTION(BlueprintPure, Category = "Tutorial|UI")
	bool IsRaceStartPending() const
	{
		return bRaceStartPending;
	}

	UFUNCTION(BlueprintPure, Category = "Tutorial|UI")
	FText GetSkipStatusText(AController* Controller) const;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial")
	float TutorialDuration = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial")
	float StartRaceDelaySeconds = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial")
	bool bForceFinishWhenTimerExpires = true;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Tutorial|WaitSpots")
	TObjectPtr<AActor> WaitSpotP1 = nullptr;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Tutorial|WaitSpots")
	TObjectPtr<AActor> WaitSpotP2 = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial|UI")
	TSubclassOf<UUserWidget> TutorialOverlayClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial|Skip")
	float SkipHoldDuration = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial|Items")
	TSubclassOf<UBuffBase> TutorialStarterBuffClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial|Items")
	E_BuffType TutorialStarterBuffTypeUI;

private:
	UPROPERTY()
	TObjectPtr<ARaceGameMode> RaceGameMode = nullptr;

	UPROPERTY()
	TSet<TObjectPtr<AController>> ReadyControllers;

	UPROPERTY()
	FTimerHandle StartRaceDelayHandle;

	UPROPERTY()
	TArray<TObjectPtr<UUserWidget>> ActiveTutorialWidgets;

	UPROPERTY()
	TSet<TObjectPtr<AController>> SkipHeldControllers;

	UPROPERTY()
	TMap<TObjectPtr<AController>, float> SkipHoldTimeByController;

	void HandleSkipInputs(float DeltaTime);
	bool IsSkipInputDown(APlayerController* PC) const;

	void CreateTutorialWidgets();
	void RemoveTutorialWidgets();

	bool bTutorialActive = false;
	bool bTutorialFinished = false;
	bool bRaceStartPending = false;
	float ElapsedTutorialTime = 0.f;

	TArray<APlayerController*> GetLocalRaceControllers() const;
	AActor* GetWaitSpotForController(AController* Controller) const;
	void SendPawnToWaitSpot(ASTR_RacerPawn* RacerPawn, AActor* WaitSpot);
	void TryStartRaceIfEveryoneReady();
	void BeginActualRace();
	void SetTrackRulesEnabledForAllPawns(bool bEnabled);
	bool bInitialTrackRulesApplied = false;

	void ApplyInitialTutorialTrackRulesIfPossible();

	bool bTutorialStarterItemsGranted = false;

	void GiveTutorialStarterItemsIfPossible();
};
