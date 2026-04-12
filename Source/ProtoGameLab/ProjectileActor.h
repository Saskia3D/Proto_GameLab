// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectileActor.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

UCLASS()
class PROTOGAMELAB_API AProjectileActor : public AActor
{
    GENERATED_BODY()

public:
    AProjectileActor();

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere)
    UBoxComponent* CollisionComp;

    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* Mesh;

    UPROPERTY(EditAnywhere)
    float Speed = 3000.f;

    UPROPERTY(EditAnywhere)
    float SlowMultiplier = 0.5f;

    UPROPERTY(EditAnywhere)
    float SlowDuration = 2.0f;

    FVector MoveDirection;

public:
    void InitDirection(FVector Direction);

    virtual void Tick(float DeltaTime) override;

    UFUNCTION()
    void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        FVector NormalImpulse,
        const FHitResult& Hit);

    UPROPERTY()
    APawn* OwnerPawn;

    UPROPERTY()
    APawn* Target;

    UFUNCTION()
    void InitHoming(APawn* InTarget);
};
