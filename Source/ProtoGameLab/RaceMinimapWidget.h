#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RaceMinimapWidget.generated.h"

class ATrackSplineActor;

UENUM(BlueprintType)
enum class EMinimapScreenCorner : uint8
{
	TopLeft,
	TopRight,
	BottomLeft,
	BottomRight,
	BottomCenter,
	SeamCenter
};

UCLASS()
class PROTOGAMELAB_API URaceMinimapWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual int32 NativePaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled
	) const override;

	// ---  EVENTS FOR BLUEPRINT (Sounds/Animations) ---

	/** Call this in C++ to trigger a sound or animation in the WBP */
	UFUNCTION(BlueprintImplementableEvent, Category = "Minimap|Events")
	void OnMinimapShow();

	/** Call this in C++ when the race starts to trigger UI effects */
	UFUNCTION(BlueprintImplementableEvent, Category = "Minimap|Events")
	void OnMinimapHide();

	// --- PROPERTIES ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	float MinimapSize = 190.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Layout")
	EMinimapScreenCorner ScreenCorner = EMinimapScreenCorner::SeamCenter;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Layout")
	float HorizontalPadding = 28.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Layout")
	float VerticalPadding = 28.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Layout")
	bool bUseCameraAlignedAxes = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Layout")
	bool bFlipHorizontally = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Layout")
	bool bFlipVertically = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	float InnerPadding = 18.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	float TrackThickness = 3.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	float PlayerMarkerSize = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	float DirectionLineLength = 16.f;

	// We use BlueprintReadWrite so your "Pixelation Material" logic can access these colors
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Colors")
	FLinearColor BackgroundColor = FLinearColor(0.02f, 0.03f, 0.05f, 0.80f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Colors")
	FLinearColor FrameColor = FLinearColor(0.96f, 0.74f, 0.28f, 0.95f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Colors")
	FLinearColor TrackColor = FLinearColor(0.73f, 0.82f, 0.92f, 0.95f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Colors")
	FLinearColor LocalPlayerColor = FLinearColor(1.0f, 0.85f, 0.30f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Colors")
	FLinearColor RemotePlayerColor = FLinearColor(0.90f, 0.94f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Colors")
	FLinearColor AIColor = FLinearColor(0.42f, 0.90f, 0.82f, 1.0f);

	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void SetTrackSplineActor(ATrackSplineActor* InTrackSpline);

private:
	UPROPERTY(Transient)
	TObjectPtr<ATrackSplineActor> CachedTrackSpline = nullptr;

	TArray<FVector> CachedSplineSamples;
	FBox2D CachedWorldBounds;
	bool bHasValidTrackCache = false;

	void RefreshTrackCache();
	FVector2D ProjectWorldToTrackSpace(const FVector& WorldLocation) const;
	FVector2D ProjectWorldToMinimap(const FVector& WorldLocation, const FVector2D& BoxOrigin, const FVector2D& BoxSize) const;
	FVector2D ResolveMinimapOrigin(const FVector2D& ViewSize, const FVector2D& BoxSize) const;
};