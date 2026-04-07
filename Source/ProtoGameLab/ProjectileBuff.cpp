// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileBuff.h"
#include "ProjectileActor.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"

void UProjectileBuff::Activate(APawn* Player)
{
    Super::Activate(Player);

    RemainingShots = 1;

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Blue, TEXT("Projectile Activated!"));
    }

    // Pas de timer de durée ici
    // Le buff reste jusqu'à ce que les tirs soient utilisés
}

UProjectileBuff::UProjectileBuff()
{
    Duration = 0.f; // empêche le timer
}

void UProjectileBuff::FireProjectile()
{
    if (!CachedPlayer || RemainingShots <= 0) return;

    UWorld* World = CachedPlayer->GetWorld();
    if (!World || !ProjectileClass) return;

    // Spawn plus loin devant pour éviter collision immédiate
    FVector Forward = CachedPlayer->GetActorForwardVector();
    FVector SpawnLoc = CachedPlayer->GetActorLocation() + Forward * 600.f;
    FRotator SpawnRot = CachedPlayer->GetActorRotation();

    // Paramètres importants
    FActorSpawnParameters Params;
    Params.Owner = CachedPlayer;
    Params.Instigator = CachedPlayer->GetInstigator();

    AProjectileActor* Projectile = World->SpawnActor<AProjectileActor>(
        ProjectileClass,
        SpawnLoc,
        SpawnRot,
        Params
    );

    /*if (Projectile)
    {
        Projectile->InitDirection(Forward);
    }*/

    APawn* Target = FindTarget();

    if (Projectile)
    {
        Projectile->InitHoming(Target);
    }

    RemainingShots--;

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            -1,
            1.5f,
            FColor::Blue,
            FString::Printf(TEXT("Shots left: %d"), RemainingShots)
        );
    }

    if (RemainingShots <= 0)
    {
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Blue, TEXT("Projectile expired!"));
        }

        OnBuffExpired();
    }
}

APawn* UProjectileBuff::FindTarget()
{
    UWorld* World = GetWorld();
    if (!World) return nullptr;

    APawn* ClosestPawn = nullptr;
    float ClosestDist = FLT_MAX;

    for (TActorIterator<APawn> It(World); It; ++It)
    {
        APawn* Pawn = *It;

        if (Pawn == CachedPlayer) continue;

        float Dist = FVector::Dist(Pawn->GetActorLocation(), CachedPlayer->GetActorLocation());

        if (Dist < ClosestDist)
        {
            ClosestDist = Dist;
            ClosestPawn = Pawn;
        }
    }

    return ClosestPawn;
}