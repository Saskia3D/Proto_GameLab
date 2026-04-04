#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VehicleSelectionPreviewActor.generated.h"

class URotatingMovementComponent;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class PROTOGAMELAB_API AVehicleSelectionPreviewActor : public AActor
{
	GENERATED_BODY()

public:
	AVehicleSelectionPreviewActor();

	UFUNCTION(BlueprintCallable, Category = "Preview")
	void SetPreviewMesh(UStaticMesh* InMesh);

	UFUNCTION(BlueprintPure, Category = "Preview")
	UStaticMeshComponent* GetPreviewMeshComponent() const { return PreviewMeshComponent; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Preview")
	TObjectPtr<USceneComponent> SceneRoot = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Preview")
	TObjectPtr<UStaticMeshComponent> PreviewMeshComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Preview")
	TObjectPtr<URotatingMovementComponent> RotatingMovementComponent = nullptr;
};
