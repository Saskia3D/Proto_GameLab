// Fill out your copyright notice in the Description page of Project Settings.


#include "TurboBuff.h"
//#include "MyVehiclePawn.h"
#include "STR_RacerPawn.h"
#include "NiagaraFunctionLibrary.h"
#include "ChaosWheeledVehicleMovementComponent.h"

void UTurboBuff::Activate(APawn* Player)
{
    if (!Player) return;

    CachedPlayer = Player;
    
    ASTR_RacerPawn* Racer = Cast<ASTR_RacerPawn>(Player);
    if (!Racer) return;

    /*
    UChaosWheeledVehicleMovementComponent* Movement =
        Cast<UChaosWheeledVehicleMovementComponent>(Player->GetVehicleMovementComponent());

    if (!Movement) return;*/

    CachedRacer = Racer;
    OriginalMaxSpeed = Racer->MaxSpeed;
    Racer->MaxSpeed = OriginalMaxSpeed * TurboMultiplier;

    if (Racer->BoostTrail)
    {
        ActiveBoostFX_Left = UNiagaraFunctionLibrary::SpawnSystemAttached(
            Racer->BoostTrail,
            Racer->CarMesh, // attach à la voiture
            "Exhaust_L",
            FVector(-10.f, 10.f, 0.f), // gauche
            FRotator::ZeroRotator,
            EAttachLocation::KeepRelativeOffset,
            true // auto destroy
        );

        ActiveBoostFX_Right = UNiagaraFunctionLibrary::SpawnSystemAttached(
            Racer->BoostTrail,
            Racer->CarMesh, // attach à la voiture
            "Exhaust_R",
            FVector(10.f, 10.f, 0.f), // droite
            FRotator::ZeroRotator,
            EAttachLocation::KeepRelativeOffset,
            true // auto destroy
        );

    }

    OnTurboActivated();

    /*
    // Sauvegarder valeur actuelle
    OriginalTorqueMultiplier = Movement->EngineSetup.TorqueCurve.GetRichCurveConst()->GetLastKey().Value;

    // Multiplier torque (exemple simple)
    Movement->EngineSetup.MaxTorque *= 1.5f;*/

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Blue, TEXT("Turbo Activated!"));
    }

    UE_LOG(LogTemp, Warning, TEXT("Turbo Activated!"));

    Super::Activate(Player);
}

void UTurboBuff::OnBuffExpired()
{
    /*
    if (!CachedPlayer) return;

    UChaosWheeledVehicleMovementComponent* Movement =
        Cast<UChaosWheeledVehicleMovementComponent>(CachedPlayer->GetVehicleMovementComponent());

    if (Movement)
    {
        Movement->EngineSetup.MaxTorque /= 1.5f;
    }*/

    if (ASTR_RacerPawn* Racer = CachedRacer.Get())
    {
        Racer->MaxSpeed = OriginalMaxSpeed;
    }

    if (ActiveBoostFX_Left)
    {
        ActiveBoostFX_Left->Deactivate();
    }

    if (ActiveBoostFX_Right)
    {
        ActiveBoostFX_Right->Deactivate();
    }

    OnTurboExpired();

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Blue, TEXT("Turbo Expired"));
    }

    UE_LOG(LogTemp, Warning, TEXT("Turbo Expired!"));

    Super::OnBuffExpired();
}
