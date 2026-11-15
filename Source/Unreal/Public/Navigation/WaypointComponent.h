// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Layout/Margin.h"
#include "WaypointComponent.generated.h"

class APlayerController;
class UWaypointWidget;

UNREAL_API DECLARE_LOG_CATEGORY_EXTERN(LogWaypoint, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActiveWaypointChangedSignature, AActor*, NewTarget);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWaypointReachedSignature, AActor*, ReachedTarget);

/**
 * Tracks a single active waypoint for the owning (local) player controller, projects it into
 * screen space, clamps it to a safe screen rect when off-screen/behind the camera, and drives a
 * UWaypointWidget. Ticks only while a waypoint is active.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class UNREAL_API UWaypointComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWaypointComponent();

	// ---- Config ----

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Waypoint|UI")
	TSubclassOf<UWaypointWidget> WaypointWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Waypoint|UI")
	int32 WidgetZOrder{ -1 };

	// Safe area inset (Slate units) the marker is kept inside.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Waypoint|UI")
	FMargin EdgePadding{ 80.0f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Waypoint|UI", meta = (ClampMin = "0.0"))
	float InterpSpeed{ 12.0f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Waypoint|UI", meta = (ClampMin = "0.0"))
	float ArrowInterpSpeed{ 14.0f };

	// Screen jumps larger than this (Slate units) snap instead of interpolating. 0 = never snap.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Waypoint|UI", meta = (ClampMin = "0.0"))
	float SnapDistance{ 1000.0f };

	// Added to the arrow angle. Arrow art pointing up needs +90.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Waypoint|UI")
	float ArrowAngleOffset{ 90.0f };

	// How often (seconds) distance and arrival are evaluated.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Waypoint", meta = (ClampMin = "0.0"))
	float DistanceUpdateInterval{ 0.2f };

	// Arrival radius (cm) for targets that are not AWaypointTargetActor.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Waypoint", meta = (ClampMin = "0.0"))
	float DefaultArrivalRadius{ 300.0f };

	// Marker offset (world space) for targets that are not AWaypointTargetActor.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Waypoint")
	FVector DefaultMarkerOffset{ 0.0, 0.0, 100.0 };

	// ---- Events ----

	UPROPERTY(BlueprintAssignable, Category = "Waypoint")
	FOnActiveWaypointChangedSignature OnActiveWaypointChanged;

	UPROPERTY(BlueprintAssignable, Category = "Waypoint")
	FOnWaypointReachedSignature OnWaypointReached;

	// ---- API ----

	UFUNCTION(BlueprintCallable, Category = "Waypoint")
	void SetActiveWaypoint(AActor* NewTarget);

	// Use a raw world location as the waypoint. ArrivalRadius < 0 uses DefaultArrivalRadius.
	UFUNCTION(BlueprintCallable, Category = "Waypoint")
	void SetActiveWaypointLocation(FVector WorldLocation, float ArrivalRadius = -1.0f);

	UFUNCTION(BlueprintCallable, Category = "Waypoint")
	void ClearActiveWaypoint();

	UFUNCTION(BlueprintPure, Category = "Waypoint")
	AActor* GetActiveWaypoint() const;

	UFUNCTION(BlueprintPure, Category = "Waypoint")
	bool HasActiveWaypoint() const { return bHasActiveWaypoint; }

	// Distance from the player to the waypoint in meters, or -1 if there is none.
	UFUNCTION(BlueprintPure, Category = "Waypoint")
	float GetDistanceToWaypoint() const;

	UFUNCTION(BlueprintCallable, Category = "Waypoint")
	void SetIndicatorVisible(bool bVisible);

	UFUNCTION(BlueprintPure, Category = "Waypoint")
	bool IsIndicatorOnScreen() const { return bLastOnScreen; }

	UFUNCTION(BlueprintPure, Category = "Waypoint")
	UWaypointWidget* GetWaypointWidget() const { return WaypointWidget; }

	// Finds the waypoint component on player controller 0.
	UFUNCTION(BlueprintPure, Category = "Waypoint", meta = (WorldContext = "WorldContextObject"))
	static UWaypointComponent* GetWaypointComponent(const UObject* WorldContextObject);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	TWeakObjectPtr<AActor> ActiveTarget;
	FVector StaticTargetLocation{ FVector::ZeroVector };
	FVector ActiveMarkerOffset{ FVector::ZeroVector };
	float ActiveArrivalRadius{ 300.0f };
	bool bHasActiveWaypoint{ false };
	bool bUsingStaticLocation{ false };
	bool bIndicatorVisibleRequested{ true };
	bool bHasPlacement{ false };

	UPROPERTY(Transient)
	TObjectPtr<UWaypointWidget> WaypointWidget;

	FVector2D SmoothedScreenPosition{ FVector2D::ZeroVector };
	float SmoothedArrowAngle{ 0.0f };
	bool bNeedsSnap{ true };
	bool bLastOnScreen{ true };
	bool bHasOnScreenState{ false };
	float DistanceTimer{ 0.0f };
	int32 LastDisplayedMeters{ -1 };

	APlayerController* GetOwningPC() const;
	bool GetTargetBaseLocation(FVector& OutLocation) const;
	bool GetTargetMarkerLocation(FVector& OutLocation) const;
	bool GetDistanceOrigin(FVector& OutOrigin) const;
	float GetDistanceCm() const;

	void EnsureWidget();
	void UpdateTickState();
	void UpdateIndicatorVisibility();
	void ResetTrackingState();
	// Returns true if the waypoint was reached (and cleared).
	bool UpdateDistanceAndArrival();
	void UpdateScreenPosition(float DeltaTime);
	bool ComputeScreenPlacement(FVector2D& OutPosition, float& OutArrowAngle, bool& bOutOnScreen) const;
	void HandleTargetLost();
};
