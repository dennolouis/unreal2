// Fill out your copyright notice in the Description page of Project Settings.

#include "Navigation/WaypointSequenceActor.h"
#include "Navigation/WaypointComponent.h"
#include "Navigation/WaypointTargetActor.h"
#include "TimerManager.h"

AWaypointSequenceActor::AWaypointSequenceActor()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AWaypointSequenceActor::BeginPlay()
{
	Super::BeginPlay();

	if (bStartOnBeginPlay)
	{
		GetWorldTimerManager().SetTimerForNextTick(this, &AWaypointSequenceActor::StartSequence);
	}
}

void AWaypointSequenceActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopSequence();
	Super::EndPlay(EndPlayReason);
}

void AWaypointSequenceActor::StartSequence()
{
	StopSequence();

	UWaypointComponent* Component = UWaypointComponent::GetWaypointComponent(this);
	if (!Component)
	{
		UE_LOG(LogWaypoint, Warning, TEXT("[Waypoint] %s could not start sequence: no WaypointComponent on player controller 0"), *GetName());
		return;
	}

	WaypointComponent = Component;
	Component->OnWaypointReached.AddUniqueDynamic(this, &AWaypointSequenceActor::HandleWaypointReached);
	bIsRunning = true;
	CurrentWaypointIndex = INDEX_NONE;
	ActivateNextWaypoint();
}

void AWaypointSequenceActor::StartSequenceFromIndex(int32 StartIndex)
{
	if (!Waypoints.IsValidIndex(StartIndex))
	{
		UE_LOG(LogWaypoint, Warning, TEXT("[Waypoint] %s could not start sequence: index %d is out of range"), *GetName(), StartIndex);
		return;
	}

	StopSequence();

	UWaypointComponent* Component = UWaypointComponent::GetWaypointComponent(this);
	if (!Component)
	{
		UE_LOG(LogWaypoint, Warning, TEXT("[Waypoint] %s could not start sequence: no WaypointComponent on player controller 0"), *GetName());
		return;
	}

	WaypointComponent = Component;
	Component->OnWaypointReached.AddUniqueDynamic(this, &AWaypointSequenceActor::HandleWaypointReached);
	bIsRunning = true;
	CurrentWaypointIndex = StartIndex - 1;
	ActivateNextWaypoint();
}

void AWaypointSequenceActor::StopSequence()
{
	if (UWaypointComponent* Component = WaypointComponent.Get())
	{
		if (bIsRunning)
		{
			AWaypointTargetActor* CurrentWaypoint = GetCurrentWaypoint();
			if (CurrentWaypoint && Component->GetActiveWaypoint() == CurrentWaypoint)
			{
				Component->ClearActiveWaypoint();
			}
		}
	}

	UnbindFromWaypointComponent();
	bIsRunning = false;
	CurrentWaypointIndex = INDEX_NONE;
}

AWaypointTargetActor* AWaypointSequenceActor::GetCurrentWaypoint() const
{
	return Waypoints.IsValidIndex(CurrentWaypointIndex) ? Waypoints[CurrentWaypointIndex].Get() : nullptr;
}

void AWaypointSequenceActor::HandleWaypointReached(AActor* ReachedTarget)
{
	if (!bIsRunning || ReachedTarget != GetCurrentWaypoint())
	{
		return;
	}

	ActivateNextWaypoint();
}

void AWaypointSequenceActor::ActivateNextWaypoint()
{
	UWaypointComponent* Component = WaypointComponent.Get();
	if (!Component)
	{
		UE_LOG(LogWaypoint, Error, TEXT("[Waypoint] %s stopped: its WaypointComponent is no longer available"), *GetName());
		StopSequence();
		return;
	}

	while (++CurrentWaypointIndex < Waypoints.Num())
	{
		AWaypointTargetActor* Target = Waypoints[CurrentWaypointIndex].Get();
		if (!IsValid(Target))
		{
			UE_LOG(LogWaypoint, Warning, TEXT("[Waypoint] %s skipped invalid waypoint at index %d"), *GetName(), CurrentWaypointIndex);
			continue;
		}

		Component->SetActiveWaypoint(Target);
		return;
	}

	FinishSequence();
}

void AWaypointSequenceActor::FinishSequence()
{
	UnbindFromWaypointComponent();
	bIsRunning = false;
	CurrentWaypointIndex = INDEX_NONE;
	OnSequenceCompleted.Broadcast();
}

void AWaypointSequenceActor::UnbindFromWaypointComponent()
{
	if (UWaypointComponent* Component = WaypointComponent.Get())
	{
		Component->OnWaypointReached.RemoveDynamic(this, &AWaypointSequenceActor::HandleWaypointReached);
	}

	WaypointComponent.Reset();
}
