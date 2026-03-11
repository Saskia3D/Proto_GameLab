#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "CameraMan.generated.h"

UCLASS()
class PROTOGAMELAB_API ACamManager : public AActor
{
	GENERATED_BODY()

public:
	ACamManager();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere)
	USpringArmComponent* SpringArm;

	UPROPERTY(VisibleAnywhere)
	UCameraComponent* Camera;

	UPROPERTY(EditAnywhere)
	float CameraHeight = 900.f;

	UPROPERTY(EditAnywhere)
	float CameraBackOffset = 300.f;

	// Min and max zoom distance
	UPROPERTY(EditAnywhere)
	float MinZoom = 800.f;

	UPROPERTY(EditAnywhere)
	float MaxZoom = 3000.f;

	UPROPERTY(EditAnywhere)
	float ZoomMultiplier = 1.0f;
};