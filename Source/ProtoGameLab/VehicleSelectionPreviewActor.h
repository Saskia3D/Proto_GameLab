#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Math/BoxSphereBounds.h"
#include "VehicleSelectionPreviewActor.generated.h"

class URotatingMovementComponent;
class USceneComponent;
class UMaterialInterface;
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

	UFUNCTION(BlueprintCallable, Category = "Preview")
	void SetPreviewRelativeTransform(const FVector& InLocation, const FRotator& InRotation, const FVector& InScale);

	UFUNCTION(BlueprintCallable, Category = "Preview")
	void SetPreviewMaterials(const TArray<UMaterialInterface*>& InMaterials);

	UFUNCTION(BlueprintPure, Category = "Preview")
	UStaticMeshComponent* GetPreviewMeshComponent() const { return PreviewMeshComponent; }

	FBoxSphereBounds GetPreviewBounds() const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Preview")
	TObjectPtr<USceneComponent> SceneRoot = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Preview")
	TObjectPtr<UStaticMeshComponent> PreviewMeshComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Preview")
	TObjectPtr<URotatingMovementComponent> RotatingMovementComponent = nullptr;
};
