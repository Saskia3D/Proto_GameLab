#include "CameraMan.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "TimerManager.h"

ACamManager::ACamManager()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(SceneRoot);
	SpringArm->TargetArmLength = 0.f;
	SpringArm->bDoCollisionTest = false;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm);

	Camera->ProjectionMode = ECameraProjectionMode::Orthographic;
	Camera->OrthoWidth = 6000.f;
}

void ACamManager::BeginPlay()
{
	Super::BeginPlay();

	for (int32 i = 0; i < 2; i++)
	{
		APlayerController* PC = UGameplayStatics::GetPlayerController(this, i);
		if (PC)
		{
			PC->bAutoManageActiveCameraTarget = false;
			PC->SetViewTarget(this);
		}
	}
}

void ACamManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	TArray<AActor*> Players;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APawn::StaticClass(), Players);

	Players.RemoveAll([](AActor* Actor)
		{
			APawn* Pawn = Cast<APawn>(Actor);
			return !Pawn; //|| !Pawn->IsPlayerControlled();
		});

	if (Players.Num() == 0) return;

	if (Players.Num() < 2) return;

	AActor* P1 = Players[0];
	AActor* P2 = Players[1];

	// Distance + anticipation vitesse
	float SpeedBoost = (Cast<APawn>(P1)->GetVelocity().Size() + Cast<APawn>(P2)->GetVelocity().Size()) * 0.25f;
	float Distance = FVector::Dist(P1->GetActorLocation(), P2->GetActorLocation()) + SpeedBoost;

	// Gestion split / merge

	if (!bIsSplitScreenActive && Distance > SplitDistance)
	{
		bIsSplitScreenActive = true;

		APlayerController* PC0 = UGameplayStatics::GetPlayerController(this, 0);
		APlayerController* PC1 = UGameplayStatics::GetPlayerController(this, 1);

		if (PC0 && PC0->GetPawn())
		{
			PC0->SetViewTarget(PC0->GetPawn());
		}

		if (PC1 && PC1->GetPawn())
		{
			PC1->SetViewTarget(PC1->GetPawn());
		}

		SetActorHiddenInGame(true);
		return;
	}
	else if (bIsSplitScreenActive && Distance < MergeDistance)
	{
		bIsSplitScreenActive = false;

		for (int32 i = 0; i < 2; i++)
		{
			APlayerController* PC = UGameplayStatics::GetPlayerController(this, i);
			if (PC)
			{
				PC->SetViewTarget(this);
			}
		}

		SetActorHiddenInGame(false);
	}

	float TargetOrtho = OrthoWidth;

	if (!bIsSplitScreenActive)
	{
		// écran divisé -> chaque joueur a une moitié
		// donc on réduit pour compenser
		TargetOrtho = OrthoWidth * 0.5f;
	}

	// interpolation pour éviter un snap brutal
	Camera->OrthoWidth = FMath::FInterpTo(
		Camera->OrthoWidth,
		TargetOrtho,
		DeltaTime,
		2.0f
	);

	FVector Midpoint = FVector::ZeroVector;
	FVector AvgVelocity = FVector::ZeroVector;

	for (AActor* Player : Players)
	{
		Midpoint += Player->GetActorLocation();
		AvgVelocity += Cast<APawn>(Player)->GetVelocity();
	}

	Midpoint /= Players.Num();
	AvgVelocity /= Players.Num();

	FVector LookAhead = FVector::ZeroVector;
	if (!AvgVelocity.IsNearlyZero())
	{
		LookAhead = AvgVelocity.GetSafeNormal() * LookAheadDistance;
	}

	// Try X-back first. If wrong, switch to Y-back version below.
	//FVector TargetLocation = Midpoint + LookAhead + FVector(-CameraBackOffset, CameraSideOffset, CameraHeight);
	const FRotator FixedCamRot(CameraPitch, CameraYaw, 0.f);

	const FVector Forward2D = FRotationMatrix(FRotator(0.f, CameraYaw, 0.f)).GetUnitAxis(EAxis::X);
	const FVector Right2D = FRotationMatrix(FRotator(0.f, CameraYaw, 0.f)).GetUnitAxis(EAxis::Y);

	FVector TargetLocation =
		Midpoint
		+ LookAhead
		- (Forward2D * CameraBackOffset)
		+ (Right2D * CameraSideOffset)
		+ FVector(0.f, 0.f, CameraHeight);
	// Alternate version if your track runs along Y:
	// FVector TargetLocation = Midpoint + LookAhead + FVector(CameraSideOffset, -CameraBackOffset, CameraHeight);

	FVector NewLocation = FMath::VInterpTo(GetActorLocation(), TargetLocation, DeltaTime, FollowInterpSpeed);
	SetActorLocation(NewLocation);

	SetActorRotation(FRotator(CameraPitch, CameraYaw, 0.f));

	Camera->OrthoWidth = OrthoWidth;
}