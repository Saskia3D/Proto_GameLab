// RacerAIController.cpp

#include "RacerAIController.h"
#include "ProtoGameLab/STR_RacerPawn.h"
#include "ProtoGameLab/TrackSplineActor.h"
#include "ProtoGameLab/TurboBuff.h"
#include "ProtoGameLab/TimeStopBuff.h"
#include "ProtoGameLab/BuffComponent.h"
#include "Components/SplineComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"

ARacerAIController::ARacerAIController()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ARacerAIController::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoFindTrackSpline && !TrackSplineActor)
	{
		TrackSplineActor = Cast<ATrackSplineActor>(
			UGameplayStatics::GetActorOfClass(GetWorld(), ATrackSplineActor::StaticClass())
		);
	}

	UE_LOG(LogTemp, Warning, TEXT("[AI] TrackSplineActor = %s"), *GetNameSafe(TrackSplineActor));
}

void ARacerAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	ASTR_RacerPawn* Racer = Cast<ASTR_RacerPawn>(GetPawn());
	if (!Racer)
	{
		return;
	}

	if (!TrackSplineActor || !TrackSplineActor->Spline)
	{
		Racer->ClearDrivingInputs();
		return;
	}

	//Logique anti bloque
	if (RecoveryTimeRemaining > 0.f)
	{
		RecoveryTimeRemaining -= DeltaSeconds;

		Racer->SetSteeringInput(static_cast<float>(RecoverySteerSign));
		Racer->SetAutoDriveEnabled(true);
		Racer->SetDriftButtonHeld(false);

		if (RecoveryTimeRemaining <= 0.f)
		{
			RecoveryTimeRemaining = 0.f;
		}
		return;
	}

	if (Racer->GetCurrentSpeed() < StuckSpeedThreshold)
	{
		LowSpeedTimer += DeltaSeconds;
	}
	else
	{
		LowSpeedTimer = 0.f;
	}

	if (LowSpeedTimer >= StuckTimeBeforeRecovery)
	{
		LowSpeedTimer = 0.f;
		RecoveryTimeRemaining = RecoveryDuration;
		RecoverySteerSign *= -1;
		return;
	}

	UpdateDriving(Racer, DeltaSeconds);
}

void ARacerAIController::UpdateDriving(ASTR_RacerPawn* Racer, float DeltaSeconds)
{
	USplineComponent* SplineComp = TrackSplineActor ? TrackSplineActor->Spline : nullptr;
	if (!SplineComp || !Racer)
	{
		return;
	}

	const FVector PawnLocation = Racer->GetActorLocation();

	const FVector ClosestLocation =
		SplineComp->FindLocationClosestToWorldLocation(PawnLocation, ESplineCoordinateSpace::World);

	const FVector SplineDirection =
		SplineComp->FindDirectionClosestToWorldLocation(PawnLocation, ESplineCoordinateSpace::World).GetSafeNormal2D();
	const float LookAhead =
		FMath::Clamp(BaseLookAhead + Racer->GetCurrentSpeed() * LookAheadSpeedFactor, BaseLookAhead, MaxLookAhead);

	const FVector TargetPoint = ClosestLocation + SplineDirection * LookAhead;
	const FVector ToTarget = (TargetPoint - PawnLocation).GetSafeNormal2D();

	const float SignedAngleDeg = ComputeSignedAngleDeg2D(Racer->GetActorForwardVector(), ToTarget);
	const float AbsAngleDeg = FMath::Abs(SignedAngleDeg);

	const float SteerValue = FMath::Clamp(SignedAngleDeg / FullSteerAngleDeg, -1.f, 1.f);

	const bool bBrakeForTurn =
		(Racer->GetCurrentSpeed() >= BrakeSpeedThreshold && AbsAngleDeg >= BrakeAngleDeg);

	const bool bVerySharpTurn = AbsAngleDeg >= StrongBrakeAngleDeg;

	Racer->SetSteeringInput(SteerValue);
	Racer->SetDriftButtonHeld(bBrakeForTurn);
	Racer->SetAutoDriveEnabled(!bVerySharpTurn);

	UpdateItemUsage(Racer, AbsAngleDeg);

#if !UE_BUILD_SHIPPING
	DrawDebugSphere(GetWorld(), TargetPoint, 20.f, 8, FColor::Green, false, 0.f);
	DrawDebugLine(GetWorld(), PawnLocation, TargetPoint, FColor::Cyan, false, 0.f, 0, 2.f);
#endif
}

void ARacerAIController::UpdateItemUsage(ASTR_RacerPawn* Racer, float AbsAngleToTargetDeg)
{
	if (!Racer || !Racer->HasBuff() || !Racer->BuffComponent || !Racer->BuffComponent->CurrentBuff)
	{
		return;
	}

	UBuffBase* CurrentBuff = Racer->BuffComponent->CurrentBuff;
	if (!CurrentBuff)
	{
		return;
	}

	if (CurrentBuff->IsA(UTurboBuff::StaticClass()))
	{
		const bool bGoodStraightLine =
			AbsAngleToTargetDeg <= TurboUseMaxAngleDeg &&
			Racer->GetCurrentSpeed() >= TurboUseMinSpeed;

		if (bGoodStraightLine)
		{
			Racer->TriggerItemUse();
		}
	}
	else if (CurrentBuff->IsA(UTimeStopBuff::StaticClass()))
	{
		if (IsAnotherPawnAhead(Racer, TimeStopUseRange, 0.25f))
		{
			Racer->TriggerItemUse();
		}
	}
}

float ARacerAIController::ComputeSignedAngleDeg2D(const FVector& Forward, const FVector& ToTarget) const
{
	const FVector Fwd = Forward.GetSafeNormal2D();
	const FVector Target = ToTarget.GetSafeNormal2D();

	const float Dot = FMath::Clamp(FVector::DotProduct(Fwd, Target), -1.f, 1.f);
	const float AngleDeg = FMath::RadiansToDegrees(FMath::Acos(Dot));

	const float CrossZ = (Fwd.X * Target.Y) - (Fwd.Y * Target.X);
	const float Sign = (CrossZ >= 0.f) ? 1.f : -1.f;

	return AngleDeg * Sign;
}

bool ARacerAIController::IsAnotherPawnAhead(APawn* SelfPawn, float MaxDistance, float MinForwardDot) const
{
	if (!SelfPawn)
	{
		return false;
	}

	TArray<AActor*> FoundPawns;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APawn::StaticClass(), FoundPawns);

	const FVector SelfLocation = SelfPawn->GetActorLocation();
	const FVector SelfForward = SelfPawn->GetActorForwardVector().GetSafeNormal2D();
	const float MaxDistanceSq = MaxDistance * MaxDistance;

	for (AActor* Actor : FoundPawns)
	{
		APawn* OtherPawn = Cast<APawn>(Actor);
		if (!OtherPawn || OtherPawn == SelfPawn)
		{
			continue;
		}

		const FVector ToOther = OtherPawn->GetActorLocation() - SelfLocation;
		const FVector ToOther2D = ToOther.GetSafeNormal2D();

		if (FVector::DistSquared(SelfLocation, OtherPawn->GetActorLocation()) > MaxDistanceSq)
		{
			continue;
		}

		const float ForwardDot = FVector::DotProduct(SelfForward, ToOther2D);
		if (ForwardDot >= MinForwardDot)
		{
			return true;
		}
	}

	return false;
}