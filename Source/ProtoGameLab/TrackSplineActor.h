
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TrackSplineActor.generated.h"

class USplineComponent;
class USplineMeshComponent;
class UStaticMesh;
class UMaterialInterface;
class AFinishLine;
class UArrowComponent;

UCLASS()
class PROTOGAMELAB_API ATrackSplineActor : public AActor
{
	GENERATED_BODY()

public:
	ATrackSplineActor();

#if WITH_EDITOR
	virtual void OnConstruction(const FTransform& Transform) override;
#endif

	// --- EXISTING ROAD SETTINGS ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Track")
	TObjectPtr<USplineComponent> Spline;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Track|Road")
	TObjectPtr<UStaticMesh> RoadMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Track|Road")
	TObjectPtr<UMaterialInterface> RoadMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Track|Road")
	bool bClosedLoop = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Track|Road")
	bool bCollisionEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Track|Road")
	float RoadWidthScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Track|Sprite")
	float SpriteWidthScale = 0.5f;

	// --- NEW SPRITE SETTINGS ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Track|Sprite")
	TObjectPtr<UStaticMesh> SpriteMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Track|Sprite")
	TObjectPtr<UMaterialInterface> SpriteMaterial = nullptr; // The road texture material

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Track|Sprite")
	float SpriteHeightOffset = 2.0f; // Lifts the sprite slightly above the road to prevent flickering

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Track|Sprite")
	float TileLength = 400.0f; // The physical length of one road tile in Unreal Units

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	UFUNCTION(BlueprintCallable, Category = "Track|Query")
	float GetTrackHalfWidthWorld() const;

	UFUNCTION(BlueprintCallable, Category = "Track|Query")
	float GetDistanceFromTrackCenter2D(const FVector& WorldLocation) const;

	UFUNCTION(BlueprintCallable, Category = "Track|Query")
	bool IsLocationOnTrack(const FVector& WorldLocation, float ExtraMargin = 0.f) const;

	UFUNCTION(BlueprintCallable, Category = "Track|Query")
	float GetClosestDistanceAlongSpline(const FVector& WorldLocation) const;

	UFUNCTION(BlueprintCallable, Category = "Track|Query")
	FVector GetClosestWorldLocationOnTrack(const FVector& WorldLocation) const;

	UFUNCTION(BlueprintCallable, Category = "Track|Query")
	float GetTrackZAtWorldLocation(const FVector& WorldLocation) const;

	UFUNCTION(BlueprintCallable, Category = "Track|Query")

	FVector GetTrackForwardDirectionAtWorldLocation(const FVector& WorldLocation) const;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Track|FinishLine")
	TSubclassOf<AFinishLine> FinishLineClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Track|FinishLine")
	float FinishLineDistance = -10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Track|FinishLine")
	float FinishLineZOffset = 2.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Track|FinishLine")
	float FinishLineYawOffset = 90.0f;

	UPROPERTY(Transient)
	TObjectPtr<AFinishLine> SpawnedFinishLine = nullptr;

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	// --- SEGMENT ARRAYS ---
	UPROPERTY(Transient)
	TArray<TObjectPtr<USplineMeshComponent>> RoadSegments;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USplineMeshComponent>> SpriteSegments; // Track the sprite layer separately

	void ClearGenerated();
	void BuildRoad();

	void BuildFinishLine();
	void ClearFinishLine();
};