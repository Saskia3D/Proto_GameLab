#include "STR_RacerPawn.h"

#include "BuffComponent.h"
#include "ProjectileBuff.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/SpringArmComponent.h"
#include "PaperSpriteComponent.h"
#include "RaceMinimapWidget.h"
#include "TrackSplineActor.h"
#include "Kismet/GameplayStatics.h"

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

	TrackSplineActor = Cast<ATrackSplineActor>(
		UGameplayStatics::GetActorOfClass(GetWorld(), ATrackSplineActor::StaticClass())
	);

	if (!TrackSplineActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("[OFF TRACK] No TrackSplineActor found for %s"), *GetName());
	}

	BoxComp->OnComponentHit.AddDynamic(this, &ASTR_RacerPawn::OnHit);
	EnsureMinimapWidget();
}

void ASTR_RacerPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (HitStunTimer > 0.f)
	{
		HitStunTimer -= DeltaTime;
	}

	if (TeleportFeedbackTimer > 0.f)
	{
		TeleportFeedbackTimer = FMath::Max(0.f, TeleportFeedbackTimer - DeltaTime);
	}

	UpdateOffTrackState(DeltaTime);

	UpdateWrongWayState(DeltaTime);

	float EffectiveAccelerationRate = AccelerationRate;
	float EffectiveBrakingDeceleration = BrakingDeceleration;
	float EffectiveCoastingDeceleration = CoastingDeceleration;
	float EffectiveMaxSpeed = MaxSpeed + ActiveBoostBonusSpeed;
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
	if (ActiveBoostTimer > 0.f)
	{
		ActiveBoostTimer -= DeltaTime;
		if (ActiveBoostTimer <= 0.f)
		{
			ActiveBoostTimer = 0.f;
			ActiveBoostBonusSpeed = 0.f;
		}
	}

	//Lissage du steering
	CurrentSteeringInput = FMath::FInterpTo(
		CurrentSteeringInput,
		TargetSteeringInput,
		DeltaTime,
		SteeringInterpSpeed
	);

	const bool bHasSteerForDrift = FMath::Abs(CurrentSteeringInput) >= DriftSteerThreshold;
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
	}

	if (bAllowDrift && !bIsDrifting)
	{
		if (bWantsDrift && bFastEnoughToStartDrift && bHasSteerForDrift)
		{
			bIsDrifting = true;
			DriftDirection = (CurrentSteeringInput > 0.f) ? 1 : -1;
			DriftChargeDirection = DriftDirection;
			bDriftBoostStillValid = true;
		}
	}
	else
	{
		if (!bWantsDrift || !bFastEnoughToKeepDrift)
		{
			bIsDrifting = false;
			DriftDirection = 0;
			DriftChargeDirection = 0;
		}
	}

	if (bIsDrifting)
	{
		const int32 NewSteerDirection =
			(CurrentSteeringInput > DriftDirectionSwitchThreshold) ? 1 :
			(CurrentSteeringInput < -DriftDirectionSwitchThreshold) ? -1 : 0;

		if (NewSteerDirection != 0 && DriftChargeDirection != 0 && NewSteerDirection != DriftChargeDirection)
		{
			DriftCharge = 0.f;
			DriftHeldTime = 0.f;
			DriftChargeDirection = NewSteerDirection;
			DriftDirection = NewSteerDirection;
			bDriftBoostStillValid = true;
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

	const float SpeedRatio = FMath::Clamp(CurrentSpeed / MaxSpeed, 0.f, 1.f);
	const float BaseTurnRate = FMath::Lerp(
		MaxTurnRate,
		MinTurnRateAtMaxSpeed,
		SpeedRatio
	);

	//mouvement hors drift
	if (!bIsDrifting)
	{
		if (CurrentSpeed > MinSpeedToTurn && !FMath::IsNearlyZero(CurrentSteeringInput, 0.01f))
		{
			const float YawDelta = CurrentSteeringInput * BaseTurnRate * DeltaTime;
			AddActorLocalRotation(FRotator(0.f, YawDelta, 0.f));
		}

		CurrentDriftAngle = FMath::FInterpTo(
			CurrentDriftAngle,
			0.f,
			DeltaTime,
			DriftAngleInterpSpeed
		);

		const FVector DesiredVelocity = GetActorForwardVector() * CurrentSpeed;

		// AJOUT CRUCIAL
		MoveVelocity = FMath::VInterpTo(
			MoveVelocity,
			DesiredVelocity,
			DeltaTime,
			10.f // ajuste entre 4 et 10
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
	}

	bWasDriftingLastFrame = bIsDrifting;
	LastTravelDir = CurrentTravelDir;

	const FVector Delta = MoveVelocity * DeltaTime;

	FHitResult Hit;
	BoxComp->MoveComponent(Delta, GetActorRotation(), true, &Hit);

	if (DeltaTime > 0.f)
	{
		BoxComp->ComponentVelocity = MoveVelocity;
	}

	UpdateSafeRecoveryPoint();
}

void ASTR_RacerPawn::UpdateOffTrackState(float DeltaTime)
{
	const bool bWasOffTrack = bIsOffTrack;
	const bool bWasPenaltyActive = bOffTrackPenaltyActive;

	if (!TrackSplineActor)
	{
		TrackSplineActor = Cast<ATrackSplineActor>(
			UGameplayStatics::GetActorOfClass(GetWorld(), ATrackSplineActor::StaticClass())
		);
	}

	if (!TrackSplineActor)
	{
		bIsOffTrack = false;
		bOffTrackPenaltyActive = false;
		OffTrackTime = 0.f;
		return;
	}

	bIsOffTrack = !TrackSplineActor->IsLocationOnTrack(GetActorLocation(), OffTrackDetectionMargin);

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
	if (!TrackSplineActor)
	{
		return;
	}

	const float TrackHalfWidth = TrackSplineActor->GetTrackHalfWidthWorld();
	if (TrackHalfWidth <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const float DistanceToCenter = TrackSplineActor->GetDistanceFromTrackCenter2D(GetActorLocation());

	// On n'enregistre un point sûr que si le joueur est confortablement sur la piste,
	// pas juste collé au bord.
	const bool bComfortablyOnTrack = DistanceToCenter <= (TrackHalfWidth * SafeRecoveryTrackRatio);

	if (!bComfortablyOnTrack)
	{
		return;
	}

	bHasSafeRecoveryPoint = true;
	LastSafeLocation = GetActorLocation();

	FVector SafeForward = MoveVelocity.GetSafeNormal2D();
	if (SafeForward.IsNearlyZero())
	{
		SafeForward = GetActorForwardVector().GetSafeNormal2D();
	}
	if (SafeForward.IsNearlyZero())
	{
		SafeForward = FVector::ForwardVector;
	}

	LastSafeForward = SafeForward;
	LastSafeSpeed = CurrentSpeed;
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
	const float RawSteer = Value.Get<float>();
	TargetSteeringInput = FMath::Clamp(RawSteer, -1.0f, 1.0f);

	if (FMath::Abs(TargetSteeringInput) < 0.1f)
	{
		TargetSteeringInput = 0.f;
	}
}

void ASTR_RacerPawn::UseItem(const FInputActionValue& Value)
{
	if (!BuffComponent) return;

	// Si buff projectile actif -> tirer
	for (UBuffBase* Buff : BuffComponent->ActiveBuffs)
	{
		if (UProjectileBuff* ProjectileBuff = Cast<UProjectileBuff>(Buff))
		{
			ProjectileBuff->FireProjectile();
			return;
		}
	}

	// Sinon -> activer le buff
	BuffComponent->UseBuff();

	// NOUVEAU : tirer immédiatement après activation
	for (UBuffBase* Buff : BuffComponent->ActiveBuffs)
	{
		if (UProjectileBuff* ProjectileBuff = Cast<UProjectileBuff>(Buff))
		{
			ProjectileBuff->FireProjectile();
			return;
		}
	}
}

void ASTR_RacerPawn::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	APlayerController* PC = Cast<APlayerController>(NewController);
	if (!PC) return;

	ULocalPlayer* LP = PC->GetLocalPlayer();
	if (!LP) return;

	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LP);

	if (Subsystem && DefaultMappingContext)
	{
		Subsystem->AddMappingContext(DefaultMappingContext, 0);
	}

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
		MinimapWidget->AddToPlayerScreen(40);
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
	bIsDriftButtonHeld = false;
}

void ASTR_RacerPawn::TriggerItemUse()
{
	UE_LOG(LogTemp, Warning, TEXT("[ITEM] TriggerItemUse called on %s"), *GetName());

	if (BuffComponent)
	{
		BuffComponent->UseBuff();
	}
}

bool ASTR_RacerPawn::HasBuff() const
{
	return BuffComponent && BuffComponent->CurrentBuff != nullptr;
}

void ASTR_RacerPawn::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, FVector NormalImpulse,
	const FHitResult& Hit)
{
	if (!OtherActor) return;

	// Cooldown anti spam
	if (GetWorld()->TimeSeconds - LastHitTime < HitCooldown)
		return;

	LastHitTime = GetWorld()->TimeSeconds;

	// Direction de collision
	FVector KnockbackDir = Hit.ImpactNormal;

	// Petit lift pour éviter de rester collé
	KnockbackDir.Z += 0.25f;
	KnockbackDir.Normalize();

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
	}

	// Réduction de vitesse globale
	CurrentSpeed *= 0.5f;

	// Stop drift
	bIsDrifting = false;

	// Petit stun pour éviter ré-accélération instantanée
	HitStunTimer = 0.01f;
}

void ASTR_RacerPawn::TeleportBackToTrack()
{
	if (!bHasSafeRecoveryPoint)
	{
		UE_LOG(LogTemp, Warning, TEXT("[OFF TRACK] %s has no safe recovery point"), *GetName());
		return;
	}

	FVector SafeForward = LastSafeForward.GetSafeNormal2D();
	if (SafeForward.IsNearlyZero())
	{
		SafeForward = GetActorForwardVector().GetSafeNormal2D();
	}

	if (SafeForward.IsNearlyZero())
	{
		SafeForward = FVector::ForwardVector;
	}

	const FVector NewLocation = LastSafeLocation + FVector(0.f, 0.f, RecoveryHeightOffset);
	const FRotator NewRotation = SafeForward.Rotation();

	SetActorLocationAndRotation(
		NewLocation,
		NewRotation,
		false,
		nullptr,
		ETeleportType::TeleportPhysics
	);

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

	// Reset état off-track
	bIsOffTrack = false;
	bOffTrackPenaltyActive = false;
	OffTrackTime = 0.f;

	UE_LOG(LogTemp, Warning, TEXT("[OFF TRACK] %s teleported back to LAST SAFE POINT"), *GetName());

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

void ASTR_RacerPawn::UpdateWrongWayState(float DeltaTime)
{
	const bool bWasWrongWay = bIsGoingWrongWay;
	const bool bWasWarningActive = bWrongWayWarningActive;

	if (!TrackSplineActor)
	{
		TrackSplineActor = Cast<ATrackSplineActor>(
			UGameplayStatics::GetActorOfClass(GetWorld(), ATrackSplineActor::StaticClass())
		);
	}

	if (!TrackSplineActor)
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
	const FVector TrackDirection = TrackSplineActor->GetTrackForwardDirectionAtWorldLocation(GetActorLocation());

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

void ASTR_RacerPawn::SetAutoDriveEnabled(bool bShouldAutoDrive)
{
	bAutoDriveEnabled = bShouldAutoDrive;
}

void ASTR_RacerPawn::SetDriftButtonHeld(bool bShouldHoldDrift)
{
	bIsDriftButtonHeld = bShouldHoldDrift;
}

void ASTR_RacerPawn::StartDrift(const FInputActionValue& Value)
{
	bIsDriftButtonHeld = true;
}

void ASTR_RacerPawn::StopDrift(const FInputActionValue& Value)
{
	bIsDriftButtonHeld = false;
}

