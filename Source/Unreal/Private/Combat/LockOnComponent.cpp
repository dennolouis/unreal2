// Fill out your copyright notice in the Description page of Project Settings.


#include "Combat/LockOnComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "GameFramework/SpringArmComponent.h"
#include "Interfaces/Enemy.h"
#include "Engine/World.h"
#include "PhysicsEngine/SphereElem.h"

// Sets default values for this component's properties
ULockOnComponent::ULockOnComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void ULockOnComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerRef = GetOwner<ACharacter>();
	Controller = GetWorld()->GetFirstPlayerController(); //may need to change this when adding multi player
	MovementComp = OwnerRef->GetCharacterMovement();
	SpringArmComp = OwnerRef->FindComponentByClass<USpringArmComponent>();

	SetupInput();
}

void ULockOnComponent::PauseCameraControl()
{
    bCameraControlEnabled = false;
}

void ULockOnComponent::ResumeCameraControl()
{
    bCameraControlEnabled = true;
}

void ULockOnComponent::StartLockOn(float Radius)
{
	FHitResult OutResult;
	FVector CurrentLocation{ OwnerRef->GetActorLocation()};
	FCollisionShape Sphere{ FCollisionShape::MakeSphere(Radius) };
	FCollisionQueryParams IgnoreParams{
		FName {TEXT("Ignore Collision Params")},
		false,
		OwnerRef
	};

	bool bHasFoundtarget{ GetWorld()->SweepSingleByChannel(
		OutResult,
		CurrentLocation,
		CurrentLocation,
		FQuat::Identity,
		ECollisionChannel::ECC_GameTraceChannel1,
		Sphere,
		IgnoreParams
	) };

	if (!bHasFoundtarget) { return; }

	if (!OutResult.GetActor()->Implements<UEnemy>()) { return; }

	CurrentTargetActor = OutResult.GetActor();

	if (!bIsSideScroller)
	{
		Controller->SetIgnoreLookInput(true);
		//MovementComp->bOrientRotationToMovement = false;
		MovementComp->bUseControllerDesiredRotation = true;

		SpringArmComp->TargetOffset = FVector{ 0.0, 0.0, 100.0 };
	}

	IEnemy::Execute_OnSelect(CurrentTargetActor);

	OnUpdatedTargetDelegate.Broadcast(CurrentTargetActor);
}

void ULockOnComponent::EndLockOn()
{
	if (!IsValid(CurrentTargetActor)) { return; }

	IEnemy::Execute_OnDeselect(CurrentTargetActor);
	
	CurrentTargetActor = nullptr;

	MovementComp->bOrientRotationToMovement = true;
	MovementComp->bUseControllerDesiredRotation = false;
	SpringArmComp->TargetOffset = FVector::ZeroVector;

	Controller->ResetIgnoreLookInput();

	OnUpdatedTargetDelegate.Broadcast(CurrentTargetActor);

}

void ULockOnComponent::FaceCurrentTargetForOneFrame()
{
	if (!IsValid(CurrentTargetActor) || !IsValid(OwnerRef) || !IsValid(Controller)) return;

	FVector CurrentLocation = OwnerRef->GetActorLocation();
	FVector TargetLocation = CurrentTargetActor->GetActorLocation();

	TargetLocation.Z -= 125; // Adjust camera look direction slightly downward

	// Find rotation towards the target
	FRotator NewRotation = UKismetMathLibrary::FindLookAtRotation(CurrentLocation, TargetLocation);

	// Apply rotation only for one frame
	OwnerRef->SetActorRotation(FRotator(0.0f, NewRotation.Yaw, 0.0f)); // Keep character upright

}

void ULockOnComponent::ToggleLockOn(float Radius)
{
	if (IsValid(CurrentTargetActor)) 
	{
		EndLockOn();
	}
	else
	{
		StartLockOn(Radius);
	}
}


// Called every frame
void ULockOnComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!IsValid(CurrentTargetActor)) { return; }

	FVector CurrentLocation{ OwnerRef->GetActorLocation() };
	FVector TargetLocation{ CurrentTargetActor->GetActorLocation() };

	double TargetDistance{ 
		FVector::Distance(CurrentLocation, TargetLocation)
	};

	if (TargetDistance >= BreakDistance)
	{
		EndLockOn();
		return;
	}

	if (!bIsSideScroller && bCameraControlEnabled)
	{
		TargetLocation.Z -= 125; //adjusting so the camera looks more downward

		FRotator NewRotation{ UKismetMathLibrary::FindLookAtRotation(
			CurrentLocation, TargetLocation
		) };

		// Clamp the pitch angle to prevent camera from looking too far up or down
		NewRotation.Pitch = FMath::Clamp(NewRotation.Pitch, MinLockOnPitch, MaxLockOnPitch);

		Controller->SetControlRotation(NewRotation);
	}

	// Update nearby targets for target switching
	FindNearbyTargets();
}

void ULockOnComponent::FindNearbyTargets()
{
	if (!IsValid(OwnerRef)) { return; }

	NearbyTargets.Empty();

	FVector CurrentLocation{ OwnerRef->GetActorLocation() };
	FCollisionShape Sphere{ FCollisionShape::MakeSphere(TargetSwitchRadius) };
	FCollisionQueryParams QueryParams{
		FName{TEXT("Target Switch Query")},
		false,
		OwnerRef
	};

	TArray<FHitResult> OutHits;
	GetWorld()->SweepMultiByChannel(
		OutHits,
		CurrentLocation,
		CurrentLocation,
		FQuat::Identity,
		ECC_GameTraceChannel1,
		Sphere,
		QueryParams
	);

	for (const FHitResult& Hit : OutHits)
	{
		AActor* HitActor = Hit.GetActor();

		// Skip the current target
		if (HitActor == CurrentTargetActor) { continue; }

		// Only add valid enemies
		if (HitActor && HitActor->Implements<UEnemy>())
		{
			NearbyTargets.Add(HitActor);
		}
	}
}

AActor* ULockOnComponent::FindNextTarget(float DirectionAngle)
{
	if (NearbyTargets.IsEmpty() || !IsValid(OwnerRef) || !IsValid(CurrentTargetActor))
	{
		return nullptr;
	}

	FVector OwnerLocation{ OwnerRef->GetActorLocation() };
	FVector CurrentTargetDir{ (CurrentTargetActor->GetActorLocation() - OwnerLocation).GetSafeNormal2D() };
	
	float CurrentTargetAngle = FMath::Atan2(CurrentTargetDir.Y, CurrentTargetDir.X);
	float TargetAngle = CurrentTargetAngle + FMath::DegreesToRadians(DirectionAngle);

	AActor* BestTarget = nullptr;
	float SmallestAngleDiff = FLT_MAX;

	for (AActor* Candidate : NearbyTargets)
	{
		if (!IsValid(Candidate)) { continue; }

		FVector CandidateDir{ (Candidate->GetActorLocation() - OwnerLocation).GetSafeNormal2D() };
		float CandidateAngle = FMath::Atan2(CandidateDir.Y, CandidateDir.X);

		// Calculate the smallest angle difference
		float AngleDiff = FMath::Abs(FMath::FindDeltaAngleDegrees(
			FMath::RadiansToDegrees(CurrentTargetAngle),
			FMath::RadiansToDegrees(CandidateAngle)
		));

		// Adjust angle difference based on desired direction
		if (DirectionAngle > 0.0f) // Clockwise
		{
			// Prefer targets to the right
			float DirDiff = FMath::FindDeltaAngleDegrees(
				FMath::RadiansToDegrees(CurrentTargetAngle),
				FMath::RadiansToDegrees(CandidateAngle)
			);
			if (DirDiff < 0.0f) DirDiff += 360.0f;
			AngleDiff = DirDiff;
		}
		else // Counter-clockwise
		{
			// Prefer targets to the left
			float DirDiff = FMath::FindDeltaAngleDegrees(
				FMath::RadiansToDegrees(CurrentTargetAngle),
				FMath::RadiansToDegrees(CandidateAngle)
			);
			if (DirDiff > 0.0f) DirDiff -= 360.0f;
			AngleDiff = FMath::Abs(DirDiff);
		}

		// Only consider targets within angle threshold
		if (AngleDiff <= AngleThreshold && AngleDiff < SmallestAngleDiff)
		{
			SmallestAngleDiff = AngleDiff;
			BestTarget = Candidate;
		}
	}

	return BestTarget;
}

void ULockOnComponent::CycleNextTarget()
{

	if (!IsValid(CurrentTargetActor)) return;

	AActor* NextTarget = FindNextTarget(45.0f); // Clockwise direction
	
	if (!IsValid(NextTarget))
	{
		return;
	}

	IEnemy::Execute_OnDeselect(CurrentTargetActor);
	CurrentTargetActor = NextTarget;
	IEnemy::Execute_OnSelect(CurrentTargetActor);

	OnUpdatedTargetDelegate.Broadcast(CurrentTargetActor);
}

void ULockOnComponent::CyclePreviousTarget()
{

	if (!IsValid(CurrentTargetActor)) return;

	AActor* PreviousTarget = FindNextTarget(-45.0f); // Counter-clockwise direction

	if (!IsValid(PreviousTarget))
	{
		return;
	}

	IEnemy::Execute_OnDeselect(CurrentTargetActor);
	CurrentTargetActor = PreviousTarget;
	IEnemy::Execute_OnSelect(CurrentTargetActor);

	OnUpdatedTargetDelegate.Broadcast(CurrentTargetActor);
}

void ULockOnComponent::SetupInput()
{
	if (!IsValid(Controller)) { return; }

	// Get the player's input component
	APawn* OwnerPawn = Cast<APawn>(OwnerRef);
	if (!IsValid(OwnerPawn)) { return; }

	UInputComponent* InputComponent = OwnerPawn->InputComponent;
	if (!IsValid(InputComponent)) { return; }

	// Bind right joystick axis (gamepad)
	InputComponent->BindAxis("RightStickX", this, &ULockOnComponent::OnTargetSwitchInput);

	// Bind keyboard keys for target switching
	InputComponent->BindAction("TargetSwitchLeft", IE_Pressed, this, &ULockOnComponent::CyclePreviousTarget);
	InputComponent->BindAction("TargetSwitchRight", IE_Pressed, this, &ULockOnComponent::CycleNextTarget);
}

void ULockOnComponent::OnTargetSwitchInput(float AxisValue)
{
	// Only process input if we're locked on and camera control is enabled
	if (!IsValid(CurrentTargetActor) || !bCameraControlEnabled)
	{
		JoystickInputBuffer = 0.0f;
		return;
	}

	// Accumulate joystick input
	JoystickInputBuffer += AxisValue;

	// Trigger target switch when input exceeds threshold
	if (FMath::Abs(JoystickInputBuffer) >= JoystickSwitchThreshold)
	{
		if (JoystickInputBuffer > 0.0f)
		{
			CycleNextTarget();
		}
		else
		{
			CyclePreviousTarget();
		}

		// Reset buffer after switching
		JoystickInputBuffer = 0.0f;
	}
}
