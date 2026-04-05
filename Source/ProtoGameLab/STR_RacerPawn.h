#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputActionValue.h"
#include "BuffType.h"
#include "Components/StaticMeshComponent.h"
#include "STR_RacerPawn.generated.h"

// Forward declarations
class UBoxComponent;
class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class UPaperSpriteComponent;
class UPaperSprite;
class UBuffComponent;
class URaceMinimapWidget;
class ATrackSplineActor;
class UNiagaraSystem;

class ARaceGameMode;

enum class EDriftTuningParam : uint8
{
	TurnRateMultiplier,
	BaseAutoSteer,
	SameDirectionMultiplier,
	OppositeDirectionMultiplier,
	DriftGrip,
	MaxDriftAngle,
	DriftSpeedLossPerSecond,
	DriftAccelMultiplier,
	MinSpeedToStartDrift,
	Count
};

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
	UBoxComponent* BoxComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	//UPaperSpriteComponent* SpriteComp;
	UStaticMeshComponent* CarMesh;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float MaxSpeed = 1600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float AccelerationRate = 725.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float BrakingDeceleration = 800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float CoastingDeceleration = 220.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Steering")
	float MaxTurnRate = 165.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Steering")
	float MinTurnRateAtMaxSpeed = 130.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Steering")
	float SteeringInterpSpeed = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Steering")
	float MinSpeedToTurn = 40.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buff")
	UBuffComponent* BuffComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprites")
	TArray<UPaperSprite*> CarSprites;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Track|UI")
	float TeleportFeedbackTimer = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Minimap")
	TSubclassOf<URaceMinimapWidget> MinimapWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Track|UI")
	float TeleportFeedbackDuration = 1.75f;

	UFUNCTION(BlueprintCallable, Category = "Movement")
	float GetCurrentSpeed() const { return CurrentSpeed; }

	//Ref Pawns
	UPROPERTY(BlueprintReadWrite, Category = "Buff")
	E_BuffType CurrentBuff;

	// Knockback
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision")
	float KnockbackStrength = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision")
	float KnockbackVerticalBoost = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision")
	float HitCooldown = 0.35f;

	float LastHitTime = -100.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|DriftExit")
	bool bPostDriftRecoveryActive = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|DriftExit")
	float PostDriftRecoveryTimer = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|DriftExit")
	int32 LastDriftDirection = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|DriftExit")
	float PostDriftRecoveryDuration = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|DriftExit")
	float PostDriftSteerSameDirectionMultiplier = 0.30f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|DriftExit")
	float PostDriftSteerOppositeDirectionMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|DriftExit")
	float PostDriftRotationBlendSpeed = 9.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|DriftBoost")
	float CurrentBoostExtraSpeed = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|DriftBoost")
	float BoostDecaySpeed = 1200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Drift")
	float DriftDirectionChangeCooldown = 0.12f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|Drift")
	float DriftDirectionChangeCooldownTimer = 0.f;

	UFUNCTION(BlueprintCallable, Category = "Collision")
	void SetIgnoreObstacleHits(bool bShouldIgnore);

	UFUNCTION(BlueprintPure, Category = "Collision")
	bool IsIgnoringObstacleHits() const { return bIgnoreObstacleHits; }

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Track|Rules")
	bool bTrackRulesEnabled = true;

	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, FVector NormalImpulse,
		const FHitResult& Hit);

	float HitStunTimer = 0.f;

	UPROPERTY(EditAnywhere, Category = "VFX")
	UNiagaraSystem* ImpactEffect;

	UPROPERTY(EditAnywhere, Category = "VFX")
	UNiagaraSystem* BoostTrail;

	//fonctions IA
	UFUNCTION(BlueprintCallable, Category = "AI")
	void SetSteeringInput(float InSteer);

	UFUNCTION(BlueprintCallable, Category = "AI")
	void ClearDrivingInputs();

	UFUNCTION(BlueprintCallable, Category = "AI")
	void TriggerItemUse();

	UFUNCTION(BlueprintCallable, Category = "AI")
	bool HasBuff() const;

	UFUNCTION(BlueprintCallable, Category = "AI")
	void SetAutoDriveEnabled(bool bShouldAutoDrive);

	UFUNCTION(BlueprintCallable, Category = "AI")
	void SetDriftButtonHeld(bool bShouldHoldDrift);

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

	UFUNCTION(BlueprintCallable, Category = "Tutorial|Movement")
	void SetMovementLocked(bool bLocked);

	UFUNCTION(BlueprintPure, Category = "Tutorial|Movement")
	bool IsMovementLocked() const { return bMovementLocked; }

	UFUNCTION(BlueprintCallable, Category = "Track|Rules")
	void SetTrackRulesEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "Track|Rules")
	bool AreTrackRulesEnabled() const { return bTrackRulesEnabled; }

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Track|WrongWay")
	bool bIsGoingWrongWay = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Track|WrongWay")
	bool bWrongWayWarningActive = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Track|WrongWay")
	float WrongWayTime = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Track|WrongWay")
	float WrongWayDetectionDelay = 0.6f;

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
	bool bMovementLocked = false;

	void Steer(const FInputActionValue& Value);
	void StartDrift(const FInputActionValue& Value);
	void StopDrift(const FInputActionValue& Value);
	void UseItem(const FInputActionValue& Value);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|Drift")
	bool bIsDrifting = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|Drift")
	FVector MoveVelocity = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Drift")
	float MinSpeedToStartDrift = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Drift")
	float NormalGrip = 17.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Drift")
	float DriftGrip = 50.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Drift")
	float DriftSpeedLossPerSecond = 40.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Drift")
	float DriftAccelMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Drift")
	float DriftDirectionSwitchThreshold = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Drift")
	float DriftSteerThreshold = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Drift")
	float MinDriftSpeed = 100.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|Drift")
	float CurrentDriftAngle = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Drift")
	float MaxDriftAngle = 15.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Drift")
	float DriftAngleInterpSpeed = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Drift")
	float DriftTurnRateMultiplier = 1.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Drift")
	float DriftBaseAutoSteer = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Drift")
	float DriftSteerSameDirectionMultiplier = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Drift")
	float DriftSteerOppositeDirectionMultiplier = 1.f;

	int32 DriftDirection = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|DriftBoost")
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
	float DriftChargeRate = 150.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|DriftBoost")
	float DriftChargeDecayRate = 12.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|DriftBoost")
	float CounterSteerDecayRate = 40.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|DriftBoost")
	float MinChargeTimeForBoost = 0.20f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|DriftBoost")
	float SmallBoostCharge = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|DriftBoost")
	float MediumBoostCharge = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|DriftBoost")
	float LargeBoostCharge = 40.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|DriftBoost")
	float SmallBoostBonusSpeed = 370.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|DriftBoost")
	float MediumBoostBonusSpeed = 575.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|DriftBoost")
	float LargeBoostBonusSpeed = 780.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|DriftBoost")
	float SmallBoostDuration = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|DriftBoost")
	float MediumBoostDuration = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|DriftBoost")
	float LargeBoostDuration = 1.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mouvement|Drift")
	bool bAllowDrift = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|DriftBoost")
	float DriftBoostInvalidationThreshold = 0.1f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|DriftBoost")
	bool bDriftBoostStillValid = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collision")
	bool bIgnoreObstacleHits = false;

	//Detection offtrack
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Track|OffTrack")
	bool bIsOffTrack = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Track|OffTrack")
	bool bOffTrackPenaltyActive = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Track|OffTrack")
	float OffTrackTime = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Track|OffTrack")
	float OffTrackDetectionMargin = 635.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Track|OffTrack")
	float OffTrackPenaltyDelay = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Track|OffTrack")
	float OffTrackMaxSpeedMultiplier = 0.38f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Track|OffTrack")
	float OffTrackAccelerationMultiplier = 0.22f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Track|OffTrack")
	float OffTrackExtraDeceleration = 350.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Track|OffTrack")
	float OffTrackTeleportDelay = 3.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Track|OffTrack")
	float RecoveryHeightOffset = 15.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Track|OffTrack")
	float RecoverySpeedAfterTeleport = 350.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Track|OffTrack")
	TObjectPtr<ATrackSplineActor> TrackSplineActor = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<URaceMinimapWidget> MinimapWidget = nullptr;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|DriftTuning")
	bool bEnableRuntimeDriftTuning = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|DriftTuning")
	bool bShowRuntimeDriftTuningOnScreen = true;

	EDriftTuningParam SelectedDriftTuningParam = EDriftTuningParam::TurnRateMultiplier;

	void HandleRuntimeDriftTuning();
	void CycleRuntimeDriftTuningParam(int32 Direction);
	void AdjustRuntimeDriftTuningValue(float Direction);
	FString GetRuntimeDriftTuningLabel() const;
	float GetRuntimeDriftTuningValue() const;
	void ShowRuntimeDriftTuningMessage() const;

	void UpdateSafeRecoveryPoint();
	bool ShouldIgnoreHit(const AActor* OtherActor, const UPrimitiveComponent* OtherComp) const;
	void ApplySelectedVehicleMesh();

	void UpdateOffTrackState(float DeltaTime);
	void EnsureMinimapWidget();
	void RemoveMinimapWidget();

	void TeleportBackToTrack();

	void ResetTrackRuleState();

	float GetSignedSlipAngleDegrees() const;
	float GetTravelYawRateDegrees(float DeltaTime, const FVector& CurrentTravelDir) const;
	void StartDriftBoost(float BonusSpeed, float Duration);

	void UpdateWrongWayState(float DeltaTime);

	void BeginPostDriftRecovery();
	float GetPostDriftSteeringInput(float RawSteeringInput) const;

	ATrackSplineActor* ResolveTrackSplineActor();
};

