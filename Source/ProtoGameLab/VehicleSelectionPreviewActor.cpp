#include "VehicleSelectionPreviewActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/RotatingMovementComponent.h"

AVehicleSelectionPreviewActor::AVehicleSelectionPreviewActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	PreviewMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PreviewMeshComponent"));
	PreviewMeshComponent->SetupAttachment(SceneRoot);
	PreviewMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PreviewMeshComponent->SetGenerateOverlapEvents(false);
	PreviewMeshComponent->SetCastShadow(true);
	PreviewMeshComponent->SetRelativeLocation(FVector(0.f, 0.f, -40.f));
	PreviewMeshComponent->SetRelativeRotation(FRotator(0.f, -90.f, 90.f));

	RotatingMovementComponent = CreateDefaultSubobject<URotatingMovementComponent>(TEXT("RotatingMovementComponent"));
	RotatingMovementComponent->RotationRate = FRotator(0.f, 22.f, 0.f);
}

void AVehicleSelectionPreviewActor::SetPreviewMesh(UStaticMesh* InMesh)
{
	if (PreviewMeshComponent)
	{
		PreviewMeshComponent->SetStaticMesh(InMesh);
	}
}
