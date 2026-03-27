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

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (PC)
	{
		PC->bAutoManageActiveCameraTarget = false;
		PC->SetViewTarget(this);
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
			return !Pawn || !Pawn->IsPlayerControlled();
		});

	if (Players.Num() == 0) return;

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