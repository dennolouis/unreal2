// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WaypointTargetActor.generated.h"

class UBillboardComponent;

/**
 * Placeable waypoint destination. Any actor can be a waypoint target; this class just adds
 * per-target settings (arrival radius, marker offset) and optional auto-activation.
 */
UCLASS(Blueprintable)
class UNREAL_API AWaypointTargetActor : public AActor
{
	GENERATED_BODY()

public:
	AWaypointTargetActor();

	// Distance (cm) from the player at which this waypoint counts as reached.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Waypoint", meta = (ClampMin = "0.0"))
	float ArrivalRadius{ 300.0f };

	// Local offset from the actor origin where the on-screen marker is anchored.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Waypoint", meta = (MakeEditWidget = true))
	FVector MarkerOffset{ 0.0, 0.0, 100.0 };

	// If true, this target makes itself the player's active waypoint when play begins.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Waypoint")
	bool bActivateOnBeginPlay{ false };

	// Makes this actor the local player's active waypoint.
	UFUNCTION(BlueprintCallable, Category = "Waypoint")
	void ActivateAsWaypoint();

	// World-space location the marker points at (actor location + rotated MarkerOffset).
	UFUNCTION(BlueprintPure, Category = "Waypoint")
	FVector GetMarkerWorldLocation() const;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Waypoint")
	TObjectPtr<USceneComponent> SceneRoot;

#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, Category = "Waypoint")
	TObjectPtr<UBillboardComponent> EditorBillboard;
#endif
};
