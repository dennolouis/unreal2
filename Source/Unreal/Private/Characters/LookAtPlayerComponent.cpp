// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/LookAtPlayerComponent.h"
#include "Kismet/KismetMathLibrary.h"

// Sets default values for this component's properties
ULookAtPlayerComponent::ULookAtPlayerComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void ULookAtPlayerComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void ULookAtPlayerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bCanRotate) { return; }

	AActor* OwnerRef{ GetOwner() };
	FVector OwnerLocation{ OwnerRef->GetActorLocation() };

	APlayerController* PlayerController{
		GetWorld()->GetFirstPlayerController()
	};
	APawn* PlayerPawn{ PlayerController->GetPawn() };
	FVector PlayerLocation{ PlayerPawn->GetActorLocation() };

	FRotator DesiredRotation{
		UKismetMathLibrary::FindLookAtRotation(
			OwnerLocation, PlayerLocation
		)
	};

	FRotator CurrentRotation{ OwnerRef->GetActorRotation()};

	FRotator NewRotation{
		UKismetMathLibrary::RInterpTo_Constant(
			CurrentRotation,
			DesiredRotation,
			DeltaTime,
			Speed
		)
	};

	FRotator NewYawOnlyRotation{
		CurrentRotation.Pitch, NewRotation.Yaw, CurrentRotation.Roll
	};

	OwnerRef->SetActorRotation(NewYawOnlyRotation);
}

