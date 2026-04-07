#include "STR_RacerPawn.h"

#include "BuffComponent.h"
#include "BuffBase.h"
#include "ProjectileBuff.h"
#include "ProtoGameLabGameInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/Blueprint.h"
#include "Engine/StaticMesh.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/SpringArmComponent.h"
#include "PaperSpriteComponent.h"
#include "RaceMinimapWidget.h"
#include "TrackSplineActor.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Engine/Engine.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraFunctionLibrary.h"
#include "RaceGameMode.h"
#include "NiagaraSystem.h"

namespace
{
	const FVector DefaultSelectedVehicleLocation(0.f, 0.f, -40.f);
	const FRotator DefaultSelectedVehicleRotation(0.f, -90.f, 90.f);
	const FVector DefaultSelectedVehicleScale(1.f, 1.f, 1.f);

	struct FRacerPawnSelectedVehicleData
	{
		UStaticMesh* Mesh = nullptr;
		TArray<UMaterialInterface*> Materials;
		FVector RelativeLocation = DefaultSelectedVehicleLocation;
		FRotator RelativeRotation = DefaultSelectedVehicleRotation;
		FVector RelativeScale = DefaultSelectedVehicleScale;

		bool IsValid() const
		{
			return Mesh != nullptr;
		}
	};

	FRacerPawnSelectedVehicleData ResolveSelectedVehicleDataFromClass(UClass* VehicleClass)
	{
		FRacerPawnSelectedVehicleData VehicleData;

		if (!VehicleClass || !VehicleClass->IsChildOf(ASTR_RacerPawn::StaticClass()))
		{
			return VehicleData;
		}

		const ASTR_RacerPawn* DefaultPawn = Cast<ASTR_RacerPawn>(VehicleClass->GetDefaultObject());
		if (!DefaultPawn || !DefaultPawn->CarMesh)
		{
			return VehicleData;
		}

		VehicleData.Mesh = DefaultPawn->CarMesh->GetStaticMesh();
		VehicleData.RelativeLocation = DefaultPawn->CarMesh->GetRelativeLocation();
		VehicleData.RelativeRotation = DefaultPawn->CarMesh->GetRelativeRotation();
		VehicleData.RelativeScale = DefaultPawn->CarMesh->GetRelativeScale3D();

		const int32 NumMaterials = DefaultPawn->CarMesh->GetNumMaterials();
		VehicleData.Materials.Reserve(NumMaterials);
		for (int32 MaterialIndex = 0; MaterialIndex < NumMaterials; ++MaterialIndex)
		{
			VehicleData.Materials.Add(DefaultPawn->CarMesh->GetMaterial(MaterialIndex));
		}

		return VehicleData;
	}

	FRacerPawnSelectedVehicleData ResolveSelectedVehicleDataFromPath(const FSoftObjectPath& VehiclePath)
	{
		FRacerPawnSelectedVehicleData VehicleData;

		if (VehiclePath.IsNull())
		{
			return VehicleData;
		}

		if (UObject* LoadedObject = VehiclePath.TryLoad())
		{
			if (UStaticMesh* StaticMesh = Cast<UStaticMesh>(LoadedObject))
			{
				VehicleData.Mesh = StaticMesh;
				return VehicleData;
			}

			if (UBlueprint* Blueprint = Cast<UBlueprint>(LoadedObject))
			{
				return ResolveSelectedVehicleDataFromClass(Blueprint->GeneratedClass);
			}

			if (UClass* LoadedClass = Cast<UClass>(LoadedObject))
			{
				return ResolveSelectedVehicleDataFromClass(LoadedClass);
			}
		}

		return VehicleData;
	}

	UProjectileBuff* FindActiveProjectileBuff(UBuffComponent* BuffComponent)
	{
		if (!BuffComponent)
		{
			return nullptr;
		}

		for (UBuffBase* Buff : BuffComponent->ActiveBuffs)
		{
			if (UProjectileBuff* ProjectileBuff = Cast<UProjectileBuff>(Buff))
			{
				return ProjectileBuff;
			}
		}

		return nullptr;
	}
}

ASTR_RacerPawn::ASTR_RacerPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	// 1. Setup de la Capsule (Collision)
	BoxComp = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxComp"));
	RootComponent = BoxComp;
	BoxComp->SetBoxExtent(FVector(120.f, 60.f, 40.f));
	BoxComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	BoxComp->SetGenerateOverlapEvents(true);
	BoxComp->SetCollisionObjectType(ECC_Pawn);
	BoxComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	BoxComp->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	BoxComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	BoxComp->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Overlap);
	BoxComp->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	BoxComp->SetNotifyRigidBodyCollision(true);

	BoxComp->SetHiddenInGame(false);
	BoxComp->SetVisibility(true);

	// 2. Setup du Sprite
	//SpriteComp = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("SpriteComp"));
	CarMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CarMesh"));
	CarMesh->SetupAttachment(BoxComp);
	//SpriteComp->SetupAttachment(RootComponent);
	CarMesh->SetRelativeRotation(FRotator(0.0f, -90.0f, 90.0f));
	CarMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CarMesh->SetGenerateOverlapEvents(false);
	CarMesh->SetRelativeLocation(FVector(0.f, 0.f, -40.f));

	CarMesh->SetHiddenInGame(false);
	CarMesh->SetVisibility(true);

	// 3. Setup de la Caméra
	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComp"));
	SpringArmComp->SetupAttachment(RootComponent);
	SpringArmComp->SetUsingAbsoluteRotation(true);
	SpringArmComp->TargetArmLength = 800.0f;
	SpringArmComp->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));

	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComp"));
	CameraComp->SetupAttachment(SpringArmComp);

	BuffComponent = CreateDefaultSubobject<UBuffComponent>(TEXT("BuffComponent"));
	MinimapWidgetClass = URaceMinimapWidget::StaticClass();
}

void ASTR_RacerPawn::BeginPlay()
{
	Super::BeginPlay();

	TArray<AActor*> FoundTracks;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATrackSplineActor::StaticClass(), FoundTracks);

	for (AActor* Actor : FoundTracks)
	{
		if (ATrackSplineActor* Track = Cast<ATrackSplineActor>(Actor))
		{
			CachedTracks.Add(Track);
		}
	}

	if (CachedTracks.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TRACK] No tracks found for %s"), *GetName());
	}

	InitializeHeightLock();
	EnforceTrackHeight(true);

	BoxComp->OnComponentHit.AddDynamic(this, &ASTR_RacerPawn::OnHit);
	ApplySelectedVehicleMesh();
	EnsureMinimapWidget();
}

void ASTR_RacerPawn::InitializeHeightLock()
{
	LockedWorldZ = GetActorLocation().Z;
}

void ASTR_RacerPawn::EnforceTrackHeight(bool bTeleport)
{
	if (!bLockHeightToTrack)
	{
		return;
	}

	FVector FixedLocation = GetActorLocation();

	if (FMath::IsNearlyEqual(FixedLocation.Z, LockedWorldZ, 0.1f))
	{
		return;
	}

	FixedLocation.Z = LockedWorldZ;

	SetActorLocation(
		FixedLocation,
		false,
		nullptr,
		ETeleportType::TeleportPhysics
	);
}

void ASTR_RacerPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	HandleRuntimeDriftTuning();

	if (bMovementLocked)
	{
		TargetSteeringInput = 0.f;
		CurrentSteeringInput = 0.f;
		bIsDriftButtonHeld = false;
		bIsDrifting = false;
		DriftDirection = 0;
		DriftChargeDirection = 0;
		DriftCharge = 0.f;
		DriftHeldTime = 0.f;
		CurrentDriftAngle = 0.f;
		ActiveBoostTimer = 0.f;
		ActiveBoostBonusSpeed = 0.f;
		CurrentSpeed = 0.f;
		MoveVelocity = FVector::ZeroVector;

		if (BoxComp)
		{
			BoxComp->ComponentVelocity = FVector::ZeroVector;
		}

		return;
	}

	if (HitStunTimer > 0.f)
	{
		HitStunTimer -= DeltaTime;
	}

	if (TeleportFeedbackTimer > 0.f)
	{
		TeleportFeedbackTimer = FMath::Max(0.f, TeleportFeedbackTimer - DeltaTime);
	}

	if (bTrackRulesEnabled)
	{
		UpdateOffTrackState(DeltaTime);
		UpdateWrongWayState(DeltaTime);
	}
	else
	{
		ResetTrackRuleState();
	}

	float EffectiveAccelerationRate = AccelerationRate;
	float EffectiveBrakingDeceleration = BrakingDeceleration;
	float EffectiveCoastingDeceleration = CoastingDeceleration;
	float EffectiveMaxSpeed = MaxSpeed + CurrentBoostExtraSpeed;
	float ExtraOffTrackDeceleration = 0.f;

	if (bOffTrackPenaltyActive)
	{
		EffectiveAccelerationRate *= OffTrackAccelerationMultiplier;
		EffectiveMaxSpeed *= OffTrackMaxSpeedMultiplier;
		EffectiveCoastingDeceleration += OffTrackExtraDeceleration;
		EffectiveBrakingDeceleration += OffTrackExtraDeceleration * 0.5f;
		ExtraOffTrackDeceleration = OffTrackExtraDeceleration;
	}

	// si drift actif, alors on reduit son temps restant
	if (DriftDirectionChangeCooldownTimer > 0.f)
	{
		DriftDirectionChangeCooldownTimer = FMath::Max(0.f, DriftDirectionChangeCooldownTimer - DeltaTime);
	}

	if (bPostDriftRecoveryActive)
	{
		PostDriftRecoveryTimer -= DeltaTime;
		if (PostDriftRecoveryTimer <= 0.f)
		{
			bPostDriftRecoveryActive = false;
			PostDriftRecoveryTimer = 0.f;
		}
	}

	if (ActiveBoostTimer > 0.f)
	{
		ActiveBoostTimer -= DeltaTime;
		if (ActiveBoostTimer <= 0.f)
		{
			ActiveBoostTimer = 0.f;
		}
	}

	if (ActiveBoostTimer > 0.f)
	{
		CurrentBoostExtraSpeed = ActiveBoostBonusSpeed;
	}
	else
	{
		CurrentBoostExtraSpeed = FMath::Max(0.f, CurrentBoostExtraSpeed - BoostDecaySpeed * DeltaTime);
	}

	//Lissage du steering
	CurrentSteeringInput = FMath::FInterpTo(
		CurrentSteeringInput,
		TargetSteeringInput,
		DeltaTime,
		SteeringInterpSpeed
	);

	const float DriftIntentInput = !FMath::IsNearlyZero(TargetSteeringInput, 0.01f)
		? TargetSteeringInput
		: CurrentSteeringInput;

	const bool bHasSteerForDrift = FMath::Abs(DriftIntentInput) >= DriftSteerThreshold;
	const bool bFastEnoughToStartDrift = CurrentSpeed >= MinSpeedToStartDrift;
	const bool bFastEnoughToKeepDrift = CurrentSpeed >= MinDriftSpeed;

	// drift = bouton drift seulement
	const bool bWantsDrift = bIsDriftButtonHeld;

	// auto-accel = logique feature
	const bool bShouldAccelerate = bAutoDriveEnabled && HitStunTimer <= 0.f;

	if (!bAllowDrift && bIsDrifting)
	{
		bIsDrifting = false;
		DriftDirection = 0;
		DriftChargeDirection = 0;
		CurrentDriftAngle = 0.f;
		DriftDirectionChangeCooldownTimer = 0.f;
	}

	if (bAllowDrift && !bIsDrifting)
	{
		if (bWantsDrift && bFastEnoughToStartDrift && bHasSteerForDrift)
		{
			bIsDrifting = true;
			DriftDirection = (DriftIntentInput > 0.f) ? 1 : -1;
			DriftChargeDirection = DriftDirection;
			bDriftBoostStillValid = true;
			DriftDirectionChangeCooldownTimer = 0.f;
		}
	}
	else
	{
		if (!bWantsDrift || !bFastEnoughToKeepDrift)
		{
			bIsDrifting = false;
			DriftDirection = 0;
			DriftChargeDirection = 0;
			DriftDirectionChangeCooldownTimer = 0.f;
		}
	}

	if (bIsDrifting)
	{
		const int32 NewSteerDirection =
			(DriftIntentInput > DriftDirectionSwitchThreshold) ? 1 :
			(DriftIntentInput < -DriftDirectionSwitchThreshold) ? -1 : 0;

		if (NewSteerDirection != 0 &&
			DriftChargeDirection != 0 &&
			NewSteerDirection != DriftChargeDirection &&
			DriftDirectionChangeCooldownTimer <= 0.f)
		{
			DriftCharge = 0.f;
			DriftHeldTime = 0.f;
			DriftChargeDirection = NewSteerDirection;
			DriftDirection = NewSteerDirection;
			bDriftBoostStillValid = true;
			DriftDirectionChangeCooldownTimer = DriftDirectionChangeCooldown;
		}
	}

	if (bIsDrifting)
	{
		if (bShouldAccelerate)
		{
			CurrentSpeed += (
				EffectiveAccelerationRate * DriftAccelMultiplier
				- DriftSpeedLossPerSecond
				- ExtraOffTrackDeceleration
				) * DeltaTime;
		}
		else
		{
			CurrentSpeed -= (EffectiveCoastingDeceleration + DriftSpeedLossPerSecond) * DeltaTime;
		}
	}
	else if (bShouldAccelerate)
	{
		CurrentSpeed += EffectiveAccelerationRate * DeltaTime;
	}
	else
	{
		CurrentSpeed -= EffectiveCoastingDeceleration * DeltaTime;
	}

	//gestion du steering a haute et basse vitesse
	CurrentSpeed = FMath::Clamp(CurrentSpeed, -600.f, EffectiveMaxSpeed);

	const float SpeedRatio = FMath::Clamp(CurrentSpeed / EffectiveMaxSpeed, 0.f, 1.f);
	const float BaseTurnRate = FMath::Lerp(
		MaxTurnRate,
		MinTurnRateAtMaxSpeed,
		SpeedRatio
	);

	//mouvement hors drift
	if (!bIsDrifting)
	{
		const float PostDriftSteeringInput = GetPostDriftSteeringInput(CurrentSteeringInput);

		if (CurrentSpeed > MinSpeedToTurn && !FMath::IsNearlyZero(PostDriftSteeringInput, 0.01f))
		{
			const float YawDelta = PostDriftSteeringInput * BaseTurnRate * DeltaTime;
			AddActorLocalRotation(FRotator(0.f, YawDelta, 0.f));
		}

		const float ExitInterpSpeed = bPostDriftRecoveryActive
			? PostDriftRotationBlendSpeed
			: DriftAngleInterpSpeed;

		CurrentDriftAngle = FMath::FInterpTo(
			CurrentDriftAngle,
			0.f,
			DeltaTime,
			ExitInterpSpeed
		);

		const FVector DesiredVelocity = GetActorForwardVector() * CurrentSpeed;

		MoveVelocity = FMath::VInterpTo(
			MoveVelocity,
			DesiredVelocity,
			DeltaTime,
			NormalGrip
		);
	}
	else //mouvement en drift
	{
		FVector TravelDir = MoveVelocity.IsNearlyZero()
			? GetActorForwardVector()
			: MoveVelocity.GetSafeNormal();

		float EffectiveDriftSteerInput = 0.f;

		if (CurrentSpeed > MinSpeedToTurn && DriftDirection != 0)
		{
			// Base automatique du drift : il continue naturellement dans sa direction
			EffectiveDriftSteerInput = DriftBaseAutoSteer * DriftDirection;

			const float SteeringVsDrift = CurrentSteeringInput * DriftDirection;

			if (SteeringVsDrift > 0.f)
			{
				// Le joueur steer dans le même sens que le drift
				EffectiveDriftSteerInput += CurrentSteeringInput * DriftSteerSameDirectionMultiplier;
			}
			else if (SteeringVsDrift < 0.f)
			{
				// Le joueur contre-steer : influence faible seulement
				EffectiveDriftSteerInput += CurrentSteeringInput * DriftSteerOppositeDirectionMultiplier;
			}

			EffectiveDriftSteerInput = FMath::Clamp(EffectiveDriftSteerInput, -1.f, 1.f);

			if (!FMath::IsNearlyZero(EffectiveDriftSteerInput, 0.01f))
			{
				const float DriftYawDelta = EffectiveDriftSteerInput * BaseTurnRate * DriftTurnRateMultiplier * DeltaTime;
				TravelDir = TravelDir.RotateAngleAxis(DriftYawDelta, FVector::UpVector).GetSafeNormal();
			}
		}

		//En drift : vitesse desiree, grip, angle et rotation finale
		const FVector DesiredVelocity = TravelDir * CurrentSpeed;

		MoveVelocity = FMath::VInterpTo(
			MoveVelocity,
			DesiredVelocity,
			DeltaTime,
			DriftGrip
		);

		if (!MoveVelocity.IsNearlyZero())
		{
			MoveVelocity = MoveVelocity.GetSafeNormal() * CurrentSpeed;
		}
		else
		{
			MoveVelocity = DesiredVelocity;
		}

		CurrentDriftAngle = FMath::FInterpTo(
			CurrentDriftAngle,
			DriftDirection * MaxDriftAngle,
			DeltaTime,
			DriftAngleInterpSpeed
		);

		const FRotator TravelRot = MoveVelocity.Rotation();
		const FRotator DriftRot(0.f, TravelRot.Yaw + CurrentDriftAngle, 0.f);
		SetActorRotation(DriftRot);
	}

	//gestion drift boost
	const FVector CurrentTravelDir = MoveVelocity.IsNearlyZero()
		? GetActorForwardVector().GetSafeNormal2D()
		: MoveVelocity.GetSafeNormal2D();

	const float SignedSlipAngleDeg = GetSignedSlipAngleDegrees();
	const float TravelYawRateDeg = GetTravelYawRateDegrees(DeltaTime, CurrentTravelDir);

	float EffectiveBoostSteerInput = CurrentSteeringInput;

	if (bIsDrifting && DriftDirection != 0)
	{
		EffectiveBoostSteerInput = DriftBaseAutoSteer * DriftDirection;

		const float SteeringVsDrift = CurrentSteeringInput * DriftDirection;

		if (SteeringVsDrift > 0.f)
		{
			EffectiveBoostSteerInput += CurrentSteeringInput * DriftSteerSameDirectionMultiplier;
		}
		else if (SteeringVsDrift < 0.f)
		{
			EffectiveBoostSteerInput += CurrentSteeringInput * DriftSteerOppositeDirectionMultiplier;
		}

		EffectiveBoostSteerInput = FMath::Clamp(EffectiveBoostSteerInput, -1.f, 1.f);
	}

	const bool bNeutralBoostSteer = FMath::IsNearlyZero(EffectiveBoostSteerInput, 0.01f);
	const bool bCorrectSteerDirection =
		bNeutralBoostSteer ||
		(DriftChargeDirection == 0) ||
		(FMath::Sign(EffectiveBoostSteerInput) == DriftChargeDirection);

	const bool bRealDriftForBoost =
		bIsDrifting &&
		bDriftBoostStillValid &&
		CurrentSpeed >= MinBoostSpeed &&
		FMath::Abs(EffectiveBoostSteerInput) >= MinBoostSteerInput &&
		FMath::Abs(SignedSlipAngleDeg) >= MinBoostSlipAngleDeg &&
		TravelYawRateDeg >= MinTravelYawRateDeg &&
		bCorrectSteerDirection;

	if (bIsDrifting)
	{
		DriftHeldTime += DeltaTime;

		if (bRealDriftForBoost)
		{
			const float SlipFactor = FMath::GetMappedRangeValueClamped(
				FVector2D(MinBoostSlipAngleDeg, MaxUsefulSlipAngleDeg),
				FVector2D(0.4f, 1.0f),
				FMath::Abs(SignedSlipAngleDeg)
			);

			const float TurnFactor = FMath::GetMappedRangeValueClamped(
				FVector2D(MinTravelYawRateDeg, 120.f),
				FVector2D(0.4f, 1.0f),
				TravelYawRateDeg
			);

			DriftCharge += DriftChargeRate * SlipFactor * TurnFactor * DeltaTime;
			DriftCharge = FMath::Clamp(DriftCharge, 0.f, 100.f);
		}
		else
		{
			const float DecayRate = bCorrectSteerDirection ? DriftChargeDecayRate : CounterSteerDecayRate;
			DriftCharge = FMath::Max(0.f, DriftCharge - DecayRate * DeltaTime);
		}
	}

	if (bWasDriftingLastFrame && !bIsDrifting)
	{
		LastDriftDirection = DriftDirection;
		BeginPostDriftRecovery();

		if (bDriftBoostStillValid && DriftHeldTime >= MinChargeTimeForBoost)
		{
			if (DriftCharge >= LargeBoostCharge)
			{
				StartDriftBoost(LargeBoostBonusSpeed, LargeBoostDuration);
				UE_LOG(LogTemp, Warning, TEXT("[DRIFT BOOST] LARGE"));
			}
			else if (DriftCharge >= MediumBoostCharge)
			{
				StartDriftBoost(MediumBoostBonusSpeed, MediumBoostDuration);
				UE_LOG(LogTemp, Warning, TEXT("[DRIFT BOOST] MEDIUM"));
			}
			else if (DriftCharge >= SmallBoostCharge)
			{
				StartDriftBoost(SmallBoostBonusSpeed, SmallBoostDuration);
				UE_LOG(LogTemp, Warning, TEXT("[DRIFT BOOST] SMALL"));
			}
		}

		DriftCharge = 0.f;
		DriftHeldTime = 0.f;
		bDriftBoostStillValid = false;
		DriftChargeDirection = 0;
		DriftDirection = 0;
	}

	bWasDriftingLastFrame = bIsDrifting;
	LastTravelDir = CurrentTravelDir;

	MoveVelocity.Z = 0.f;

	const FVector Delta = FVector(MoveVelocity.X, MoveVelocity.Y, 0.f) * DeltaTime;

	FHitResult Hit;
	BoxComp->MoveComponent(Delta, GetActorRotation(), !bIgnoreObstacleHits, &Hit);
	EnforceTrackHeight();

	if (DeltaTime > 0.f)
	{
		BoxComp->ComponentVelocity = FVector(MoveVelocity.X, MoveVelocity.Y, 0.f);
	}

	UpdateSafeRecoveryPoint();

	ATrackSplineActor* NewTrack = ResolveTrackSplineActor();

	if (NewTrack && NewTrack != CurrentMinimapTrack)
	{
		CurrentMinimapTrack = NewTrack;

		if (MinimapWidget)
		{
			UE_LOG(LogTemp, Warning, TEXT("[MINIMAP] Track changed to %s"), *GetNameSafe(NewTrack));
			MinimapWidget->SetTrackSplineActor(NewTrack);
		}
	}
}

void ASTR_RacerPawn::UpdateOffTrackState(float DeltaTime)
{
	const bool bWasOffTrack = bIsOffTrack;
	const bool bWasPenaltyActive = bOffTrackPenaltyActive;

	ATrackSplineActor* ActiveTrack = ResolveTrackSplineActor();

	if (!ActiveTrack)
	{
		bIsOffTrack = false;
		bOffTrackPenaltyActive = false;
		OffTrackTime = 0.f;
		return;
	}

	bIsOffTrack = !ActiveTrack->IsLocationOnTrack(GetActorLocation(), OffTrackDetectionMargin);

	if (bIsOffTrack)
	{
		OffTrackTime += DeltaTime;
	}
	else
	{
		OffTrackTime = 0.f;
	}

	bOffTrackPenaltyActive = bIsOffTrack && OffTrackTime >= OffTrackPenaltyDelay;

	if (bIsOffTrack && OffTrackTime >= OffTrackTeleportDelay)
	{
		TeleportBackToTrack();
		return;
	}

	if (bIsOffTrack != bWasOffTrack)
	{
		UE_LOG(LogTemp, Warning, TEXT("[OFF TRACK] %s -> %s"),
			*GetName(),
			bIsOffTrack ? TEXT("LEFT TRACK") : TEXT("BACK ON TRACK"));

		OnOffTrackStateChanged(bIsOffTrack, GetLocalPlayerIndex());
	}

	if (bOffTrackPenaltyActive != bWasPenaltyActive)
	{
		UE_LOG(LogTemp, Warning, TEXT("[OFF TRACK PENALTY] %s -> %s"),
			*GetName(),
			bOffTrackPenaltyActive ? TEXT("ACTIVE") : TEXT("INACTIVE"));
	}
}

void ASTR_RacerPawn::UpdateSafeRecoveryPoint()
{
	ATrackSplineActor* ActiveTrack = ResolveTrackSplineActor();

	if (!ActiveTrack)
	{
		return;
	}

	const float TrackHalfWidth = ActiveTrack->GetTrackHalfWidthWorld();
	if (TrackHalfWidth <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const FVector ActorLocation = GetActorLocation();
	const float DistanceToCenter = ActiveTrack->GetDistanceFromTrackCenter2D(ActorLocation);

	const bool bComfortablyOnTrack = DistanceToCenter <= (TrackHalfWidth * SafeRecoveryTrackRatio);

	if (!bComfortablyOnTrack)
	{
		return;
	}

	FVector SafeLocation = ActiveTrack->GetClosestWorldLocationOnTrack(ActorLocation);
	SafeLocation.Z = LockedWorldZ;

	FVector SafeForward = ActiveTrack->GetTrackForwardDirectionAtWorldLocation(SafeLocation);

	if (SafeForward.IsNearlyZero())
	{
		SafeForward = LastSafeForward.GetSafeNormal2D();
	}
	if (SafeForward.IsNearlyZero())
	{
		SafeForward = GetActorForwardVector().GetSafeNormal2D();
	}
	if (SafeForward.IsNearlyZero())
	{
		SafeForward = FVector::ForwardVector;
	}

	bHasSafeRecoveryPoint = true;
	LastSafeLocation = SafeLocation;
	LastSafeForward = SafeForward;
	LastSafeSpeed = FMath::Max(CurrentSpeed, 0.f);
}

void ASTR_RacerPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (SteerAction)
		{
			EnhancedInputComponent->BindAction(SteerAction, ETriggerEvent::Triggered, this, &ASTR_RacerPawn::Steer);
			EnhancedInputComponent->BindAction(SteerAction, ETriggerEvent::Completed, this, &ASTR_RacerPawn::Steer);
			EnhancedInputComponent->BindAction(SteerAction, ETriggerEvent::Canceled, this, &ASTR_RacerPawn::Steer);
		}

		if (BrakeAction)
		{
			EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Started, this, &ASTR_RacerPawn::StartDrift);
			EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Completed, this, &ASTR_RacerPawn::StopDrift);
			EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Canceled, this, &ASTR_RacerPawn::StopDrift);
		}

		if (ItemAction)
		{
			EnhancedInputComponent->BindAction(ItemAction, ETriggerEvent::Started, this, &ASTR_RacerPawn::UseItem);
		}
	}
}

void ASTR_RacerPawn::Steer(const FInputActionValue& Value)
{
	if (bMovementLocked)
	{
		TargetSteeringInput = 0.f;
		return;
	}

	const float RawSteer = Value.Get<float>();
	TargetSteeringInput = FMath::Clamp(RawSteer, -1.0f, 1.0f);

	if (FMath::Abs(TargetSteeringInput) < 0.1f)
	{
		TargetSteeringInput = 0.f;
	}
}

void ASTR_RacerPawn::UseItem(const FInputActionValue& Value)
{

	if (bMovementLocked)
	{
		return;
	}

	if (!BuffComponent) return;

	if (BuffComponent->CurrentBuff)
	{
		const bool bStoredProjectile = Cast<UProjectileBuff>(BuffComponent->CurrentBuff) != nullptr;
		BuffComponent->UseBuff();

		if (bStoredProjectile)
		{
			if (UProjectileBuff* ProjectileBuff = FindActiveProjectileBuff(BuffComponent))
			{
				ProjectileBuff->FireProjectile();
			}
		}

		return;
	}

	if (UProjectileBuff* ProjectileBuff = FindActiveProjectileBuff(BuffComponent))
	{
		ProjectileBuff->FireProjectile();
	}
}

void ASTR_RacerPawn::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	APlayerController* PC = Cast<APlayerController>(NewController);
	if (!PC) return;

	PC->SetViewTargetWithBlend(this, 0.0f);

	ULocalPlayer* LP = PC->GetLocalPlayer();
	if (!LP) return;

	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LP);

	if (Subsystem && DefaultMappingContext)
	{
		Subsystem->AddMappingContext(DefaultMappingContext, 0);
	}

	ApplySelectedVehicleMesh();
	EnsureMinimapWidget();
}

void ASTR_RacerPawn::UnPossessed()
{
	RemoveMinimapWidget();

	APlayerController* PC = Cast<APlayerController>(Controller);
	if (PC)
	{
		if (ULocalPlayer* LP = PC->GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LP))
			{
				if (DefaultMappingContext)
				{
					Subsystem->RemoveMappingContext(DefaultMappingContext);
				}
			}
		}
	}

	Super::UnPossessed();
}

void ASTR_RacerPawn::EnsureMinimapWidget()
{
	if (MinimapWidget || !MinimapWidgetClass)
	{
		return;
	}

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC || !PC->IsLocalController())
	{
		return;
	}

	ULocalPlayer* LP = PC->GetLocalPlayer();
	if (!LP || LP->GetControllerId() != 0)
	{
		return;
	}

	MinimapWidget = CreateWidget<URaceMinimapWidget>(PC, MinimapWidgetClass);
	if (MinimapWidget)
	{
		if (ATrackSplineActor* ActiveTrack = ResolveTrackSplineActor())
		{
			UE_LOG(LogTemp, Warning, TEXT("[MINIMAP] ActiveTrack for %s = %s"),
				*GetName(), *GetNameSafe(ActiveTrack));
			MinimapWidget->SetTrackSplineActor(ActiveTrack);
		}
		//Ajout de la minimap au viewport avec un ZOrder de 40 pour s'assurer qu'elle est au dessus de la plupart des autres éléments UI
		MinimapWidget->AddToViewport(40);
	}
}

void ASTR_RacerPawn::RemoveMinimapWidget()
{
	if (MinimapWidget)
	{
		MinimapWidget->RemoveFromParent();
		MinimapWidget = nullptr;
	}
}

void ASTR_RacerPawn::ApplySelectedVehicleMesh()
{
	if (!CarMesh)
	{
		return;
	}

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return;
	}

	ULocalPlayer* LocalPlayer = PC->GetLocalPlayer();
	if (!LocalPlayer)
	{
		return;
	}

	const int32 PlayerIndex = LocalPlayer->GetControllerId();
	if (PlayerIndex < 0 || !GetWorld())
	{
		return;
	}

	const UProtoGameLabGameInstance* GameInstance = Cast<UProtoGameLabGameInstance>(GetWorld()->GetGameInstance());
	if (!GameInstance)
	{
		return;
	}

	const FSoftObjectPath SelectedMeshPath = GameInstance->GetSelectedVehicleMesh(PlayerIndex);
	if (SelectedMeshPath.IsNull())
	{
		return;
	}

	const FRacerPawnSelectedVehicleData SelectedVehicleData = ResolveSelectedVehicleDataFromPath(SelectedMeshPath);
	if (SelectedVehicleData.IsValid())
	{
		CarMesh->EmptyOverrideMaterials();
		CarMesh->SetStaticMesh(SelectedVehicleData.Mesh);
		CarMesh->SetRelativeLocation(SelectedVehicleData.RelativeLocation);
		CarMesh->SetRelativeRotation(SelectedVehicleData.RelativeRotation);
		CarMesh->SetRelativeScale3D(SelectedVehicleData.RelativeScale);

		for (int32 MaterialIndex = 0; MaterialIndex < SelectedVehicleData.Materials.Num(); ++MaterialIndex)
		{
			CarMesh->SetMaterial(MaterialIndex, SelectedVehicleData.Materials[MaterialIndex]);
		}
	}
}

float ASTR_RacerPawn::GetSignedSlipAngleDegrees() const
{
	if (MoveVelocity.IsNearlyZero())
	{
		return 0.f;
	}

	const FVector Forward = GetActorForwardVector().GetSafeNormal2D();
	const FVector Right = GetActorRightVector().GetSafeNormal2D();
	const FVector Travel = MoveVelocity.GetSafeNormal2D();

	const float Dot = FMath::Clamp(FVector::DotProduct(Forward, Travel), -1.f, 1.f);
	const float UnsignedAngleDeg = FMath::RadiansToDegrees(FMath::Acos(Dot));

	const float Side = FVector::DotProduct(Right, Travel);
	const float Sign = (Side >= 0.f) ? 1.f : -1.f;

	return UnsignedAngleDeg * Sign;
}

float ASTR_RacerPawn::GetTravelYawRateDegrees(float DeltaTime, const FVector& CurrentTravelDir) const
{
	if (DeltaTime <= KINDA_SMALL_NUMBER || LastTravelDir.IsNearlyZero() || CurrentTravelDir.IsNearlyZero())
	{
		return 0.f;
	}

	const float Dot = FMath::Clamp(
		FVector::DotProduct(LastTravelDir.GetSafeNormal2D(), CurrentTravelDir.GetSafeNormal2D()),
		-1.f,
		1.f
	);

	const float AngleDeg = FMath::RadiansToDegrees(FMath::Acos(Dot));
	return AngleDeg / DeltaTime;
}

void ASTR_RacerPawn::StartDriftBoost(float BonusSpeed, float Duration)
{
	ActiveBoostBonusSpeed = BonusSpeed;
	ActiveBoostTimer = Duration;
	CurrentSpeed = FMath::Min(CurrentSpeed + BonusSpeed, MaxSpeed + BonusSpeed);
	CurrentBoostExtraSpeed = FMath::Max(CurrentBoostExtraSpeed, BonusSpeed);
}

void ASTR_RacerPawn::SetSteeringInput(float InSteer)
{
	TargetSteeringInput = FMath::Clamp(InSteer, -1.0f, 1.0f);

	if (FMath::Abs(TargetSteeringInput) < 0.1f)
	{
		TargetSteeringInput = 0.f;
	}
}

void ASTR_RacerPawn::ClearDrivingInputs()
{
	TargetSteeringInput = 0.f;
	CurrentSteeringInput = 0.f;
	bIsDriftButtonHeld = false;
}

void ASTR_RacerPawn::TriggerItemUse()
{
	if (bMovementLocked)
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[ITEM] TriggerItemUse called on %s"), *GetName());

	if (!BuffComponent)
	{
		return;
	}

	if (BuffComponent->CurrentBuff)
	{
		const bool bStoredProjectile = Cast<UProjectileBuff>(BuffComponent->CurrentBuff) != nullptr;
		BuffComponent->UseBuff();

		if (bStoredProjectile)
		{
			if (UProjectileBuff* ProjectileBuff = FindActiveProjectileBuff(BuffComponent))
			{
				ProjectileBuff->FireProjectile();
			}
		}

		return;
	}

	if (UProjectileBuff* ProjectileBuff = FindActiveProjectileBuff(BuffComponent))
	{
		ProjectileBuff->FireProjectile();
	}
}

bool ASTR_RacerPawn::HasBuff() const
{
	return BuffComponent && BuffComponent->CurrentBuff != nullptr;
}

void ASTR_RacerPawn::SetIgnoreObstacleHits(bool bShouldIgnore)
{
	bIgnoreObstacleHits = bShouldIgnore;
}

bool ASTR_RacerPawn::ShouldIgnoreHit(const AActor* OtherActor, const UPrimitiveComponent* OtherComp) const
{
	if (!bIgnoreObstacleHits)
	{
		return false;
	}

	if (OtherComp && OtherComp->GetCollisionObjectType() == ECC_WorldStatic)
	{
		return true;
	}

	const UPrimitiveComponent* RootPrimitive = OtherActor
		? Cast<UPrimitiveComponent>(OtherActor->GetRootComponent())
		: nullptr;

	return RootPrimitive && RootPrimitive->GetCollisionObjectType() == ECC_WorldStatic;
}

void ASTR_RacerPawn::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, FVector NormalImpulse,
	const FHitResult& Hit)
{
	if (!OtherActor) return;

	if (ShouldIgnoreHit(OtherActor, OtherComp))
	{
		return;
	}

	// Cooldown anti spam
	if (GetWorld()->TimeSeconds - LastHitTime < HitCooldown)
		return;

	LastHitTime = GetWorld()->TimeSeconds;

	FVector KnockbackDir = Hit.ImpactNormal.GetSafeNormal2D();

	if (KnockbackDir.IsNearlyZero())
	{
		KnockbackDir = (-GetActorForwardVector()).GetSafeNormal2D();
	}

	// Détection du type de choc (face vs côté)
	const float Dot = FVector::DotProduct(GetActorForwardVector(), KnockbackDir);

	// CHOC FRONTAL → recul réel
	if (Dot < -0.3f)
	{
		CurrentSpeed = -900.f; // vitesse négative = recul
	}
	else
	{
		// CHOC LATÉRAL -> push
		MoveVelocity += KnockbackDir * KnockbackStrength;
		MoveVelocity.Z = 0.f;
	}

	// Réduction de vitesse globale
	CurrentSpeed *= 0.5f;

	// Stop drift
	bIsDrifting = false;

	// Petit stun pour éviter ré-accélération instantanée
	HitStunTimer = 0.01f;

	if (ImpactEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			ImpactEffect,
			Hit.ImpactPoint,
			Hit.ImpactNormal.Rotation()
		);
	}
}

void ASTR_RacerPawn::TeleportBackToTrack()
{
	ATrackSplineActor* ActiveTrack = ResolveTrackSplineActor();

	if (!ActiveTrack)
	{
		UE_LOG(LogTemp, Warning, TEXT("[OFF TRACK] %s has no TrackSplineActor"), *GetName());
		return;
	}

	FVector NewLocation;
	FVector SafeForward;

	if (bHasSafeRecoveryPoint)
	{
		NewLocation = LastSafeLocation;
		SafeForward = LastSafeForward.GetSafeNormal2D();
	}
	else
	{
		// Fallback de secours : on prend quand même le centre le plus proche
		NewLocation = ActiveTrack->GetClosestWorldLocationOnTrack(GetActorLocation());
		SafeForward = ActiveTrack->GetTrackForwardDirectionAtWorldLocation(NewLocation);
	}

	// On re-snap toujours au CENTRE de la track
	NewLocation = ActiveTrack->GetClosestWorldLocationOnTrack(NewLocation);
	NewLocation.Z = LockedWorldZ;

	// On re-prend toujours la direction de la track
	SafeForward = ActiveTrack->GetTrackForwardDirectionAtWorldLocation(NewLocation);

	if (SafeForward.IsNearlyZero())
	{
		SafeForward = LastSafeForward.GetSafeNormal2D();
	}
	if (SafeForward.IsNearlyZero())
	{
		SafeForward = FVector::ForwardVector;
	}

	const FRotator NewRotation = SafeForward.Rotation();

	SetActorLocationAndRotation(
		NewLocation,
		NewRotation,
		false,
		nullptr,
		ETeleportType::TeleportPhysics
	);

	MoveVelocity.Z = 0.f;
	EnforceTrackHeight(true);

	// Reset état drift / boost
	bIsDrifting = false;
	DriftDirection = 0;
	CurrentDriftAngle = 0.f;
	DriftCharge = 0.f;
	DriftHeldTime = 0.f;
	DriftChargeDirection = 0;
	bWasDriftingLastFrame = false;
	bDriftBoostStillValid = false;

	ActiveBoostTimer = 0.f;
	ActiveBoostBonusSpeed = 0.f;

	// Remet une vitesse propre
	CurrentSpeed = FMath::Min(LastSafeSpeed, RecoverySpeedAfterTeleport);
	if (CurrentSpeed < 200.f)
	{
		CurrentSpeed = 200.f;
	}

	MoveVelocity = SafeForward * CurrentSpeed;
	LastTravelDir = SafeForward;

	// On met à jour le point safe avec la vraie version propre
	bHasSafeRecoveryPoint = true;
	LastSafeLocation = NewLocation;
	LastSafeForward = SafeForward;
	LastSafeSpeed = CurrentSpeed;

	// Reset état off-track
	bIsOffTrack = false;
	bOffTrackPenaltyActive = false;
	OffTrackTime = 0.f;

	UE_LOG(LogTemp, Warning, TEXT("[OFF TRACK] %s teleported back to TRACK CENTER"), *GetName());

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			2.0f,
			FColor::Orange,
			FString::Printf(TEXT("%s was returned to the track"), *GetName())
		);
	}

	TeleportFeedbackTimer = TeleportFeedbackDuration;
}

void ASTR_RacerPawn::ResetTrackRuleState()
{
	bIsOffTrack = false;
	bOffTrackPenaltyActive = false;
	OffTrackTime = 0.f;

	bIsGoingWrongWay = false;
	bWrongWayWarningActive = false;
	WrongWayTime = 0.f;

	TeleportFeedbackTimer = 0.f;
}

void ASTR_RacerPawn::UpdateWrongWayState(float DeltaTime)
{
	const bool bWasWrongWay = bIsGoingWrongWay;
	const bool bWasWarningActive = bWrongWayWarningActive;

	ATrackSplineActor* ActiveTrack = ResolveTrackSplineActor();

	if (!ActiveTrack)
	{
		bIsGoingWrongWay = false;
		bWrongWayWarningActive = false;
		WrongWayTime = 0.f;
		return;
	}

	// on ne détecte pas le contre-sens quand le joueur est hors piste.
	if (bIsOffTrack)
	{
		bIsGoingWrongWay = false;
		bWrongWayWarningActive = false;
		WrongWayTime = 0.f;
		return;
	}

	// Si la voiture ne bouge presque pas, pas de warning.
	if (CurrentSpeed < WrongWayMinSpeed || MoveVelocity.IsNearlyZero())
	{
		bIsGoingWrongWay = false;
		bWrongWayWarningActive = false;
		WrongWayTime = 0.f;
		return;
	}

	const FVector TravelDirection = MoveVelocity.GetSafeNormal2D();
	const FVector TrackDirection = ActiveTrack->GetTrackForwardDirectionAtWorldLocation(GetActorLocation());

	const float Dot = FVector::DotProduct(TravelDirection, TrackDirection);

	// Dot proche de 1  -> bon sens
	// Dot proche de -1 -> contre-sens
	bIsGoingWrongWay = (Dot <= WrongWayDotThreshold);

	if (bIsGoingWrongWay)
	{
		WrongWayTime += DeltaTime;
	}
	else
	{
		WrongWayTime = 0.f;
	}

	bWrongWayWarningActive = bIsGoingWrongWay && (WrongWayTime >= WrongWayDetectionDelay);

	if (bIsGoingWrongWay != bWasWrongWay)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WRONG WAY] %s -> %s (Dot=%.2f)"),
			*GetName(),
			bIsGoingWrongWay ? TEXT("DETECTED") : TEXT("CLEARED"),
			Dot);
	}

	if (bWrongWayWarningActive != bWasWarningActive)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WRONG WAY UI] %s -> %s"),
			*GetName(),
			bWrongWayWarningActive ? TEXT("SHOW WARNING") : TEXT("HIDE WARNING"));
	}
}

void ASTR_RacerPawn::SetMovementLocked(bool bLocked)
{
	bMovementLocked = bLocked;

	if (bMovementLocked)
	{
		// Inputs
		TargetSteeringInput = 0.f;
		CurrentSteeringInput = 0.f;
		bIsDriftButtonHeld = false;

		// Drift
		bIsDrifting = false;
		DriftDirection = 0;
		DriftChargeDirection = 0;
		DriftCharge = 0.f;
		DriftHeldTime = 0.f;
		CurrentDriftAngle = 0.f;
		bWasDriftingLastFrame = false;
		bDriftBoostStillValid = false;

		// Boost
		ActiveBoostTimer = 0.f;
		ActiveBoostBonusSpeed = 0.f;

		// Movement
		CurrentSpeed = 0.f;
		MoveVelocity = FVector::ZeroVector;
		LastTravelDir = GetActorForwardVector().GetSafeNormal2D();

		// Divers
		HitStunTimer = 0.f;

		if (BoxComp)
		{
			BoxComp->ComponentVelocity = FVector::ZeroVector;
		}
	}
}

void ASTR_RacerPawn::SetAutoDriveEnabled(bool bShouldAutoDrive)
{
	bAutoDriveEnabled = bShouldAutoDrive;
}

void ASTR_RacerPawn::SetTrackRulesEnabled(bool bEnabled)
{
	bTrackRulesEnabled = bEnabled;
	UE_LOG(LogTemp, Warning, TEXT("[TRACK RULES] %s -> %s"),
		*GetName(),
		bEnabled ? TEXT("ENABLED") : TEXT("DISABLED"));

	if (!bTrackRulesEnabled)
	{
		ResetTrackRuleState();
	}
}

void ASTR_RacerPawn::SetDriftButtonHeld(bool bShouldHoldDrift)
{
	bIsDriftButtonHeld = bShouldHoldDrift;
}

void ASTR_RacerPawn::StartDrift(const FInputActionValue& Value)
{
	if (bMovementLocked)
	{
		bIsDriftButtonHeld = false;
		return;
	}

	bIsDriftButtonHeld = true;
}

void ASTR_RacerPawn::StopDrift(const FInputActionValue& Value)
{
	bIsDriftButtonHeld = false;
}

void ASTR_RacerPawn::HandleRuntimeDriftTuning()
{
	if (!bEnableRuntimeDriftTuning)
	{
		return;
	}

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC || !PC->IsLocalController())
	{
		return;
	}

	ULocalPlayer* LP = PC->GetLocalPlayer();
	if (!LP || LP->GetControllerId() != 0)
	{
		return;
	}

	if (PC->WasInputKeyJustPressed(EKeys::K))
	{
		CycleRuntimeDriftTuningParam(-1);
		ShowRuntimeDriftTuningMessage();
	}

	if (PC->WasInputKeyJustPressed(EKeys::L))
	{
		CycleRuntimeDriftTuningParam(+1);
		ShowRuntimeDriftTuningMessage();
	}

	if (PC->WasInputKeyJustPressed(EKeys::P))
	{
		AdjustRuntimeDriftTuningValue(+1.f);
		ShowRuntimeDriftTuningMessage();
	}

	if (PC->WasInputKeyJustPressed(EKeys::O))
	{
		AdjustRuntimeDriftTuningValue(-1.f);
		ShowRuntimeDriftTuningMessage();
	}

	if (PC->WasInputKeyJustPressed(EKeys::I))
	{
		ShowRuntimeDriftTuningMessage();
	}
}

void ASTR_RacerPawn::CycleRuntimeDriftTuningParam(int32 Direction)
{
	const int32 Count = static_cast<int32>(EDriftTuningParam::Count);
	int32 NewIndex = static_cast<int32>(SelectedDriftTuningParam) + Direction;

	if (NewIndex < 0)
	{
		NewIndex = Count - 1;
	}
	else if (NewIndex >= Count)
	{
		NewIndex = 0;
	}

	SelectedDriftTuningParam = static_cast<EDriftTuningParam>(NewIndex);
}

void ASTR_RacerPawn::AdjustRuntimeDriftTuningValue(float Direction)
{
	switch (SelectedDriftTuningParam)
	{
	case EDriftTuningParam::TurnRateMultiplier:
		DriftTurnRateMultiplier = FMath::Max(0.f, DriftTurnRateMultiplier + 0.5f * Direction);
		break;

	case EDriftTuningParam::BaseAutoSteer:
		DriftBaseAutoSteer = FMath::Clamp(DriftBaseAutoSteer + 0.05f * Direction, 0.f, 1.f);
		break;

	case EDriftTuningParam::SameDirectionMultiplier:
		DriftSteerSameDirectionMultiplier = FMath::Clamp(DriftSteerSameDirectionMultiplier + 0.05f * Direction, 0.f, 2.f);
		break;

	case EDriftTuningParam::OppositeDirectionMultiplier:
		DriftSteerOppositeDirectionMultiplier = FMath::Clamp(DriftSteerOppositeDirectionMultiplier + 0.02f * Direction, 0.f, 1.f);
		break;

	case EDriftTuningParam::DriftGrip:
		DriftGrip = FMath::Max(0.1f, DriftGrip + 0.5f * Direction);
		break;

	case EDriftTuningParam::MaxDriftAngle:
		MaxDriftAngle = FMath::Clamp(MaxDriftAngle + 1.f * Direction, 0.f, 60.f);
		break;

	case EDriftTuningParam::DriftSpeedLossPerSecond:
		DriftSpeedLossPerSecond = FMath::Max(0.f, DriftSpeedLossPerSecond + 25.f * Direction);
		break;

	case EDriftTuningParam::DriftAccelMultiplier:
		DriftAccelMultiplier = FMath::Clamp(DriftAccelMultiplier + 0.05f * Direction, 0.f, 2.f);
		break;

	case EDriftTuningParam::MinSpeedToStartDrift:
		MinSpeedToStartDrift = FMath::Max(0.f, MinSpeedToStartDrift + 25.f * Direction);
		break;

	default:
		break;
	}
}

FString ASTR_RacerPawn::GetRuntimeDriftTuningLabel() const
{
	switch (SelectedDriftTuningParam)
	{
	case EDriftTuningParam::TurnRateMultiplier:
		return TEXT("DriftTurnRateMultiplier");

	case EDriftTuningParam::BaseAutoSteer:
		return TEXT("DriftBaseAutoSteer");

	case EDriftTuningParam::SameDirectionMultiplier:
		return TEXT("DriftSteerSameDirectionMultiplier");

	case EDriftTuningParam::OppositeDirectionMultiplier:
		return TEXT("DriftSteerOppositeDirectionMultiplier");

	case EDriftTuningParam::DriftGrip:
		return TEXT("DriftGrip");

	case EDriftTuningParam::MaxDriftAngle:
		return TEXT("MaxDriftAngle");

	case EDriftTuningParam::DriftSpeedLossPerSecond:
		return TEXT("DriftSpeedLossPerSecond");

	case EDriftTuningParam::DriftAccelMultiplier:
		return TEXT("DriftAccelMultiplier");

	case EDriftTuningParam::MinSpeedToStartDrift:
		return TEXT("MinSpeedToStartDrift");

	default:
		return TEXT("Unknown");
	}
}

float ASTR_RacerPawn::GetRuntimeDriftTuningValue() const
{
	switch (SelectedDriftTuningParam)
	{
	case EDriftTuningParam::TurnRateMultiplier:
		return DriftTurnRateMultiplier;

	case EDriftTuningParam::BaseAutoSteer:
		return DriftBaseAutoSteer;

	case EDriftTuningParam::SameDirectionMultiplier:
		return DriftSteerSameDirectionMultiplier;

	case EDriftTuningParam::OppositeDirectionMultiplier:
		return DriftSteerOppositeDirectionMultiplier;

	case EDriftTuningParam::DriftGrip:
		return DriftGrip;

	case EDriftTuningParam::MaxDriftAngle:
		return MaxDriftAngle;

	case EDriftTuningParam::DriftSpeedLossPerSecond:
		return DriftSpeedLossPerSecond;

	case EDriftTuningParam::DriftAccelMultiplier:
		return DriftAccelMultiplier;

	case EDriftTuningParam::MinSpeedToStartDrift:
		return MinSpeedToStartDrift;

	default:
		return 0.f;
	}
}

void ASTR_RacerPawn::ShowRuntimeDriftTuningMessage() const
{
	const FString Msg = FString::Printf(
		TEXT("[DRIFT TUNING] %s = %.2f | K/L: select | O/P: change | I: show"),
		*GetRuntimeDriftTuningLabel(),
		GetRuntimeDriftTuningValue()
	);

	UE_LOG(LogTemp, Warning, TEXT("%s"), *Msg);

	if (GEngine && bShowRuntimeDriftTuningOnScreen)
	{
		GEngine->AddOnScreenDebugMessage(
			424242,
			2.0f,
			FColor::Cyan,
			Msg
		);
	}
}

void ASTR_RacerPawn::BeginPostDriftRecovery()
{
	bPostDriftRecoveryActive = true;
	PostDriftRecoveryTimer = PostDriftRecoveryDuration;
}

float ASTR_RacerPawn::GetPostDriftSteeringInput(float RawSteeringInput) const
{
	if (!bPostDriftRecoveryActive || LastDriftDirection == 0)
	{
		return RawSteeringInput;
	}

	const float Alpha = 1.f - (PostDriftRecoveryTimer / FMath::Max(PostDriftRecoveryDuration, KINDA_SMALL_NUMBER));
	const float RecoveryStrength = 1.f - Alpha; // fort au debut, puis diminue

	const float SteeringVsLastDrift = RawSteeringInput * LastDriftDirection;

	float Multiplier = 1.f;

	if (SteeringVsLastDrift > 0.f)
	{
		// meme sens que le drift qu'on vient de quitter = gros frein
		Multiplier = FMath::Lerp(PostDriftSteerSameDirectionMultiplier, 1.f, Alpha);
	}
	else if (SteeringVsLastDrift < 0.f)
	{
		// contre steer / correction = presque libre
		Multiplier = FMath::Lerp(PostDriftSteerOppositeDirectionMultiplier, 1.f, Alpha);
	}

	return RawSteeringInput * Multiplier;
}

ATrackSplineActor* ASTR_RacerPawn::ResolveTrackSplineActor()
{
	if (CachedTracks.Num() == 0)
	{
		return nullptr;
	}

	ATrackSplineActor* ClosestTrack = nullptr;
	float BestDistance = BIG_NUMBER;

	for (ATrackSplineActor* Track : CachedTracks)
	{
		if (!Track) continue;

		const float Dist = Track->GetDistanceFromTrackCenter2D(GetActorLocation());

		if (Dist < BestDistance)
		{
			BestDistance = Dist;
			ClosestTrack = Track;
		}
	}

	return ClosestTrack;
}

int32 ASTR_RacerPawn::GetLocalPlayerIndex() const
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return -1;
	}

	ULocalPlayer* LP = PC->GetLocalPlayer();
	if (!LP)
	{
		return -1;
	}

	return LP->GetControllerId();
}