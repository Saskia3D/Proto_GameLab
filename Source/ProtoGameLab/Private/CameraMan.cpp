#include "CameraMan.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "Math/UnrealMathUtility.h"

ACamManager::ACamManager()
{
	PrimaryActorTick.bCanEverTick = true;

	// C++ constructor
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	RootComponent = SpringArm;


	// Spring arm not really needed for top-down side camera, but kept for future adjustments
	SpringArm->TargetArmLength = 1.f;
	SpringArm->bDoCollisionTest = false;

	//Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm);
	//Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
}

void ACamManager::BeginPlay()
{
	Super::BeginPlay();

	// Make this camera the active view target
	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(TimerHandle, [this]()
		{
			APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
			if (PC)
			{
				FViewTargetTransitionParams Params;
				Params.BlendTime = 0.f;

				PC->bAutoManageActiveCameraTarget = false;
				PC->SetViewTarget(this, Params);
			}

		}, 0.5f, false);
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

	// Compute midpoint of all players (works for 1 player automatically)
	FVector Midpoint = FVector::ZeroVector;
	for (AActor* Player : Players)
	{
		Midpoint += Player->GetActorLocation();
	}
	Midpoint /= Players.Num();
	Midpoint.Z += CameraHeight;

	// Determine max distance between players for zoom
	float MaxDistance = 0.f;
	if (Players.Num() > 1)
	{
		for (int i = 0; i < Players.Num(); i++)
		{
			for (int j = i + 1; j < Players.Num(); j++)
			{
				float Dist = FVector::Dist(Players[i]->GetActorLocation(), Players[j]->GetActorLocation());
				MaxDistance = FMath::Max(MaxDistance, Dist);
			}
		}
	}

	// Compute side offset for camera (if only 1 player, uses default side offset)
	float SideOffset = Players.Num() > 1
		? FMath::Clamp(MaxDistance * ZoomMultiplier, MinZoom, MaxZoom)
		: MinZoom;

	// Camera is always behind and to the side of the midpoint
	FVector CameraOffset;
	CameraOffset.X = -800.f;       // behind along track
	CameraOffset.Y = -SideOffset;  // side offset based on zoom
	CameraOffset.Z = CameraHeight; // height above track

	FVector TargetLocation = Midpoint + CameraOffset;

	// Smooth camera movement
	FVector NewLocation = FMath::VInterpTo(GetActorLocation(), TargetLocation, DeltaTime, 5.f);
	SetActorLocation(NewLocation);

	// Look at the midpoint (players)
	FRotator LookAtRotation = (Midpoint - NewLocation).Rotation();
	SetActorRotation(LookAtRotation);
}