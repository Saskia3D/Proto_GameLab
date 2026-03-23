#include "CameraMan.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "Math/UnrealMathUtility.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/PlayerController.h"

ACamManager::ACamManager()
{
    PrimaryActorTick.bCanEverTick = false;
    /*
    PrimaryActorTick.bCanEverTick = true; */

    USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RootComponent = SceneRoot;

    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
    SpringArm->SetupAttachment(SceneRoot);
    SpringArm->TargetArmLength = 0.f;
    SpringArm->bDoCollisionTest = false;

    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(SpringArm);
}

void ACamManager::BeginPlay()
{
    Super::BeginPlay();

    // Position fixe de la caméra
    SetActorLocation(FVector(24190.f, -7530.f, 13800.f));

    // Rotation top-down fixe
    SetActorRotation(FRotator(-90.f, 10.f, 0.f));

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
}

/*
void ACamManager::BeginPlay()
{
    Super::BeginPlay();

    TArray<AActor*> Starts;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), Starts);

    if (Starts.Num() >= 2)
    {
        FVector MidPoint = (Starts[0]->GetActorLocation() + Starts[1]->GetActorLocation()) / 2.f;

        SetActorLocation(MidPoint + FVector(
            0.f,
            0.f,
            2500.f
        ));

        SetActorRotation(FRotator(
            -60.f,
            0.f,
            0.f
        ));
    }

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
}*/

/*
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

    // Midpoint between all players
    FVector Midpoint = FVector::ZeroVector;
    for (AActor* Player : Players)
    {
        Midpoint += Player->GetActorLocation();
    }
    Midpoint /= Players.Num();

    // Max distance between players for zoom
    float MaxDistance = 0.f;
    for (int32 i = 0; i < Players.Num(); i++)
    {
        for (int32 j = i + 1; j < Players.Num(); j++)
        {
            float Dist = FVector::Dist(
                Players[i]->GetActorLocation(),
                Players[j]->GetActorLocation()
            );
            MaxDistance = FMath::Max(MaxDistance, Dist);
        }
    }

    float ZoomDistance = MaxDistance * 0.685f;

    
    FVector CameraOffset = FVector::ZeroVector;
    CameraOffset.X = -(MinZoom)-ZoomDistance;
    CameraOffset.Y = -(CameraBackOffset);
    CameraOffset.Z = CameraHeight;

    FVector TargetLocation = Midpoint + CameraOffset;

	// follow target location 
    FVector NewLocation = FMath::VInterpTo(GetActorLocation(), TargetLocation, DeltaTime, 5.f);
    SetActorLocation(NewLocation);

    // Always look at midpoint
    FRotator LookAtRotation = (Midpoint - NewLocation).Rotation();
    SetActorRotation(LookAtRotation);
}*/