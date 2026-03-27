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
	USceneComponent* SceneRoot;

	UPROPERTY(VisibleAnywhere)
	USpringArmComponent* SpringArm;

	UPROPERTY(VisibleAnywhere)
	UCameraComponent* Camera;

	UPROPERTY(EditAnywhere, Category = "Camera")
	float CameraPitch = -60.f;

	UPROPERTY(EditAnywhere, Category = "Camera")
	float CameraYaw = 90.f;

	UPROPERTY(EditAnywhere, Category = "Camera")
	float CameraHeight = 1800.f;

	UPROPERTY(EditAnywhere, Category = "Camera")
	float CameraBackOffset = 400.f;

	UPROPERTY(EditAnywhere, Category = "Camera")
	float CameraSideOffset = 0.f;

	UPROPERTY(EditAnywhere, Category = "Camera")
	float FollowInterpSpeed = 5.f;

	UPROPERTY(EditAnywhere, Category = "Camera")
	float LookAheadDistance = 0.f;

	UPROPERTY(EditAnywhere, Category = "Camera")
	float OrthoWidth = 6000.f;
};