// Fill out your copyright notice in the Description page of Project Settings.

#include "Navigation/WaypointComponent.h"
#include "Navigation/WaypointTargetActor.h"
#include "UI/WaypointWidget.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Camera/PlayerCameraManager.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY(LogWaypoint);

UWaypointComponent::UWaypointComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	// Run after the camera has updated so the marker never lags a frame behind.
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}

UWaypointComponent* UWaypointComponent::GetWaypointComponent(const UObject* WorldContextObject)
{
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(WorldContextObject, 0))
	{
		return PC->FindComponentByClass<UWaypointComponent>();
	}
	return nullptr;
}

void UWaypointComponent::BeginPlay()
{
	Super::BeginPlay();

	EnsureWidget();
	UpdateTickState();
	UpdateIndicatorVisibility();
}

void UWaypointComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (WaypointWidget)
	{
		WaypointWidget->RemoveFromParent();
		WaypointWidget = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

APlayerController* UWaypointComponent::GetOwningPC() const
{
	return Cast<APlayerController>(GetOwner());
}

void UWaypointComponent::EnsureWidget()
{
	if (WaypointWidget)
	{
		return;
	}

	APlayerController* PC = GetOwningPC();
	if (!PC)
	{
		UE_LOG(LogWaypoint, Warning, TEXT("[Waypoint] WaypointComponent on %s must be owned by a PlayerController"), *GetNameSafe(GetOwner()));
		return;
	}
	if (!PC->IsLocalController())
	{
		return;
	}
	if (!WaypointWidgetClass)
	{
		UE_LOG(LogWaypoint, Warning, TEXT("[Waypoint] WaypointWidgetClass is not set on %s; no indicator will be shown"), *GetNameSafe(PC));
		return;
	}

	WaypointWidget = CreateWidget<UWaypointWidget>(PC, WaypointWidgetClass);
	if (WaypointWidget)
	{
		WaypointWidget->AddToViewport(WidgetZOrder);
		UE_LOG(LogWaypoint, Log, TEXT("[Waypoint] Indicator widget %s created"), *WaypointWidgetClass->GetName());
	}
	else
	{
		UE_LOG(LogWaypoint, Error, TEXT("[Waypoint] Failed to create waypoint widget"));
	}
}

void UWaypointComponent::ResetTrackingState()
{
	bNeedsSnap = true;
	bHasPlacement = false;
	bHasOnScreenState = false;
	bLastOnScreen = true;
	DistanceTimer = 0.0f;
	LastDisplayedMeters = -1;
}

void UWaypointComponent::SetActiveWaypoint(AActor* NewTarget)
{
	if (!IsValid(NewTarget))
	{
		ClearActiveWaypoint();
		return;
	}

	ActiveTarget = NewTarget;
	bUsingStaticLocation = false;
	bHasActiveWaypoint = true;

	if (const AWaypointTargetActor* WaypointTarget = Cast<AWaypointTargetActor>(NewTarget))
	{
		ActiveArrivalRadius = WaypointTarget->ArrivalRadius;
		ActiveMarkerOffset = FVector::ZeroVector; // handled by GetMarkerWorldLocation
	}
	else
	{
		ActiveArrivalRadius = DefaultArrivalRadius;
		ActiveMarkerOffset = DefaultMarkerOffset;
	}

	ResetTrackingState();
	EnsureWidget();
	UpdateTickState();

	UE_LOG(LogWaypoint, Log, TEXT("[Waypoint] Active waypoint set: %s (arrival radius %.0f cm)"), *NewTarget->GetName(), ActiveArrivalRadius);
	OnActiveWaypointChanged.Broadcast(NewTarget);
}

void UWaypointComponent::SetActiveWaypointLocation(FVector WorldLocation, float ArrivalRadius)
{
	ActiveTarget.Reset();
	StaticTargetLocation = WorldLocation;
	bUsingStaticLocation = true;
	bHasActiveWaypoint = true;
	ActiveArrivalRadius = ArrivalRadius >= 0.0f ? ArrivalRadius : DefaultArrivalRadius;
	ActiveMarkerOffset = DefaultMarkerOffset;

	ResetTrackingState();
	EnsureWidget();
	UpdateTickState();

	UE_LOG(LogWaypoint, Log, TEXT("[Waypoint] Active waypoint set to location %s (arrival radius %.0f cm)"), *WorldLocation.ToString(), ActiveArrivalRadius);
	OnActiveWaypointChanged.Broadcast(nullptr);
}

void UWaypointComponent::ClearActiveWaypoint()
{
	const bool bHadWaypoint = bHasActiveWaypoint;

	ActiveTarget.Reset();
	bHasActiveWaypoint = false;
	bUsingStaticLocation = false;
	ResetTrackingState();

	UpdateIndicatorVisibility();
	UpdateTickState();

	if (bHadWaypoint)
	{
		UE_LOG(LogWaypoint, Log, TEXT("[Waypoint] Active waypoint cleared"));
		OnActiveWaypointChanged.Broadcast(nullptr);
	}
}

AActor* UWaypointComponent::GetActiveWaypoint() const
{
	return bHasActiveWaypoint ? ActiveTarget.Get() : nullptr;
}

void UWaypointComponent::SetIndicatorVisible(bool bVisible)
{
	if (bIndicatorVisibleRequested == bVisible)
	{
		return;
	}
	bIndicatorVisibleRequested = bVisible;
	if (bVisible)
	{
		bNeedsSnap = true;
		bHasPlacement = false;
	}
	UpdateIndicatorVisibility();
	UE_LOG(LogWaypoint, Log, TEXT("[Waypoint] Indicator visibility requested: %s"), bVisible ? TEXT("visible") : TEXT("hidden"));
}

void UWaypointComponent::UpdateTickState()
{
	SetComponentTickEnabled(bHasActiveWaypoint);
}

void UWaypointComponent::UpdateIndicatorVisibility()
{
	if (!WaypointWidget)
	{
		return;
	}

	// Only show once the marker has a valid placement so it never flashes at (0,0).
	const bool bShouldShow = bHasActiveWaypoint && bIndicatorVisibleRequested && bHasPlacement;
	if (bShouldShow)
	{
		WaypointWidget->ShowIndicator();
	}
	else
	{
		WaypointWidget->HideIndicator();
	}
}

bool UWaypointComponent::GetTargetBaseLocation(FVector& OutLocation) const
{
	if (!bHasActiveWaypoint)
	{
		return false;
	}
	if (bUsingStaticLocation)
	{
		OutLocation = StaticTargetLocation;
		return true;
	}
	if (const AActor* Target = ActiveTarget.Get())
	{
		OutLocation = Target->GetActorLocation();
		return true;
	}
	return false;
}

bool UWaypointComponent::GetTargetMarkerLocation(FVector& OutLocation) const
{
	if (!bUsingStaticLocation)
	{
		if (const AWaypointTargetActor* WaypointTarget = Cast<AWaypointTargetActor>(ActiveTarget.Get()))
		{
			OutLocation = WaypointTarget->GetMarkerWorldLocation();
			return true;
		}
	}
	if (GetTargetBaseLocation(OutLocation))
	{
		OutLocation += ActiveMarkerOffset;
		return true;
	}
	return false;
}

bool UWaypointComponent::GetDistanceOrigin(FVector& OutOrigin) const
{
	const APlayerController* PC = GetOwningPC();
	if (!PC)
	{
		return false;
	}
	if (const APawn* Pawn = PC->GetPawn())
	{
		OutOrigin = Pawn->GetActorLocation();
		return true;
	}
	if (PC->PlayerCameraManager)
	{
		OutOrigin = PC->PlayerCameraManager->GetCameraLocation();
		return true;
	}
	return false;
}

float UWaypointComponent::GetDistanceCm() const
{
	FVector TargetLocation, Origin;
	if (!GetTargetBaseLocation(TargetLocation) || !GetDistanceOrigin(Origin))
	{
		return -1.0f;
	}
	return static_cast<float>(FVector::Dist(Origin, TargetLocation));
}

float UWaypointComponent::GetDistanceToWaypoint() const
{
	const float DistanceCm = GetDistanceCm();
	return DistanceCm < 0.0f ? -1.0f : DistanceCm / 100.0f;
}

void UWaypointComponent::HandleTargetLost()
{
	UE_LOG(LogWaypoint, Warning, TEXT("[Waypoint] Active waypoint target destroyed; clearing waypoint"));
	ClearActiveWaypoint();
}

void UWaypointComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bHasActiveWaypoint)
	{
		UpdateTickState();
		return;
	}

	if (!bUsingStaticLocation && !ActiveTarget.IsValid())
	{
		HandleTargetLost();
		return;
	}

	DistanceTimer -= DeltaTime;
	if (DistanceTimer <= 0.0f)
	{
		DistanceTimer = DistanceUpdateInterval;
		if (UpdateDistanceAndArrival())
		{
			return;
		}
	}

	if (WaypointWidget && bIndicatorVisibleRequested)
	{
		UpdateScreenPosition(DeltaTime);
	}
	UpdateIndicatorVisibility();
}

bool UWaypointComponent::UpdateDistanceAndArrival()
{
	const float DistanceCm = GetDistanceCm();
	if (DistanceCm < 0.0f)
	{
		return false;
	}

	if (DistanceCm <= ActiveArrivalRadius)
	{
		AActor* ReachedTarget = ActiveTarget.Get();
		UE_LOG(LogWaypoint, Log, TEXT("[Waypoint] Waypoint reached: %s (%.0f cm <= %.0f cm)"), *GetNameSafe(ReachedTarget), DistanceCm, ActiveArrivalRadius);
		ClearActiveWaypoint();
		OnWaypointReached.Broadcast(ReachedTarget);
		return true;
	}

	const int32 Meters = FMath::RoundToInt(DistanceCm / 100.0f);
	if (Meters != LastDisplayedMeters)
	{
		LastDisplayedMeters = Meters;
		if (WaypointWidget)
		{
			WaypointWidget->SetDistanceMeters(Meters);
		}
	}
	return false;
}

bool UWaypointComponent::ComputeScreenPlacement(FVector2D& OutPosition, float& OutArrowAngle, bool& bOutOnScreen) const
{
	APlayerController* PC = GetOwningPC();
	if (!PC || !PC->PlayerCameraManager)
	{
		return false;
	}

	FVector TargetLocation;
	if (!GetTargetMarkerLocation(TargetLocation))
	{
		return false;
	}

	int32 ViewportX = 0, ViewportY = 0;
	PC->GetViewportSize(ViewportX, ViewportY);
	if (ViewportX <= 0 || ViewportY <= 0)
	{
		return false;
	}

	float ViewportScale = UWidgetLayoutLibrary::GetViewportScale(PC);
	if (ViewportScale <= KINDA_SMALL_NUMBER)
	{
		ViewportScale = 1.0f;
	}

	// Everything below is in Slate units (DPI-scaled), matching the widget's coordinate space.
	const FVector2D ViewSize = FVector2D(ViewportX, ViewportY) / ViewportScale;
	const FVector2D SafeMin(EdgePadding.Left, EdgePadding.Top);
	const FVector2D SafeMax(FMath::Max(SafeMin.X + 1.0, ViewSize.X - EdgePadding.Right), FMath::Max(SafeMin.Y + 1.0, ViewSize.Y - EdgePadding.Bottom));
	const FVector2D SafeCenter = (SafeMin + SafeMax) * 0.5;
	const FVector2D HalfSafe = (SafeMax - SafeMin) * 0.5;

	const FVector CameraLocation = PC->PlayerCameraManager->GetCameraLocation();
	const FRotator CameraRotation = PC->PlayerCameraManager->GetCameraRotation();
	// Camera space: X forward, Y right, Z up.
	const FVector Local = CameraRotation.UnrotateVector(TargetLocation - CameraLocation);
	const bool bInFront = Local.X > KINDA_SMALL_NUMBER;

	FVector2D Direction = FVector2D::ZeroVector;
	FVector2D ProjectedPixels;
	if (bInFront && PC->ProjectWorldLocationToScreen(TargetLocation, ProjectedPixels, true))
	{
		const FVector2D Projected = ProjectedPixels / ViewportScale;
		const bool bInsideSafe = Projected.X >= SafeMin.X && Projected.X <= SafeMax.X && Projected.Y >= SafeMin.Y && Projected.Y <= SafeMax.Y;
		if (bInsideSafe)
		{
			OutPosition = Projected;
			OutArrowAngle = 90.0f + ArrowAngleOffset; // arrow is hidden on-screen; point down by default
			bOutOnScreen = true;
			return true;
		}
		Direction = Projected - SafeCenter;
	}
	else
	{
		// Behind (or beside) the camera: projection would be mirrored, so use camera-space lateral
		// offset for X and force the bottom half of the screen. Directly behind => bottom-center.
		Direction = FVector2D(Local.Y, FMath::Abs(Local.X));
	}

	if (!Direction.Normalize())
	{
		Direction = FVector2D(0.0, 1.0);
	}

	const double ScaleX = FMath::Abs(Direction.X) > KINDA_SMALL_NUMBER ? HalfSafe.X / FMath::Abs(Direction.X) : TNumericLimits<double>::Max();
	const double ScaleY = FMath::Abs(Direction.Y) > KINDA_SMALL_NUMBER ? HalfSafe.Y / FMath::Abs(Direction.Y) : TNumericLimits<double>::Max();
	FVector2D Edge = SafeCenter + Direction * FMath::Min(ScaleX, ScaleY);
	Edge.X = FMath::Clamp(Edge.X, SafeMin.X, SafeMax.X);
	Edge.Y = FMath::Clamp(Edge.Y, SafeMin.Y, SafeMax.Y);

	OutPosition = Edge;
	OutArrowAngle = static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(Direction.Y, Direction.X))) + ArrowAngleOffset;
	bOutOnScreen = false;
	return true;
}

void UWaypointComponent::UpdateScreenPosition(float DeltaTime)
{
	FVector2D TargetPosition;
	float TargetAngle = 0.0f;
	bool bOnScreen = true;
	if (!ComputeScreenPlacement(TargetPosition, TargetAngle, bOnScreen))
	{
		return;
	}

	const bool bBigJump = SnapDistance > 0.0f && FVector2D::Distance(TargetPosition, SmoothedScreenPosition) > SnapDistance;
	if (bNeedsSnap || bBigJump)
	{
		SmoothedScreenPosition = TargetPosition;
		SmoothedArrowAngle = TargetAngle;
		bNeedsSnap = false;
	}
	else
	{
		SmoothedScreenPosition = FMath::Vector2DInterpTo(SmoothedScreenPosition, TargetPosition, DeltaTime, InterpSpeed);
		if (!bOnScreen)
		{
			const float AngleDelta = FMath::FindDeltaAngleDegrees(SmoothedArrowAngle, TargetAngle);
			SmoothedArrowAngle = static_cast<float>(FRotator::NormalizeAxis(static_cast<double>(SmoothedArrowAngle + AngleDelta * FMath::Clamp(DeltaTime * ArrowInterpSpeed, 0.0f, 1.0f))));
		}
	}

	if (!bHasOnScreenState || bOnScreen != bLastOnScreen)
	{
		bHasOnScreenState = true;
		bLastOnScreen = bOnScreen;
		if (!bOnScreen)
		{
			// Snap arrow to the correct direction when it first appears.
			SmoothedArrowAngle = TargetAngle;
		}
		WaypointWidget->SetOnScreenState(bOnScreen);
		UE_LOG(LogWaypoint, Log, TEXT("[Waypoint] on-screen=%s pos=(%.0f, %.0f)"), bOnScreen ? TEXT("true") : TEXT("false"), TargetPosition.X, TargetPosition.Y);
	}

	WaypointWidget->UpdateIndicator(SmoothedScreenPosition, SmoothedArrowAngle);
	bHasPlacement = true;
}
