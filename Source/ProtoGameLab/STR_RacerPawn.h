#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputActionValue.h"
#include "STR_RacerPawn.generated.h"

// Forward declarations
class UCapsuleComponent;
class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class UPaperSpriteComponent;
class UPaperSprite;
class UBuffComponent;
class ATrackSplineActor;

UCLASS()
class PROTOGAMELAB_API ASTR_RacerPawn : public APawn
{
	GENERATED_BODY()

public:
	ASTR_RacerPawn();

protected:
	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCapsuleComponent* CapsuleComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UPaperSpriteComponent* SpriteComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USpringArmComponent* SpringArmComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCameraComponent* CameraComp;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* SteerAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* BrakeAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* ItemAction;

	/*UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* AccelerateAction; */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float MaxSpeed = 1245.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float AccelerationRate = 400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float BrakingDeceleration = 800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float CoastingDeceleration = 220.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Steering")
	float MaxTurnRate = 165.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Steering")
	float MinTurnRateAtMaxSpeed = 130.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Steering")
	float SteeringInterpSpeed = 2.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Steering")
	float MinSpeedToTurn = 40.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buff")
	UBuffComponent* BuffComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprites")
	TArray<UPaperSprite*> CarSprites;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Track|UI")
	float TeleportFeedbackTimer = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Track|UI")
	float TeleportFeedbackDuration = 1.75f;

	UFUNCTION(BlueprintCallable, Category = "Movement")
	float GetCurrentSpeed() const { return CurrentSpeed; }

	//fonctions IA
	UFUNCTION(BlueprintCallable, Category = "AI")
	void SetSteeringInput(float InSteer);

	UFUNCTION(BlueprintCallable, Category = "AI")
	void SetAutoDriveEnabled(bool bShouldAutoDrive);

	UFUNCTION(BlueprintCallable, Category = "AI")
	void SetDriftButtonHeld(bool bShouldHoldDrift);

	UFUNCTION(BlueprintCallable, Category = "AI")
	void ClearDrivingInputs();

	UFUNCTION(BlueprintCallable, Category = "AI")
	void TriggerItemUse();

	UFUNCTION(BlueprintCallable, Category = "AI")
	bool HasBuff() const;

	UFUNCTION(BlueprintPure, Category = "UI|Drift")
	bool IsDrifting() const { return bIsDrifting; }

	UFUNCTION(BlueprintPure, Category = "UI|Drift")
	float GetDriftCharge() const { return DriftCharge; }

	UFUNCTION(BlueprintPure, Category = "UI|Drift")
	float GetDriftChargeNormalized() const
	{
		return FMath::Clamp(DriftCharge / 100.f, 0.f, 1.f);
	}

	UFUNCTION(BlueprintPure, Category = "UI|Drift")
	bool IsDriftBoostActive() const { return ActiveBoostTimer > 0.f; }

	UFUNCTION(BlueprintPure, Category = "Track|OffTrack")
	bool IsOffTrack() const { return bIsOffTrack; }

	UFUNCTION(BlueprintPure, Category = "Track|OffTrack")
	bool IsOffTrackPenaltyActive() const { return bOffTrackPenaltyActive; }

	UFUNCTION(BlueprintPure, Category = "Track|UI")
	float GetOffTrackTime() const { return OffTrackTime; }

	UFUNCTION(BlueprintPure, Category = "Track|UI")
	float GetRemainingTimeBeforeTeleport() const
	{
		if (!bIsOffTrack)
		{
			return 0.f;
		}

		return FMath::Max(0.f, OffTrackTeleportDelay - OffTrackTime);
	}

	UFUNCTION(BlueprintPure, Category = "Track|UI")
	int32 GetTeleportCountdownSeconds() const
	{
		return FMath::CeilToInt(GetRemainingTimeBeforeTeleport());
	}

	UFUNCTION(BlueprintPure, Category = "Track|UI")
	bool IsTeleportFeedbackActive() const
	{
		return TeleportFeedbackTimer > 0.f;
	}

	UFUNCTION(BlueprintPure, Category = "Track|WrongWay")
	bool IsGoingWrongWay() const { return bIsGoingWrongWay; }

	UFUNCTION(BlueprintPure, Category = "Track|WrongWay")
	bool IsWrongWayWarningActive() const { return bWrongWayWarningActive; }

	UFUNCTION(BlueprintPure, Category = "Track|WrongWay")
	float GetWrongWayTime() const { return WrongWayTime; }

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Track|WrongWay")
	bool bIsGoingWrongWay = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Track|WrongWay")
	bool bWrongWayWarningActive = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Track|WrongWay")
	float WrongWayTime = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Track|WrongWay")
	float WrongWayDetectionDelay = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Track|WrongWay")
	float WrongWayMinSpeed = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Track|WrongWay")
	float WrongWayDotThreshold = -0.35f;

protected:
	float CurrentSpeed = 0.f;
	float TargetSteeringInput = 0.f;
	float CurrentSteeringInput = 0.f;

	bool bIsDriftButtonHeld = false;
	bool bAutoDriveEnabled = true;

	void Steer(const FInputActionValue& Value);
	void StartDrift(const FInputActionValue& Value);
	void StopDrift(const FInputActionValue& Value);
	void UseItem(const FInputActionValue& Value);
	/*
	void StartAccelerate(const FInputActionValue& Value);
	void StopAccelerate(const FInputActionValue& Value); */

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|Drift")
	bool bIsDrifting = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|Drift")
	FVector MoveVelocity = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Drift")
	float MinSpeedToStartDrift = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Drift")
	float NormalGrip = 17.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Drift")
	float DriftGrip = 12.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Drift")
	float DriftSpeedLossPerSecond = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Drift")
	float DriftAccelMultiplier = 0.28f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Drift")
	float DriftDirectionSwitchThreshold = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Drift")
	float DriftSteerThreshold = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Drift")
	float MinDriftSpeed = 100.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|Drift")
	float CurrentDriftAngle = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Drift")
	float MaxDriftAngle = 25.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Drift")
	float DriftAngleInterpSpeed = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Drift")
	float DriftTurnRateMultiplier = 9.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Drift")
	float DriftBaseAutoSteer = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Drift")
	float DriftSteerSameDirectionMultiplier = 0.20f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Drift")
	float DriftSteerOppositeDirectionMultiplier = 0.05f;

	int32 DriftDirection = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mouvement|DriftBoost")
	int32 DriftChargeDirection = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|DriftBoost")
	float DriftCharge = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|DriftBoost")
	float DriftHeldTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|DriftBoost")
	bool bWasDriftingLastFrame = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|DriftBoost")
	float ActiveBoostTimer = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|DriftBoost")
	float ActiveBoostBonusSpeed = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|DriftBoost")
	FVector LastTravelDir = FVector::ForwardVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|DriftBoost")
	float MinBoostSpeed = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|DriftBoost")
	float MinBoostSlipAngleDeg = 6.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|DriftBoost")
	float MaxUsefulSlipAngleDeg = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|DriftBoost")
	float MinBoostSteerInput = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|DriftBoost")
	float MinTravelYawRateDeg = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|DriftBoost")
	float DriftChargeRate = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|DriftBoost")
	float DriftChargeDecayRate = 12.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|DriftBoost")
	float CounterSteerDecayRate = 40.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|DriftBoost")
	float MinChargeTimeForBoost = 0.20f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|DriftBoost")
	float SmallBoostCharge = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|DriftBoost")
	float MediumBoostCharge = 25.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|DriftBoost")
	float LargeBoostCharge = 40.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|DriftBoost")
	float SmallBoostBonusSpeed = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|DriftBoost")
	float MediumBoostBonusSpeed = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|DriftBoost")
	float LargeBoostBonusSpeed = 900.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|DriftBoost")
	float SmallBoostDuration = 0.70f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|DriftBoost")
	float MediumBoostDuration = 1.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|DriftBoost")
	float LargeBoostDuration = 2.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mouvement|Drift")
	bool bAllowDrift = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|DriftBoost")
	float DriftBoostInvalidationThreshold = 0.1f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|DriftBoost")
	bool bDriftBoostStillValid = false;

	//Detection offtrack
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Track|OffTrack")
	bool bIsOffTrack = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Track|OffTrack")
	bool bOffTrackPenaltyActive = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Track|OffTrack")
	float OffTrackTime = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Track|OffTrack")
	float OffTrackDetectionMargin = 35.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Track|OffTrack")
	float OffTrackPenaltyDelay = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Track|OffTrack")
	float OffTrackMaxSpeedMultiplier = 0.50f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Track|OffTrack")
	float OffTrackAccelerationMultiplier = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Track|OffTrack")
	float OffTrackExtraDeceleration = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Track|OffTrack")
	float OffTrackTeleportDelay = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Track|OffTrack")
	float RecoveryHeightOffset = 15.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Track|OffTrack")
	float RecoverySpeedAfterTeleport = 350.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Track|OffTrack")
	TObjectPtr<ATrackSplineActor> TrackSplineActor = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Track|Recovery")
	bool bHasSafeRecoveryPoint = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Track|Recovery")
	FVector LastSafeLocation = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Track|Recovery")
	FVector LastSafeForward = FVector::ForwardVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Track|Recovery")
	float LastSafeSpeed = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Track|Recovery")
	float SafeRecoveryTrackRatio = 0.72f;

	void UpdateSafeRecoveryPoint();

	void UpdateOffTrackState(float DeltaTime);

	void TeleportBackToTrack();

	float GetSignedSlipAngleDegrees() const;
	float GetTravelYawRateDegrees(float DeltaTime, const FVector& CurrentTravelDir) const;
	void StartDriftBoost(float BonusSpeed, float Duration);

	void UpdateWrongWayState(float DeltaTime);
};