// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/HealthDropComponent.h"

// Sets default values for this component's properties
UHealthDropComponent::UHealthDropComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UHealthDropComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UHealthDropComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UHealthDropComponent::TrySpawnHealthPickup()
{
	if (!HealthPickupClass) return;
	if (!GetOwner()) return;

	// Check drop chance
	if (FMath::FRand() > DropChance)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World) return;

	FVector StartLocation = GetOwner()->GetActorLocation();
	FVector EndLocation = StartLocation - FVector(0, 0, 1000); // Trace down

	FHitResult HitResult;
	FCollisionQueryParams TraceParams;
	TraceParams.AddIgnoredActor(GetOwner());

	bool bHit = World->LineTraceSingleByChannel(
		HitResult,
		StartLocation,
		EndLocation,
		ECC_Visibility,
		TraceParams
	);

	FVector SpawnLocation;
	if (bHit)
	{
		SpawnLocation = HitResult.ImpactPoint + FVector(0, 0, 10); // Lift it slightly off the ground
	}
	else
	{
		// Fallback to current location with slight offset
		SpawnLocation = StartLocation + FVector(0, 0, 10);
	}

	FRotator SpawnRotation = FRotator::ZeroRotator;

	World->SpawnActor<AActor>(HealthPickupClass, SpawnLocation, SpawnRotation);
}


