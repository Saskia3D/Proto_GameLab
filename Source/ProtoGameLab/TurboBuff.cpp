// Fill out your copyright notice in the Description page of Project Settings.


#include "TurboBuff.h"
#include "MyVehiclePawn.h"
#include "ChaosWheeledVehicleMovementComponent.h"

void UTurboBuff::Activate(AMyVehiclePawn* Player)
{
    if (!Player) return;

    CachedPlayer = Player;

    UChaosWheeledVehicleMovementComponent* Movement =
        Cast<UChaosWheeledVehicleMovementComponent>(Player->GetVehicleMovementComponent());

    if (!Movement) return;

    // Sauvegarder valeur actuelle
    OriginalTorqueMultiplier = Movement->EngineSetup.TorqueCurve.GetRichCurveConst()->GetLastKey().Value;

    // Multiplier torque (exemple simple)
    Movement->EngineSetup.MaxTorque *= 1.5f;

    UE_LOG(LogTemp, Warning, TEXT("Turbo Activated!"));

    Super::Activate(Player);
}

void UTurboBuff::OnBuffExpired()
{
    if (!CachedPlayer) return;

    UChaosWheeledVehicleMovementComponent* Movement =
        Cast<UChaosWheeledVehicleMovementComponent>(CachedPlayer->GetVehicleMovementComponent());

    if (Movement)
    {
        Movement->EngineSetup.MaxTorque /= 1.5f;
    }

    UE_LOG(LogTemp, Warning, TEXT("Turbo Expired!"));

    Super::OnBuffExpired();
}
