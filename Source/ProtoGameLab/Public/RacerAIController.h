// RacerAIController.h

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "RacerAIController.generated.h"

class ASTR_RacerPawn;
class ATrackSplineActor;

UCLASS()
class PROTOGAMELAB_API ARacerAIController : public AAIController
{
	GENERATED_BODY()
	

public:
	ARacerAIController();

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|Track")
	TObjectPtr<ATrackSplineActor> TrackSplineActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Track")
	bool bAutoFindTrackSpline = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Drive")
	float BaseLookAhead = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Drive")
	float LookAheadSpeedFactor = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Drive")
	float MaxLookAhead = 900.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Drive")
	float FullSteerAngleDeg = 40.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Drive")
	float BrakeAngleDeg = 28.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Drive")
	float StrongBrakeAngleDeg = 45.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Drive")
	float BrakeSpeedThreshold = 700.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Recovery")
	float StuckSpeedThreshold = 80.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Recovery")
	float StuckTimeBeforeRecovery = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Recovery")
	float RecoveryDuration = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Items")
	float TurboUseMaxAngleDeg = 12.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Items")
	float TurboUseMinSpeed = 650.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Items")
	float TimeStopUseRange = 800.f;

private:
	float LowSpeedTimer = 0.f;
	float RecoveryTimeRemaining = 0.f;
	int32 RecoverySteerSign = 1;

	void UpdateDriving(ASTR_RacerPawn* Racer, float DeltaSeconds);
	void UpdateItemUsage(ASTR_RacerPawn* Racer, float AbsAngleToTargetDeg);

	float ComputeSignedAngleDeg2D(const FVector& Forward, const FVector& ToTarget) const;
	bool IsAnotherPawnAhead(APawn* SelfPawn, float MaxDistance, float MinForwardDot) const;
};
