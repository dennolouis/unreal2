// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/LookAtPlayerComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

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

	AActor* OwnerRef{ GetOwner() };
	UWorld* World{ GetWorld() };

	APlayerController* PlayerController{
		(IsValid(OwnerRef) && World) ? World->GetFirstPlayerController() : nullptr
	};
	APawn* PlayerPawn{
		IsValid(PlayerController) ? PlayerController->GetPawn() : nullptr
	};

	// Rotate only while the notify window is open and there is a real player pawn to face.
	if (!bCanRotate || !IsValid(OwnerRef) || !IsValid(PlayerPawn) || PlayerPawn == OwnerRef)
	{
		if (bRotationActive)
		{
			bRotationActive = false;
			UE_LOG(LogTemp, Log, TEXT("[LookAtPlayer] %s stopped rotating"), *GetNameSafe(OwnerRef));
		}

		return;
	}

	if (!bRotationActive)
	{
		bRotationActive = true;
		UE_LOG(LogTemp, Log, TEXT("[LookAtPlayer] %s started rotating (Speed=%.1f)"), *GetNameSafe(OwnerRef), Speed);
	}

	FVector OwnerLocation{ OwnerRef->GetActorLocation() };
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

