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
}

void AProjectileActor::InitDirection(FVector Direction)
{
    MoveDirection = Direction.GetSafeNormal();
}

void AProjectileActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    SetActorLocation(GetActorLocation() + MoveDirection * Speed * DeltaTime);
}

void AProjectileActor::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, FVector NormalImpulse,
    const FHitResult& Hit)
{
    ASTR_RacerPawn* HitPlayer = Cast<ASTR_RacerPawn>(OtherActor);

    if (HitPlayer)
    {
        HitPlayer->MaxSpeed *= SlowMultiplier;

        FTimerHandle Timer;
        GetWorld()->GetTimerManager().SetTimer(Timer, [HitPlayer, this]()
            {
                if (HitPlayer)
                {
                    HitPlayer->MaxSpeed /= SlowMultiplier;
                }
            }, SlowDuration, false);
    }

    Destroy();
}
