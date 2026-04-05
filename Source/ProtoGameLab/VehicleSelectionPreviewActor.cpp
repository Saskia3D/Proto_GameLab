#include "VehicleSelectionPreviewActor.h"

#include "Components/MeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/RotatingMovementComponent.h"
#include "Materials/MaterialInterface.h"

AVehicleSelectionPreviewActor::AVehicleSelectionPreviewActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	PreviewMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PreviewMeshComponent"));
	PreviewMeshComponent->SetupAttachment(SceneRoot);
	PreviewMeshComponent->SetMobility(EComponentMobility::Movable);
	PreviewMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PreviewMeshComponent->SetGenerateOverlapEvents(false);
	PreviewMeshComponent->SetCastShadow(true);
	PreviewMeshComponent->SetRelativeLocation(FVector::ZeroVector);
	PreviewMeshComponent->SetRelativeRotation(FRotator::ZeroRotator);
	PreviewMeshComponent->SetRelativeScale3D(FVector(1.f, 1.f, 1.f));

	RotatingMovementComponent = CreateDefaultSubobject<URotatingMovementComponent>(TEXT("RotatingMovementComponent"));
	RotatingMovementComponent->RotationRate = FRotator::ZeroRotator;
}

void AVehicleSelectionPreviewActor::SetPreviewMesh(UStaticMesh* InMesh)
{
	if (PreviewMeshComponent)
	{
		PreviewMeshComponent->EmptyOverrideMaterials();
		PreviewMeshComponent->SetStaticMesh(InMesh);
		PreviewMeshComponent->UpdateBounds();
		PreviewMeshComponent->MarkRenderStateDirty();
	}
}

void AVehicleSelectionPreviewActor::SetPreviewRelativeTransform(const FVector& InLocation, const FRotator& InRotation, const FVector& InScale)
{
	if (!PreviewMeshComponent)
	{
		return;
	}

	PreviewMeshComponent->SetRelativeLocation(InLocation);
	PreviewMeshComponent->SetRelativeRotation(InRotation);
	PreviewMeshComponent->SetRelativeScale3D(InScale);
	PreviewMeshComponent->UpdateBounds();
	PreviewMeshComponent->MarkRenderTransformDirty();
}

void AVehicleSelectionPreviewActor::SetPreviewMaterials(const TArray<UMaterialInterface*>& InMaterials)
{
	if (!PreviewMeshComponent)
	{
		return;
	}

	PreviewMeshComponent->EmptyOverrideMaterials();

	for (int32 MaterialIndex = 0; MaterialIndex < InMaterials.Num(); ++MaterialIndex)
	{
		PreviewMeshComponent->SetMaterial(MaterialIndex, InMaterials[MaterialIndex]);
	}

	PreviewMeshComponent->MarkRenderStateDirty();
}

FBoxSphereBounds AVehicleSelectionPreviewActor::GetPreviewBounds() const
{
	return PreviewMeshComponent ? PreviewMeshComponent->Bounds : FBoxSphereBounds(EForceInit::ForceInit);
}
