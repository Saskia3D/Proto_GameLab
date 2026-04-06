#include "TutorialManager.h"

#include "RaceGameMode.h"
#include "STR_RacerPawn.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"
#include "Blueprint/UserWidget.h"
#include "TimerManager.h"
#include "BuffComponent.h"
#include "InputCoreTypes.h"
#include "BuffBase.h"

ATutorialManager::ATutorialManager()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ATutorialManager::BeginPlay()
{
	Super::BeginPlay();

	RaceGameMode = Cast<ARaceGameMode>(GetWorld() ? GetWorld()->GetAuthGameMode() : nullptr);

	if (!RaceGameMode)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TUTORIAL] No RaceGameMode found"));
		return;
	}

	if (!WaitSpotP1 || !WaitSpotP2)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TUTORIAL] Wait spots are not assigned"));
	}

	bTutorialActive = true;
	bTutorialFinished = false;
	bRaceStartPending = false;
	bTutorialStarterItemsGranted = false;
	GiveTutorialStarterItemsIfPossible();
	ElapsedTutorialTime = 0.f;
	ReadyControllers.Reset();
	SkipHoldTimeByController.Reset();
	SkipHeldControllers.Reset();

	bInitialTrackRulesApplied = false;
	ApplyInitialTutorialTrackRulesIfPossible();

	CreateTutorialWidgets();

	UE_LOG(LogTemp, Warning, TEXT("[TUTORIAL] Tutorial started | Duration=%.2f"), TutorialDuration);
}

void ATutorialManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bTutorialActive && !bTutorialStarterItemsGranted)
	{
		GiveTutorialStarterItemsIfPossible();
	}

	if (bTutorialActive && !bInitialTrackRulesApplied)
	{
		ApplyInitialTutorialTrackRulesIfPossible();
	}

	if (!bTutorialActive || bTutorialFinished || bRaceStartPending)
	{
		return;
	}

	HandleSkipInputs(DeltaTime);

	ElapsedTutorialTime += DeltaTime;

	if (bForceFinishWhenTimerExpires && ElapsedTutorialTime >= TutorialDuration)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TUTORIAL] Timer expired -> ForceFinishTutorial"));
		ForceFinishTutorial();
	}
}

TArray<APlayerController*> ATutorialManager::GetLocalRaceControllers() const
{
	TArray<APlayerController*> Result;

	if (!GetWorld())
	{
		return Result;
	}

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (PC)
		{
			Result.Add(PC);
		}
	}

	return Result;
}

void ATutorialManager::SetTrackRulesEnabledForAllPawns(bool bEnabled)
{
	const TArray<APlayerController*> Controllers = GetLocalRaceControllers();

	for (APlayerController* PC : Controllers)
	{
		if (!PC)
		{
			continue;
		}

		ASTR_RacerPawn* RacerPawn = Cast<ASTR_RacerPawn>(PC->GetPawn());
		if (!RacerPawn)
		{
			continue;
		}

		RacerPawn->SetTrackRulesEnabled(bEnabled);
	}

	UE_LOG(LogTemp, Warning, TEXT("[TUTORIAL] Track rules set to %s for all pawns"),
		bEnabled ? TEXT("ENABLED") : TEXT("DISABLED"));
}

AActor* ATutorialManager::GetWaitSpotForController(AController* Controller) const
{
	APlayerController* PC = Cast<APlayerController>(Controller);
	if (!PC)
	{
		return nullptr;
	}

	ULocalPlayer* LP = PC->GetLocalPlayer();
	if (!LP)
	{
		return nullptr;
	}

	const int32 ControllerId = LP->GetControllerId();

	if (ControllerId == 0)
	{
		return WaitSpotP1;
	}

	if (ControllerId == 1)
	{
		return WaitSpotP2;
	}

	return nullptr;
}

void ATutorialManager::SendPawnToWaitSpot(ASTR_RacerPawn* RacerPawn, AActor* WaitSpot)
{
	if (!RacerPawn || !WaitSpot)
	{
		return;
	}

	RacerPawn->SetActorLocationAndRotation(
		WaitSpot->GetActorLocation(),
		WaitSpot->GetActorRotation(),
		false,
		nullptr,
		ETeleportType::TeleportPhysics
	);

	RacerPawn->ClearDrivingInputs();
	RacerPawn->SetAutoDriveEnabled(false);
	RacerPawn->SetMovementLocked(true);
	RacerPawn->SetIgnoreObstacleHits(true);
}

void ATutorialManager::CompleteTutorialForPawn(APawn* PlayerPawn)
{
	if (!bTutorialActive || bTutorialFinished || bRaceStartPending || !PlayerPawn)
	{
		return;
	}

	AController* Controller = PlayerPawn->GetController();
	if (!Controller)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TUTORIAL] CompleteTutorialForPawn ignored: no controller"));
		return;
	}

	if (ReadyControllers.Contains(Controller))
	{
		return;
	}

	ASTR_RacerPawn* RacerPawn = Cast<ASTR_RacerPawn>(PlayerPawn);
	if (!RacerPawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TUTORIAL] Pawn is not ASTR_RacerPawn: %s"), *GetNameSafe(PlayerPawn));
		return;
	}

	AActor* WaitSpot = GetWaitSpotForController(Controller);
	if (!WaitSpot)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TUTORIAL] No wait spot found for controller %s"), *GetNameSafe(Controller));
		return;
	}

	SendPawnToWaitSpot(RacerPawn, WaitSpot);
	ReadyControllers.Add(Controller);

	UE_LOG(LogTemp, Warning, TEXT("[TUTORIAL] Player ready: %s | ReadyCount=%d"),
		*GetNameSafe(Controller),
		ReadyControllers.Num());

	TryStartRaceIfEveryoneReady();
}

void ATutorialManager::ForceFinishTutorial()
{
	if (!bTutorialActive || bTutorialFinished || bRaceStartPending)
	{
		return;
	}

	const TArray<APlayerController*> Controllers = GetLocalRaceControllers();

	for (APlayerController* PC : Controllers)
	{
		if (!PC || ReadyControllers.Contains(PC))
		{
			continue;
		}

		APawn* Pawn = PC->GetPawn();
		if (Pawn)
		{
			CompleteTutorialForPawn(Pawn);
		}
	}

	TryStartRaceIfEveryoneReady();
}

void ATutorialManager::TryStartRaceIfEveryoneReady()
{
	if (!bTutorialActive || bTutorialFinished || bRaceStartPending)
	{
		return;
	}

	const TArray<APlayerController*> Controllers = GetLocalRaceControllers();
	if (Controllers.Num() == 0)
	{
		return;
	}

	for (APlayerController* PC : Controllers)
	{
		if (!PC || !ReadyControllers.Contains(PC))
		{
			return;
		}
	}

	bTutorialActive = false;
	bRaceStartPending = true;

	UE_LOG(LogTemp, Warning, TEXT("[TUTORIAL] Everyone ready -> race will start in %.2fs"), StartRaceDelaySeconds);

	if (GetWorld())
	{
		GetWorldTimerManager().SetTimer(
			StartRaceDelayHandle,
			this,
			&ATutorialManager::BeginActualRace,
			StartRaceDelaySeconds,
			false
		);
	}
}

void ATutorialManager::BeginActualRace()
{
	if (bTutorialFinished)
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[TUTORIAL UI] BeginActualRace called"));

	RemoveTutorialWidgets();

	bTutorialFinished = true;
	bRaceStartPending = false;

	const TArray<APlayerController*> Controllers = GetLocalRaceControllers();

	for (APlayerController* PC : Controllers)
	{
		if (!PC)
		{
			continue;
		}

		ASTR_RacerPawn* RacerPawn = Cast<ASTR_RacerPawn>(PC->GetPawn());
		if (!RacerPawn)
		{
			continue;
		}

		RacerPawn->SetIgnoreObstacleHits(false);
		RacerPawn->SetMovementLocked(false);
		RacerPawn->SetAutoDriveEnabled(true);
		RacerPawn->ClearDrivingInputs();
	}

	SetTrackRulesEnabledForAllPawns(true);

	if (RaceGameMode)
	{
		RaceGameMode->StartRace();
		UE_LOG(LogTemp, Warning, TEXT("[TUTORIAL] Real race started"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[TUTORIAL] Cannot start race: RaceGameMode is null"));
	}
}

void ATutorialManager::ApplyInitialTutorialTrackRulesIfPossible()
{
	if (bInitialTrackRulesApplied)
	{
		return;
	}

	const TArray<APlayerController*> Controllers = GetLocalRaceControllers();
	if (Controllers.Num() == 0)
	{
		return;
	}

	bool bAllPawnsReady = true;

	for (APlayerController* PC : Controllers)
	{
		if (!PC || !PC->GetPawn())
		{
			bAllPawnsReady = false;
			break;
		}
	}

	if (!bAllPawnsReady)
	{
		return;
	}

	SetTrackRulesEnabledForAllPawns(false);
	bInitialTrackRulesApplied = true;

	UE_LOG(LogTemp, Warning, TEXT("[TUTORIAL] Initial tutorial track rules applied"));
}

bool ATutorialManager::IsControllerReady(AController* Controller) const
{
	if (!Controller)
	{
		return false;
	}

	return ReadyControllers.Contains(Controller);
}

int32 ATutorialManager::GetExpectedPlayerCount() const
{
	return GetLocalRaceControllers().Num();
}

void ATutorialManager::CreateTutorialWidgets()
{
	RemoveTutorialWidgets();

	if (!TutorialOverlayClass || !GetWorld())
	{
		UE_LOG(LogTemp, Warning, TEXT("[TUTORIAL UI] No TutorialOverlayClass assigned"));
		return;
	}

	const TArray<APlayerController*> Controllers = GetLocalRaceControllers();

	UE_LOG(LogTemp, Warning, TEXT("[TUTORIAL UI] CreateTutorialWidgets | Controllers=%d"), Controllers.Num());

	for (APlayerController* PC : Controllers)
	{
		if (!PC || !PC->IsLocalController())
		{
			UE_LOG(LogTemp, Warning, TEXT("[TUTORIAL UI] Skipping controller %s"), *GetNameSafe(PC));
			continue;
		}

		UUserWidget* Widget = CreateWidget<UUserWidget>(PC, TutorialOverlayClass);
		if (!Widget)
		{
			UE_LOG(LogTemp, Warning, TEXT("[TUTORIAL UI] Failed to create widget for %s"), *GetNameSafe(PC));
			continue;
		}

		Widget->AddToViewport(100);
		ActiveTutorialWidgets.Add(Widget);
	}
}

void ATutorialManager::RemoveTutorialWidgets()
{
	UE_LOG(LogTemp, Warning, TEXT("[TUTORIAL UI] RemoveTutorialWidgets | Count=%d"), ActiveTutorialWidgets.Num());

	for (UUserWidget* Widget : ActiveTutorialWidgets)
	{
		if (Widget)
		{
			UE_LOG(LogTemp, Warning, TEXT("[TUTORIAL UI] Removing widget %s"), *GetNameSafe(Widget));
			Widget->RemoveFromParent();
		}
	}

	ActiveTutorialWidgets.Reset();
}

void ATutorialManager::GiveTutorialStarterItemsIfPossible()
{
	if (bTutorialStarterItemsGranted)
	{
		return;
	}

	if (!TutorialStarterBuffClass)
	{
		return;
	}

	const TArray<APlayerController*> Controllers = GetLocalRaceControllers();
	if (Controllers.Num() == 0)
	{
		return;
	}

	bool bAllPawnsReady = true;

	for (APlayerController* PC : Controllers)
	{
		if (!PC || !PC->GetPawn())
		{
			bAllPawnsReady = false;
			break;
		}
	}

	if (!bAllPawnsReady)
	{
		return;
	}

	for (APlayerController* PC : Controllers)
	{
		if (!PC)
		{
			continue;
		}

		ASTR_RacerPawn* RacerPawn = Cast<ASTR_RacerPawn>(PC->GetPawn());
		if (!RacerPawn || !RacerPawn->BuffComponent)
		{
			continue;
		}

		RacerPawn->BuffComponent->AddBuff(TutorialStarterBuffClass);
		RacerPawn->CurrentBuff = TutorialStarterBuffTypeUI;

		UE_LOG(LogTemp, Warning, TEXT("[TUTORIAL ITEM] Gave %s to %s"),
			*GetNameSafe(TutorialStarterBuffClass),
			*GetNameSafe(RacerPawn));
	}

	bTutorialStarterItemsGranted = true;
}

FText ATutorialManager::GetSkipStatusText(AController* Controller) const
{
	if (bRaceStartPending)
	{
		return FText::FromString(TEXT("Starting race..."));
	}

	if (!bTutorialActive || bTutorialFinished)
	{
		return FText::GetEmpty();
	}

	if (IsControllerReady(Controller))
	{
		return FText::FromString(TEXT("Ready - waiting for other player..."));
	}

	return FText::FromString(TEXT("D-Pad Up : Skip Tutorial"));
}

bool ATutorialManager::IsSkipInputDown(APlayerController* PC) const
{
	if (!PC)
	{
		return false;
	}

	return PC->IsInputKeyDown(EKeys::Gamepad_DPad_Up);
}

void ATutorialManager::HandleSkipInputs(float DeltaTime)
{
	const TArray<APlayerController*> Controllers = GetLocalRaceControllers();

	for (APlayerController* PC : Controllers)
	{
		if (!PC)
		{
			continue;
		}

		if (ReadyControllers.Contains(PC))
		{
			SkipHoldTimeByController.Remove(PC);
			continue;
		}

		if (IsSkipInputDown(PC))
		{
			float& HeldTime = SkipHoldTimeByController.FindOrAdd(PC);
			HeldTime += DeltaTime;

			if (HeldTime >= SkipHoldDuration)
			{
				UE_LOG(LogTemp, Warning, TEXT("[TUTORIAL] Skip hold completed by %s"), *GetNameSafe(PC));

				if (APawn* Pawn = PC->GetPawn())
				{
					CompleteTutorialForPawn(Pawn);
				}

				SkipHoldTimeByController.Remove(PC);
			}
		}
		else
		{
			SkipHoldTimeByController.Remove(PC);
		}
	}
}