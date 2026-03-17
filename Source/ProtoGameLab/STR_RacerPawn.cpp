#include "STR_RacerPawn.h"

#include "BuffComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/SpringArmComponent.h"
#include "PaperSpriteComponent.h"

ASTR_RacerPawn::ASTR_RacerPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	// 1. Setup de la Capsule (Collision)
	CapsuleComp = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComp"));
	RootComponent = CapsuleComp;
	CapsuleComp->SetCapsuleSize(40.f, 40.f);
	CapsuleComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CapsuleComp->SetGenerateOverlapEvents(true);
	CapsuleComp->SetCollisionObjectType(ECC_Pawn);
	CapsuleComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	CapsuleComp->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	CapsuleComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CapsuleComp->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Overlap);
	CapsuleComp->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);

	// 2. Setup du Sprite
	SpriteComp = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("SpriteComp"));
	SpriteComp->SetupAttachment(RootComponent);
	SpriteComp->SetRelativeRotation(FRotator(0.0f, -90.0f, 90.0f));
	SpriteComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SpriteComp->SetGenerateOverlapEvents(false);

	// 3. Setup de la Caméra
	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComp"));
	SpringArmComp->SetupAttachment(RootComponent);
	SpringArmComp->SetUsingAbsoluteRotation(true);
	SpringArmComp->TargetArmLength = 800.0f;
	SpringArmComp->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));

	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComp"));
	CameraComp->SetupAttachment(SpringArmComp);

	BuffComponent = CreateDefaultSubobject<UBuffComponent>(TEXT("BuffComponent"));
}

void ASTR_RacerPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

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

	//variables de verifications au niveau du drift
	const bool bHasSteerForDrift = FMath::Abs(CurrentSteeringInput) >= DriftSteerThreshold;
	const bool bFastEnoughToStartDrift = CurrentSpeed >= MinSpeedToStartDrift;
	const bool bFastEnoughToKeepDrift = CurrentSpeed >= MinDriftSpeed;


	//entree en drift
	if (!bIsDrifting)
	{
		if (bIsBraking && bFastEnoughToStartDrift && bHasSteerForDrift)
		{
			bIsDrifting = true;
			DriftDirection = (CurrentSteeringInput > 0.f) ? 1 : -1; //droite = 1 , gauche = -1
		}
	}
	else
	{
		//gestion maintien du drift
		const bool bHardCounterSteer =
			(DriftDirection > 0 && CurrentSteeringInput < -DriftDirectionSwitchThreshold) ||
			(DriftDirection < 0 && CurrentSteeringInput > DriftDirectionSwitchThreshold);

		if (!bIsBraking || !bFastEnoughToKeepDrift || bHardCounterSteer)
		{
			bIsDrifting = false;
			DriftDirection = 0;
		}
	}

	if (bIsDrifting)
	{
		if (bIsAccelerating) //cas 1 : on drift + acceleration
		{
			CurrentSpeed += (AccelerationRate * DriftAccelMultiplier - DriftSpeedLossPerSecond) * DeltaTime;
		}
		else //cas 2 : on drift sans accel
		{
			CurrentSpeed -= (CoastingDeceleration + DriftSpeedLossPerSecond) * DeltaTime;
		}
	}
	else if (bIsBraking) //brake normal
	{
		CurrentSpeed -= BrakingDeceleration * DeltaTime;
	}
	else if (bIsAccelerating) //accel normal
	{
		CurrentSpeed += AccelerationRate * DeltaTime;
	}
	else //aucune action de mouvement (deceleration)
	{
		CurrentSpeed -= CoastingDeceleration * DeltaTime;
	}


	//gestion du steering a haute et basse vitesse
	const float EffectiveMaxSpeed = MaxSpeed + ActiveBoostBonusSpeed;
	CurrentSpeed = FMath::Clamp(CurrentSpeed, 0.f, EffectiveMaxSpeed);

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

		MoveVelocity = FMath::VInterpTo(
			MoveVelocity,
			DesiredVelocity,
			DeltaTime,
			NormalGrip
		);

		if (!MoveVelocity.IsNearlyZero())
		{
			MoveVelocity = MoveVelocity.GetSafeNormal() * CurrentSpeed;
		}
		else
		{
			MoveVelocity = DesiredVelocity;
		}
	}
	else //mouvement en drift
	{
		FVector TravelDir = MoveVelocity.IsNearlyZero()
			? GetActorForwardVector()
			: MoveVelocity.GetSafeNormal();

		if (CurrentSpeed > MinSpeedToTurn)
		{
			float DriftSteerInput = CurrentSteeringInput; //adapte le steering en drift

			if (FMath::Abs(DriftSteerInput) < 0.05f && DriftDirection != 0)
			{
				DriftSteerInput = 0.25f * DriftDirection;
			}

			if (!FMath::IsNearlyZero(DriftSteerInput, 0.01f))
			{
				//trajectoire
				const float DriftYawDelta = DriftSteerInput * BaseTurnRate * DriftTurnRateMultiplier * DeltaTime;
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

	if (bIsDrifting && FMath::Abs(EffectiveBoostSteerInput) < 0.05f && DriftDirection != 0)
	{
		EffectiveBoostSteerInput = 0.25f * DriftDirection;
	}

	const bool bCorrectSteerDirection =
		(DriftDirection == 0) || (FMath::Sign(EffectiveBoostSteerInput) == DriftDirection);

	const bool bRealDriftForBoost =
		bIsDrifting &&
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
		if (DriftHeldTime >= MinChargeTimeForBoost)
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
	}

	bWasDriftingLastFrame = bIsDrifting;
	LastTravelDir = CurrentTravelDir;

	const FVector Delta = MoveVelocity * DeltaTime;

	FHitResult Hit;
	CapsuleComp->MoveComponent(Delta, GetActorRotation(), true, &Hit);

	if (DeltaTime > 0.f)
	{
		CapsuleComp->ComponentVelocity = MoveVelocity;
	}
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

		if (AccelerateAction)
		{
			EnhancedInputComponent->BindAction(AccelerateAction, ETriggerEvent::Started, this, &ASTR_RacerPawn::StartAccelerate);
			EnhancedInputComponent->BindAction(AccelerateAction, ETriggerEvent::Completed, this, &ASTR_RacerPawn::StopAccelerate);
			EnhancedInputComponent->BindAction(AccelerateAction, ETriggerEvent::Canceled, this, &ASTR_RacerPawn::StopAccelerate);
		}

		if (BrakeAction)
		{
			EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Started, this, &ASTR_RacerPawn::StartBrake);
			EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Completed, this, &ASTR_RacerPawn::StopBrake);
			EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Canceled, this, &ASTR_RacerPawn::StopBrake);
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

void ASTR_RacerPawn::StartBrake(const FInputActionValue& Value)
{
	bIsBraking = true;
}

void ASTR_RacerPawn::StopBrake(const FInputActionValue& Value)
{
	bIsBraking = false;
}

void ASTR_RacerPawn::UseItem(const FInputActionValue& Value)
{
	UE_LOG(LogTemp, Warning, TEXT("[ITEM] UseItem called on %s"), *GetName());

	if (BuffComponent)
	{
		BuffComponent->UseBuff();
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
}

void ASTR_RacerPawn::UnPossessed()
{
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

void ASTR_RacerPawn::StartAccelerate(const FInputActionValue& Value)
{
	bIsAccelerating = true;
}

void ASTR_RacerPawn::StopAccelerate(const FInputActionValue& Value)
{
	bIsAccelerating = false;
}