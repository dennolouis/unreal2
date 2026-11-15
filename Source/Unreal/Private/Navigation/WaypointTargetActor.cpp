// Fill out your copyright notice in the Description page of Project Settings.

#include "Navigation/WaypointTargetActor.h"
#include "Navigation/WaypointComponent.h"
#include "Components/BillboardComponent.h"
#include "TimerManager.h"
#include "Engine/World.h"

AWaypointTargetActor::AWaypointTargetActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

#if WITH_EDITORONLY_DATA
	EditorBillboard = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("EditorBillboard"));
	if (EditorBillboard)
	{
		EditorBillboard->SetupAttachment(SceneRoot);
		EditorBillboard->bIsScreenSizeScaled = true;
	}
#endif
}

void AWaypointTargetActor::BeginPlay()
{
	Super::BeginPlay();

	if (bActivateOnBeginPlay)
	{
		// Defer one tick so the player controller (and its waypoint component/widget) has begun play.
		GetWorldTimerManager().SetTimerForNextTick(this, &AWaypointTargetActor::ActivateAsWaypoint);
	}
}

void AWaypointTargetActor::ActivateAsWaypoint()
{
	if (UWaypointComponent* WaypointComp = UWaypointComponent::GetWaypointComponent(this))
	{
		WaypointComp->SetActiveWaypoint(this);
	}
	else
	{
		UE_LOG(LogWaypoint, Warning, TEXT("[Waypoint] %s could not activate: no WaypointComponent on player controller 0"), *GetName());
	}
}

FVector AWaypointTargetActor::GetMarkerWorldLocation() const
{
	return GetActorTransform().TransformPosition(MarkerOffset);
}
