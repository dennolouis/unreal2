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

	if (FMath::FRand() <= DropChance)
	{
		FVector SpawnLocation = GetOwner()->GetActorLocation() + FVector(0, 0, 50);
		FRotator SpawnRotation = FRotator::ZeroRotator;

		GetWorld()->SpawnActor<AActor>(HealthPickupClass, SpawnLocation, SpawnRotation);
	}
}



