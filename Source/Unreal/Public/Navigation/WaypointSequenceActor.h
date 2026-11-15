// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WaypointSequenceActor.generated.h"

class AWaypointTargetActor;
class UWaypointComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWaypointSequenceCompletedSignature);

/**
 * Advances an ordered list of waypoint targets using the local player's WaypointComponent.
 */
UCLASS(Blueprintable)
class UNREAL_API AWaypointSequenceActor : public AActor
{
	GENERATED_BODY()

public:
	AWaypointSequenceActor();

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Waypoint Sequence")
	TArray<TObjectPtr<AWaypointTargetActor>> Waypoints;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Waypoint Sequence")
	bool bStartOnBeginPlay{ true };

	UPROPERTY(BlueprintAssignable, Category = "Waypoint Sequence")
	FOnWaypointSequenceCompletedSignature OnSequenceCompleted;

	UFUNCTION(BlueprintCallable, Category = "Waypoint Sequence")
	void StartSequence();

	UFUNCTION(BlueprintCallable, Category = "Waypoint Sequence")
	void StopSequence();

	UFUNCTION(BlueprintPure, Category = "Waypoint Sequence")
	bool IsSequenceRunning() const { return bIsRunning; }

	UFUNCTION(BlueprintPure, Category = "Waypoint Sequence")
	AWaypointTargetActor* GetCurrentWaypoint() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	TWeakObjectPtr<UWaypointComponent> WaypointComponent;
	int32 CurrentWaypointIndex{ INDEX_NONE };
	bool bIsRunning{ false };

	UFUNCTION()
	void HandleWaypointReached(AActor* ReachedTarget);

	void ActivateNextWaypoint();
	void FinishSequence();
	void UnbindFromWaypointComponent();
};
