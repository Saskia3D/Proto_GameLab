#include "RaceMinimapWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/SplineComponent.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Rendering/DrawElements.h"
#include "STR_RacerPawn.h"
#include "Styling/CoreStyle.h"
#include "TrackSplineActor.h"

void URaceMinimapWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (WidgetTree && !WidgetTree->RootWidget)
	{
		WidgetTree->RootWidget = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("MinimapRoot"));
	}

	SetVisibility(ESlateVisibility::HitTestInvisible);
	RefreshTrackCache();
}

void URaceMinimapWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!CachedTrackSpline || CachedSplineSamples.IsEmpty())
	{
		RefreshTrackCache();
	}
}

void URaceMinimapWidget::RefreshTrackCache()
{
	CachedSplineSamples.Reset();
	CachedWorldBounds = FBox2D(EForceInit::ForceInit);
	bHasValidTrackCache = false;

	if (!CachedTrackSpline || !CachedTrackSpline->Spline)
	{
		return;
	}

	const float SplineLength = CachedTrackSpline->Spline->GetSplineLength();
	if (SplineLength <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const int32 SampleCount = 160;
	CachedSplineSamples.Reserve(SampleCount + 1);

	for (int32 SampleIndex = 0; SampleIndex <= SampleCount; ++SampleIndex)
	{
		const float Alpha = static_cast<float>(SampleIndex) / static_cast<float>(SampleCount);
		const float Distance = Alpha * SplineLength;
		const FVector SampleLocation = CachedTrackSpline->Spline->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);

		CachedSplineSamples.Add(SampleLocation);
		CachedWorldBounds += ProjectWorldToTrackSpace(SampleLocation);
	}

	if (CachedWorldBounds.bIsValid)
	{
		FVector2D BoundsSize = CachedWorldBounds.GetSize();

		// Avoid degenerate projections when the track is very flat on one axis.
		if (BoundsSize.X < 100.f)
		{
			CachedWorldBounds.Min.X -= 50.f;
			CachedWorldBounds.Max.X += 50.f;
		}

		if (BoundsSize.Y < 100.f)
		{
			CachedWorldBounds.Min.Y -= 50.f;
			CachedWorldBounds.Max.Y += 50.f;
		}

		bHasValidTrackCache = true;
	}
}

FVector2D URaceMinimapWidget::ProjectWorldToTrackSpace(const FVector& WorldLocation) const
{
	if (bUseCameraAlignedAxes)
	{
		// Align the minimap with the top-down camera so the track feels consistent
		// with the player's screen-space left/right movement.
		return FVector2D(WorldLocation.Y, WorldLocation.X);
	}

	return FVector2D(WorldLocation.X, WorldLocation.Y);
}

FVector2D URaceMinimapWidget::ProjectWorldToMinimap(const FVector& WorldLocation, const FVector2D& BoxOrigin, const FVector2D& BoxSize) const
{
	if (!bHasValidTrackCache)
	{
		return BoxOrigin + (BoxSize * 0.5f);
	}

	const FVector2D ContentOrigin = BoxOrigin + FVector2D(InnerPadding, InnerPadding);
	const FVector2D ContentSize = BoxSize - FVector2D(InnerPadding * 2.f, InnerPadding * 2.f);
	const FVector2D BoundsSize = CachedWorldBounds.GetSize();

	if (BoundsSize.X <= KINDA_SMALL_NUMBER || BoundsSize.Y <= KINDA_SMALL_NUMBER)
	{
		return ContentOrigin + (ContentSize * 0.5f);
	}

	const float Scale = FMath::Min(ContentSize.X / BoundsSize.X, ContentSize.Y / BoundsSize.Y);
	const FVector2D ScaledBoundsSize = BoundsSize * Scale;
	const FVector2D CenteringOffset = (ContentSize - ScaledBoundsSize) * 0.5f;

	const FVector2D TrackSpaceLocation = ProjectWorldToTrackSpace(WorldLocation);
	float NormalizedX = (TrackSpaceLocation.X - CachedWorldBounds.Min.X) / BoundsSize.X;
	float NormalizedY = (TrackSpaceLocation.Y - CachedWorldBounds.Min.Y) / BoundsSize.Y;
	//float NormalizedX = 1.f - ((TrackSpaceLocation.X - CachedWorldBounds.Min.X) / BoundsSize.X);
	//float NormalizedY = 1.f - ((TrackSpaceLocation.Y - CachedWorldBounds.Min.Y) / BoundsSize.Y);

	if (bFlipHorizontally)
	{
		NormalizedX = 1.f - NormalizedX;
	}

	if (bFlipVertically)
	{
		NormalizedY = 1.f - NormalizedY;
	}

	NormalizedX = FMath::Clamp(NormalizedX, 0.f, 1.f);
	NormalizedY = FMath::Clamp(NormalizedY, 0.f, 1.f);

	return ContentOrigin + CenteringOffset + FVector2D(NormalizedX * ScaledBoundsSize.X, NormalizedY * ScaledBoundsSize.Y);
}

FVector2D URaceMinimapWidget::ResolveMinimapOrigin(const FVector2D& ViewSize, const FVector2D& BoxSize) const
{
	FVector2D Origin(HorizontalPadding, VerticalPadding);

	switch (ScreenCorner)
	{
	case EMinimapScreenCorner::TopRight:
		Origin = FVector2D(ViewSize.X - HorizontalPadding - BoxSize.X, VerticalPadding);
		break;
	case EMinimapScreenCorner::BottomLeft:
		Origin = FVector2D(HorizontalPadding, ViewSize.Y - VerticalPadding - BoxSize.Y);
		break;
	case EMinimapScreenCorner::BottomRight:
		Origin = FVector2D(ViewSize.X - HorizontalPadding - BoxSize.X, ViewSize.Y - VerticalPadding - BoxSize.Y);
		break;

	case EMinimapScreenCorner::BottomCenter: 
		Origin = FVector2D((ViewSize.X - BoxSize.X) * 0.5f,ViewSize.Y - BoxSize.Y - VerticalPadding);
		break;
	case EMinimapScreenCorner::SeamCenter:
		// Center of minimap sits exactly on the vertical split line (right edge of P1's viewport)
		Origin = FVector2D(ViewSize.X - (BoxSize.X * 0.5f), ViewSize.Y - BoxSize.Y - VerticalPadding);
		break;
	case EMinimapScreenCorner::TopLeft:
	default:
		break;
	}

	return Origin;
}

/*int32 URaceMinimapWidget::NativePaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled
) const
{
	const FVector2D ViewSize = AllottedGeometry.GetLocalSize();
	if (ViewSize.X <= KINDA_SMALL_NUMBER || ViewSize.Y <= KINDA_SMALL_NUMBER)
	{
		return Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	}
	// --- REPLACE THIS SECTION IN NATIVEPAINT ---
	const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
	if (LocalSize.X <= KINDA_SMALL_NUMBER || LocalSize.Y <= KINDA_SMALL_NUMBER)
	{
		return Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	}

	// 1. Force use of the FULL screen resolution instead of just this player's split
	FVector2D FullViewportSize = LocalSize;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(FullViewportSize);
	}

	// 2. Find where THIS player's splitscreen starts on the screen
	FVector2D PlayerViewportOffset(0.f, 0.f);
	if (const ULocalPlayer* LP = GetOwningLocalPlayer())
	{
		PlayerViewportOffset.X = LP->Origin.X * FullViewportSize.X;
		PlayerViewportOffset.Y = LP->Origin.Y * FullViewportSize.Y;
	}

	const FVector2D BoxSize(MinimapSize, MinimapSize);

	// 3. Resolve the center-bottom of the WHOLE monitor, then subtract the local offset!
	const FVector2D BoxOrigin = ResolveMinimapOrigin(FullViewportSize, BoxSize) - PlayerViewportOffset;

	// (Leave the rest of NativePaint below this exactly as it was)
	const FVector2D BoxSize(MinimapSize, MinimapSize);
	const FVector2D BoxOrigin = ResolveMinimapOrigin(ViewSize, BoxSize);

	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId,
		AllottedGeometry.ToPaintGeometry(FVector2f(BoxSize), FSlateLayoutTransform(FVector2f(BoxOrigin))),
		WhiteBrush,
		ESlateDrawEffect::None,
		BackgroundColor
	);

	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId + 1,
		AllottedGeometry.ToPaintGeometry(FVector2f(BoxSize), FSlateLayoutTransform(FVector2f(BoxOrigin))),
		WhiteBrush,
		ESlateDrawEffect::None,
		FLinearColor(FrameColor.R, FrameColor.G, FrameColor.B, 0.18f)
	);

	if (bHasValidTrackCache && CachedSplineSamples.Num() > 1)
	{
		TArray<FVector2D> TrackPoints;
		TrackPoints.Reserve(CachedSplineSamples.Num());

		for (const FVector& SampleLocation : CachedSplineSamples)
		{
			TrackPoints.Add(ProjectWorldToMinimap(SampleLocation, BoxOrigin, BoxSize));
		}

		FSlateDrawElement::MakeLines(
			OutDrawElements,
			LayerId + 2,
			AllottedGeometry.ToPaintGeometry(),
			TrackPoints,
			ESlateDrawEffect::None,
			TrackColor,
			true,
			TrackThickness
		);
	}

	TArray<AActor*> RacerActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASTR_RacerPawn::StaticClass(), RacerActors);

	for (AActor* RacerActor : RacerActors)
	{
		const ASTR_RacerPawn* RacerPawn = Cast<ASTR_RacerPawn>(RacerActor);
		if (!RacerPawn)
		{
			continue;
		}

		const AController* Controller = RacerPawn->GetController();
		const bool bIsPlayer = Controller && Controller->IsPlayerController();
		const bool bIsLocalPlayer = bIsPlayer && Controller->IsLocalController();

		const FLinearColor MarkerColor = bIsLocalPlayer
			? LocalPlayerColor
			: (bIsPlayer ? RemotePlayerColor : AIColor);

		const FVector2D MarkerCenter = ProjectWorldToMinimap(RacerPawn->GetActorLocation(), BoxOrigin, BoxSize);
		const FVector2D MarkerTopLeft = MarkerCenter - FVector2D(PlayerMarkerSize * 0.5f, PlayerMarkerSize * 0.5f);

		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId + 3,
			AllottedGeometry.ToPaintGeometry(FVector2f(PlayerMarkerSize, PlayerMarkerSize), FSlateLayoutTransform(FVector2f(MarkerTopLeft))),
			WhiteBrush,
			ESlateDrawEffect::None,
			MarkerColor
		);

		const FVector DirectionWorld = RacerPawn->GetActorLocation() + (RacerPawn->GetActorForwardVector() * 250.f);
		FVector2D DirectionTip = ProjectWorldToMinimap(DirectionWorld, BoxOrigin, BoxSize);
		const FVector2D DirectionVector = DirectionTip - MarkerCenter;
		if (!DirectionVector.IsNearlyZero())
		{
			DirectionTip = MarkerCenter + DirectionVector.GetSafeNormal() * DirectionLineLength;
		}

		TArray<FVector2D> DirectionLine;
		DirectionLine.Add(MarkerCenter);
		DirectionLine.Add(DirectionTip);

		FSlateDrawElement::MakeLines(
			OutDrawElements,
			LayerId + 4,
			AllottedGeometry.ToPaintGeometry(),
			DirectionLine,
			ESlateDrawEffect::None,
			FLinearColor(MarkerColor.R, MarkerColor.G, MarkerColor.B, 0.85f),
			true,
			2.f
		);
	}

	return Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId + 5, InWidgetStyle, bParentEnabled);
}*/
int32 URaceMinimapWidget::NativePaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled
) const
{
	const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
	if (LocalSize.X <= KINDA_SMALL_NUMBER || LocalSize.Y <= KINDA_SMALL_NUMBER)
	{
		return Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	}

	// 1. Get the FULL screen resolution
	FVector2D FullViewportSize = LocalSize;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(FullViewportSize);
	}

	// 2. Get this player's offset
	FVector2D PlayerViewportOffset(0.f, 0.f);
	if (const ULocalPlayer* LP = GetOwningLocalPlayer())
	{
		PlayerViewportOffset.X = LP->Origin.X * FullViewportSize.X;
		PlayerViewportOffset.Y = LP->Origin.Y * FullViewportSize.Y;
	}

	// 3. Define the Minimap geometry
	const FVector2D BoxSize(MinimapSize, MinimapSize);
	const FVector2D BoxOrigin = ResolveMinimapOrigin(FullViewportSize, BoxSize) - PlayerViewportOffset;

	// 4. Get the WhiteBrush (Fixes your compiler error)
	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("WhiteBrush");

	OutDrawElements.PushClip(FSlateClippingZone(AllottedGeometry.GetLayoutBoundingRect().ExtendBy(FMargin(10000.f))));
	// --- DRAW BACKGROUND ---
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId,
		AllottedGeometry.ToPaintGeometry(FVector2f(BoxSize), FSlateLayoutTransform(FVector2f(BoxOrigin))),
		WhiteBrush,
		ESlateDrawEffect::None,
		BackgroundColor
	);

	// --- DRAW FRAME ---
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId + 1,
		AllottedGeometry.ToPaintGeometry(FVector2f(BoxSize), FSlateLayoutTransform(FVector2f(BoxOrigin))),
		WhiteBrush,
		ESlateDrawEffect::None,
		FLinearColor(FrameColor.R, FrameColor.G, FrameColor.B, 0.18f)
	);

	// --- DRAW TRACK ---
	if (bHasValidTrackCache && CachedSplineSamples.Num() > 1)
	{
		TArray<FVector2D> TrackPoints;
		TrackPoints.Reserve(CachedSplineSamples.Num());

		for (const FVector& SampleLocation : CachedSplineSamples)
		{
			TrackPoints.Add(ProjectWorldToMinimap(SampleLocation, BoxOrigin, BoxSize));
		}

		FSlateDrawElement::MakeLines(
			OutDrawElements,
			LayerId + 2,
			AllottedGeometry.ToPaintGeometry(),
			TrackPoints,
			ESlateDrawEffect::None,
			TrackColor,
			true,
			TrackThickness
		);
	}

	// --- DRAW RACERS ---
	TArray<AActor*> RacerActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASTR_RacerPawn::StaticClass(), RacerActors);

	for (AActor* RacerActor : RacerActors)
	{
		const ASTR_RacerPawn* RacerPawn = Cast<ASTR_RacerPawn>(RacerActor);
		if (!RacerPawn) continue;

		const AController* Controller = RacerPawn->GetController();
		const bool bIsPlayer = Controller && Controller->IsPlayerController();
		const bool bIsLocalPlayer = bIsPlayer && Controller->IsLocalController();

		const FLinearColor MarkerColor = bIsLocalPlayer ? LocalPlayerColor : (bIsPlayer ? RemotePlayerColor : AIColor);
		const FVector2D MarkerCenter = ProjectWorldToMinimap(RacerPawn->GetActorLocation(), BoxOrigin, BoxSize);
		const FVector2D MarkerTopLeft = MarkerCenter - FVector2D(PlayerMarkerSize * 0.5f, PlayerMarkerSize * 0.5f);

		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId + 3,
			AllottedGeometry.ToPaintGeometry(FVector2f(PlayerMarkerSize, PlayerMarkerSize), FSlateLayoutTransform(FVector2f(MarkerTopLeft))),
			WhiteBrush,
			ESlateDrawEffect::None,
			MarkerColor
		);

		// Direction Indicator
		const FVector DirectionWorld = RacerPawn->GetActorLocation() + (RacerPawn->GetActorForwardVector() * 250.f);
		FVector2D DirectionTip = ProjectWorldToMinimap(DirectionWorld, BoxOrigin, BoxSize);
		const FVector2D DirectionVector = DirectionTip - MarkerCenter;
		if (!DirectionVector.IsNearlyZero())
		{
			DirectionTip = MarkerCenter + DirectionVector.GetSafeNormal() * DirectionLineLength;
		}

		TArray<FVector2D> DirectionLine;
		DirectionLine.Add(MarkerCenter);
		DirectionLine.Add(DirectionTip);

		FSlateDrawElement::MakeLines(
			OutDrawElements,
			LayerId + 4,
			AllottedGeometry.ToPaintGeometry(),
			DirectionLine,
			ESlateDrawEffect::None,
			FLinearColor(MarkerColor.R, MarkerColor.G, MarkerColor.B, 0.85f),
			true,
			2.f
		);
	}
	OutDrawElements.PopClip();

	return Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId + 5, InWidgetStyle, bParentEnabled);
}

void URaceMinimapWidget::SetTrackSplineActor(ATrackSplineActor* InTrackSpline)
{
	CachedTrackSpline = InTrackSpline;
	RefreshTrackCache();
}