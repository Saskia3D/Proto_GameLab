/// TrackSplineActor.cpp

#include "TrackSplineActor.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"

// Sets default values
ATrackSplineActor::ATrackSplineActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
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
	UE_LOG(LogTemp, Warning, TEXT("TrackSplineActor OnConstruction called"));

	Spline->SetClosedLoop(bClosedLoop);

	Spline->UpdateSpline();

	ClearGenerated();
	BuildRoad();
}
#endif

void ATrackSplineActor::ClearGenerated()
{
	for (USplineMeshComponent* C : RoadSegments)
	{
		if (C) C->DestroyComponent();
	}
	RoadSegments.Reset();
}

void ATrackSplineActor::BuildRoad()
{
	if (!RoadMesh || !Spline) return;

	const int32 NumPoints = Spline->GetNumberOfSplinePoints();
	if (NumPoints < 2) return;

	auto MakeSeg = [&](int32 A, int32 B)
		{
			USplineMeshComponent* Seg = NewObject<USplineMeshComponent>(this);
			Seg->SetMobility(EComponentMobility::Movable);
			Seg->SetStartScale(FVector2D(6.f, 0.05f));
			Seg->SetEndScale(FVector2D(6.f, 0.05f));
			Seg->RegisterComponentWithWorld(GetWorld());
			Seg->AttachToComponent(Spline, FAttachmentTransformRules::KeepRelativeTransform);

			Seg->SetStaticMesh(RoadMesh);
			if (RoadMaterial) Seg->SetMaterial(0, RoadMaterial);

			const FVector StartPos = Spline->GetLocationAtSplinePoint(A, ESplineCoordinateSpace::Local);
			const FVector StartTan = Spline->GetTangentAtSplinePoint(A, ESplineCoordinateSpace::Local);
			const FVector EndPos = Spline->GetLocationAtSplinePoint(B, ESplineCoordinateSpace::Local);
			const FVector EndTan = Spline->GetTangentAtSplinePoint(B, ESplineCoordinateSpace::Local);

			Seg->SetStartAndEnd(StartPos, StartTan, EndPos, EndTan, true);
			Seg->SetForwardAxis(ESplineMeshAxis::X);

			Seg->SetCollisionEnabled(bCollisionEnabled ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
			Seg->SetCollisionProfileName(TEXT("BlockAll"));

			Seg->SetStartScale(FVector2D(RoadWidthScale, 0.05f));
			Seg->SetEndScale(FVector2D(RoadWidthScale, 0.05f));

			RoadSegments.Add(Seg);
		};

	for (int32 i = 0; i < NumPoints - 1; ++i)
	{
		MakeSeg(i, i + 1);
	}

	if (bClosedLoop)
	{
		MakeSeg(NumPoints - 1, 0);
	}
}

// Called when the game starts or when spawned
void ATrackSplineActor::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void ATrackSplineActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

