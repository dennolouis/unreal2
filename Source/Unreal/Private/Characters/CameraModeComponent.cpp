// Fill out your copyright notice in the Description page of Project Settings.

#include "Camera/CameraComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Characters/CameraModeComponent.h"

// Sets default values for this component's properties
UCameraModeComponent::UCameraModeComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UCameraModeComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!CameraBoom)
	{
		CameraBoom = GetOwner()->FindComponentByClass<USpringArmComponent>();
	}

	if (!FollowCamera)
	{
		FollowCamera = GetOwner()->FindComponentByClass<UCameraComponent>();
	}

	ApplyCameraMode(CurrentCameraMode);
	
}


void UCameraModeComponent::SwitchCameraMode()
{
	if (CurrentCameraMode == ECameraMode::ThirdPerson)
		ApplyCameraMode(ECameraMode::TopDownSide);
	else
		ApplyCameraMode(ECameraMode::ThirdPerson);
}

void UCameraModeComponent::ApplyCameraMode(ECameraMode NewMode)
{

	ACharacter* Character = Cast<ACharacter>(GetOwner());
	APlayerController* PC = Cast<APlayerController>(Character->GetController());

	if (!Character || !PC || !CameraBoom) return;

	CurrentCameraMode = NewMode;
	const FCameraModeSettings* Settings = nullptr;

	switch (NewMode)
	{
	case ECameraMode::ThirdPerson:
		Settings = &SoulsLikeCameraSettings;
		Character->bUseControllerRotationYaw = true;
		PC->SetIgnoreLookInput(false);  // Re-enable mouse camera control
		break;
	case ECameraMode::TopDownSide:
		Settings = &TopDownSideCameraSettings;
		Character->bUseControllerRotationYaw = false;
		PC->SetIgnoreLookInput(true);  // Disable mouse camera control
		break;
	}

	if (Settings)
	{
		CameraBoom->TargetArmLength = Settings->TargetArmLength;
		CameraBoom->SetRelativeLocation(Settings->RelativeLocation);
		CameraBoom->SetRelativeRotation(Settings->RelativeRotation);
		CameraBoom->bUsePawnControlRotation = Settings->bUsePawnControlRotation;
		
		if (Settings->bDetachCamera)
		{
			CameraBoom->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		}
		else
		{
			CameraBoom->AttachToComponent(GetOwner()->GetRootComponent(), FAttachmentTransformRules::SnapToTargetIncludingScale);
		}
	}
}

// Called every frame
void UCameraModeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (CurrentCameraMode == ECameraMode::TopDownSide && CameraBoom)
	{
		// Desired follow location: follow player but keep fixed X
		FVector PlayerLoc = GetOwner()->GetActorLocation();
		FVector CameraLoc = CameraBoom->GetComponentLocation();

		FVector TargetLoc(CameraLoc.X, PlayerLoc.Y, PlayerLoc.Z + TopDownSideCameraSettings.RelativeLocation.Z);

		// Smooth follow
		FVector NewLoc = FMath::VInterpTo(CameraLoc, TargetLoc, DeltaTime, 5.f);
		CameraBoom->SetWorldLocation(NewLoc);
	}
}

