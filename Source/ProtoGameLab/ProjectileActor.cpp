// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileActor.h"
#include "Components/BoxComponent.h"
#include "STR_RacerPawn.h"
#include "GameFramework/CharacterMovementComponent.h"

AProjectileActor::AProjectileActor()
{
    PrimaryActorTick.bCanEverTick = true;

    CollisionComp = CreateDefaultSubobject<UBoxComponent>(TEXT("Collision"));
    RootComponent = CollisionComp;

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    Mesh->SetupAttachment(RootComponent);

    CollisionComp->SetNotifyRigidBodyCollision(true);
    CollisionComp->OnComponentHit.AddDynamic(this, &AProjectileActor::OnHit);
}

void AProjectileActor::BeginPlay()
{
    Super::BeginPlay();

    IgnoreOwnerPawn(Cast<APawn>(GetOwner()));
}

void AProjectileActor::InitDirection(FVector Direction)
{
    MoveDirection = Direction.GetSafeNormal();
}

void AProjectileActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (Target)
    {
        FVector DirectionToTarget = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal();

        // Lerp pour un effet smooth (optionnel mais recommandé)
        MoveDirection = FMath::VInterpTo(
            MoveDirection,
            DirectionToTarget,
            DeltaTime,
            5.0f // vitesse de rotation (ajuste ici)
        ).GetSafeNormal();
    }

    SetActorLocation(GetActorLocation() + MoveDirection * Speed * DeltaTime);
}

void AProjectileActor::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, FVector NormalImpulse,
    const FHitResult& Hit)
{
    if (!OtherActor || OtherActor == this)
    {
        return;
    }

    // Ignore le lanceur
    if (OtherActor == GetOwner() || OtherActor == OwnerPawn)
    {
        UE_LOG(LogTemp, Warning, TEXT("[PROJECTILE] Ignored owner hit"));
        return;
    }

    ASTR_RacerPawn* HitPlayer = Cast<ASTR_RacerPawn>(OtherActor);

    if (HitPlayer)
    {
        UE_LOG(LogTemp, Warning, TEXT("[PROJECTILE] HIT PLAYER: %s"), *GetNameSafe(HitPlayer));

        TWeakObjectPtr<ASTR_RacerPawn> WeakHitPlayer = HitPlayer;

        HitPlayer->ApplyProjectileSlow(SlowMultiplier);

        FTimerHandle Timer;
        GetWorld()->GetTimerManager().SetTimer(
            Timer,
            [WeakHitPlayer]()
            {
                if (ASTR_RacerPawn* Player = WeakHitPlayer.Get())
                {
                    Player->ClearProjectileSlow();
                }
            },
            SlowDuration,
            false
        );
    }

    Destroy();
}

void AProjectileActor::InitHoming(APawn* InTarget)
{
    Target = InTarget;
}

void AProjectileActor::IgnoreOwnerPawn(APawn* InOwner)
{
    if (!InOwner || !CollisionComp)
    {
        return;
    }

    OwnerPawn = InOwner;
    SetOwner(InOwner);
    SetInstigator(InOwner);

    CollisionComp->IgnoreActorWhenMoving(InOwner, true);

    if (ASTR_RacerPawn* OwnerRacer = Cast<ASTR_RacerPawn>(InOwner))
    {
        if (OwnerRacer->BoxComp)
        {
            OwnerRacer->BoxComp->IgnoreActorWhenMoving(this, true);
        }
    }
}