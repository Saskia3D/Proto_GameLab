#include "TrackSplineActor.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "FinishLine.h"
#include "Components/ArrowComponent.h"

ATrackSplineActor::ATrackSplineActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Spline = CreateDefaultSubobject<USplineComponent>(TEXT("Spline"));
	SetRootComponent(Spline);

	Spline->SetMobility(EComponentMobility::Movable);
	Spline->SetClosedLoop(true);
}

#if WITH_EDITOR
void ATrackSplineActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ClearGenerated();
	BuildRoad();
}
#endif

void ATrackSplineActor::ClearGenerated()
{
	for (USplineMeshComponent* Seg : RoadSegments)
	{
		if (Seg) Seg->DestroyComponent();
	}
	RoadSegments.Reset();

	for (USplineMeshComponent* Seg : SpriteSegments)
	{
		if (Seg) Seg->DestroyComponent();
	}
	SpriteSegments.Reset();
}

void ATrackSplineActor::BuildRoad()
{
	if (!Spline || !RoadMesh || !SpriteMesh) return;

	float SplineLength = Spline->GetSplineLength();
	float CurrentDistance = 0.f;

	while (CurrentDistance < SplineLength)
	{
		float NextDistance = FMath::Min(CurrentDistance + TileLength, SplineLength);

		FVector StartPos = Spline->GetLocationAtDistanceAlongSpline(CurrentDistance, ESplineCoordinateSpace::Local);
		FVector EndPos = Spline->GetLocationAtDistanceAlongSpline(NextDistance, ESplineCoordinateSpace::Local);

		FVector StartTan = Spline->GetDirectionAtDistanceAlongSpline(CurrentDistance, ESplineCoordinateSpace::Local) * TileLength;
		FVector EndTan = Spline->GetDirectionAtDistanceAlongSpline(NextDistance, ESplineCoordinateSpace::Local) * TileLength;

		// Road base
		USplineMeshComponent* RoadSeg = NewObject<USplineMeshComponent>(this);
		RoadSeg->SetFlags(RF_Transactional);// Enable track changes
		RoadSeg->SetMobility(EComponentMobility::Movable);
		RoadSeg->RegisterComponentWithWorld(GetWorld());
		RoadSeg->AttachToComponent(Spline, FAttachmentTransformRules::KeepRelativeTransform);
		RoadSeg->SetStaticMesh(RoadMesh);
		if (RoadMaterial) RoadSeg->SetMaterial(0, RoadMaterial);
		RoadSeg->SetStartAndEnd(StartPos, StartTan, EndPos, EndTan, true);
		RoadSeg->SetStartScale(FVector2D(RoadWidthScale, 0.05f));
		RoadSeg->SetEndScale(FVector2D(RoadWidthScale, 0.05f));
		RoadSeg->SetForwardAxis(ESplineMeshAxis::Y);
		RoadSegments.Add(RoadSeg);

		// Sprite top
		USplineMeshComponent* SpriteSeg = NewObject<USplineMeshComponent>(this);
		SpriteSeg->SetMobility(EComponentMobility::Movable);
		SpriteSeg->TranslucencySortPriority = 1;
		SpriteSeg->RegisterComponentWithWorld(GetWorld());
		SpriteSeg->AttachToComponent(Spline, FAttachmentTransformRules::KeepRelativeTransform);
		SpriteSeg->SetStaticMesh(SpriteMesh);
		SpriteSeg->SetRelativeRotation(FRotator(0.f, 0.f, 0.f));
		if (SpriteMaterial) SpriteSeg->SetMaterial(0, SpriteMaterial);

		FVector SpriteStart = StartPos + FVector(0, 0, SpriteHeightOffset);
		FVector SpriteEnd = EndPos + FVector(0, 0, SpriteHeightOffset);
		float SpriteWidth = RoadWidthScale * 0.4f;

		SpriteSeg->SetStartAndEnd(SpriteStart, StartTan, SpriteEnd, EndTan, true);
		SpriteSeg->SetStartScale(FVector2D(SpriteWidth, 0.05f));
		SpriteSeg->SetEndScale(FVector2D(SpriteWidth, 0.05f));
		SpriteSeg->SetForwardAxis(ESplineMeshAxis::Y);
		SpriteSeg->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SpriteSegments.Add(SpriteSeg);

		CurrentDistance = NextDistance;
		if (TileLength <= 0.f) break;
	}
}
#if WITH_EDITOR
void ATrackSplineActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	ClearGenerated();
	ClearFinishLine();
	BuildRoad();
	BuildFinishLine();
}
#endif

void ATrackSplineActor::BeginPlay()
{
	Super::BeginPlay();

	ClearGenerated();
	ClearFinishLine();
	BuildRoad();
	BuildFinishLine();
}

void ATrackSplineActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

float ATrackSplineActor::GetTrackHalfWidthWorld() const
{
	if (!RoadMesh)
	{
		return 0.f;
	}

	const FBoxSphereBounds MeshBounds = RoadMesh->GetBounds();
	const FVector ActorScale = GetActorScale3D().GetAbs();
	const float ScaleXY = FMath::Max(ActorScale.X, ActorScale.Y);

	// On suppose que la largeur utile de la route correspond à l'axe X du mesh.
	return MeshBounds.BoxExtent.X * RoadWidthScale * ScaleXY;
}

float ATrackSplineActor::GetDistanceFromTrackCenter2D(const FVector& WorldLocation) const
{
	if (!Spline)
	{
		return BIG_NUMBER;
	}

	const FVector ClosestLocation = Spline->FindLocationClosestToWorldLocation(
		WorldLocation,
		ESplineCoordinateSpace::World
	);

	return FVector::Dist2D(WorldLocation, ClosestLocation);
}

bool ATrackSplineActor::IsLocationOnTrack(const FVector& WorldLocation, float ExtraMargin) const
{
	const float HalfWidth = GetTrackHalfWidthWorld();

	if (HalfWidth <= 0.f)
	{
		return false;
	}

	const float DistanceToCenter = GetDistanceFromTrackCenter2D(WorldLocation);
	return DistanceToCenter <= (HalfWidth + ExtraMargin);
}

float ATrackSplineActor::GetClosestDistanceAlongSpline(const FVector& WorldLocation) const
{
	if (!Spline) return 0.f;

	const FVector ClosestLocation = Spline->FindLocationClosestToWorldLocation(
		WorldLocation,
		ESplineCoordinateSpace::World
	);

	return Spline->GetDistanceAlongSplineAtLocation(
		ClosestLocation,
		ESplineCoordinateSpace::World
	);
}

FVector ATrackSplineActor::GetTrackForwardDirectionAtWorldLocation(const FVector& WorldLocation) const
{
	if (!Spline) return FVector::ForwardVector;

	const float DistanceAlongSpline = GetClosestDistanceAlongSpline(WorldLocation);

	const FVector TrackDirection = Spline->GetDirectionAtDistanceAlongSpline(
		DistanceAlongSpline,
		ESplineCoordinateSpace::World
	);

	return TrackDirection.GetSafeNormal2D();
}
void ATrackSplineActor::ClearFinishLine()
{
	if (SpawnedFinishLine)
	{
		SpawnedFinishLine->Destroy();
		SpawnedFinishLine = nullptr;
	}
}
void ATrackSplineActor::BuildFinishLine()
{
	if (!Spline || !FinishLineClass)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float SplineLength = Spline->GetSplineLength();
	const float ClampedDistance = FMath::Clamp(FinishLineDistance, 0.0f, SplineLength);

	const FVector WorldLocation = Spline->GetLocationAtDistanceAlongSpline(
		ClampedDistance,
		ESplineCoordinateSpace::World
	);

	FRotator WorldRotation = Spline->GetRotationAtDistanceAlongSpline(
		ClampedDistance,
		ESplineCoordinateSpace::World
	);

	WorldLocation;
	FRotator SpawnRotation = WorldRotation;
	SpawnRotation.Yaw += FinishLineYawOffset;

	FVector SpawnLocation = WorldLocation;
	SpawnLocation.Z += FinishLineZOffset;

	SpawnedFinishLine = World->SpawnActor<AFinishLine>(
		FinishLineClass,
		SpawnLocation,
		SpawnRotation
	);

	if (SpawnedFinishLine)
	{
		if (UArrowComponent* Arrow = SpawnedFinishLine->FindComponentByClass<UArrowComponent>())
		{
			Arrow->SetWorldRotation(SpawnRotation);
		}
	}
}