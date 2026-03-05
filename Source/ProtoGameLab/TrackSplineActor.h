// TrackSplineActor.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TrackSplineActor.generated.h"

class USplineComponent;
class USplineMeshComponent;
class UStaticMesh;

UCLASS()
class PROTOGAMELAB_API ATrackSplineActor : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ATrackSplineActor();

#if WITH_EDITOR
	virtual void OnConstruction(const FTransform& Transform) override;
#endif

	// Route
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Track")
	TObjectPtr<USplineComponent> Spline;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Track|Road")
	TObjectPtr<UStaticMesh> RoadMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Track|Road")
	TObjectPtr<UMaterialInterface> RoadMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Track|Road")
	bool bClosedLoop = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Track|Road")
	bool bCollisionEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Track|Road")
	float RoadWidthScale = 1.0f;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USplineMeshComponent>> RoadSegments;

	void ClearGenerated();
	void BuildRoad();
};