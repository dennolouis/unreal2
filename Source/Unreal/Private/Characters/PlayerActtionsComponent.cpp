// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/PlayerActtionsComponent.h"
#include "GameFramework/Character.h"
#include "Interfaces/Mainplayer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Characters/MainCharacter.h"
#include "Combat/CombatComponent.h"
#include "Camera/CameraComponent.h"
#include <Kismet/GameplayStatics.h>


// Sets default values for this component's properties
UPlayerActtionsComponent::UPlayerActtionsComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UPlayerActtionsComponent::BeginPlay()
{
	Super::BeginPlay();

	CharacterRef = GetOwner<ACharacter>();
	MovementComp = CharacterRef->GetCharacterMovement();
	FollowCamera = CharacterRef->FindComponentByClass<UCameraComponent>();

	if (!CharacterRef->Implements<UMainplayer>()) { return; }

	IPlayerRef = Cast<IMainplayer>(CharacterRef);
	
}


// Called every frame
void UPlayerActtionsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UPlayerActtionsComponent::Sprint()
{
	const bool bCanSprint = IPlayerRef->HasEnoughStamina(SprintCost);
	const bool bIsMoving = MovementComp->Velocity.SizeSquared() > 10.f;

	// If player can't sprint or isn't moving, cancel sprint
	if (!bCanSprint || !bIsMoving)
	{
		if (bIsSprinting)
		{
			Walk(); // reset speed & FOV
		}
		return;
	}

	// If already sprinting, no need to reapply FOV every frame
	if (bIsSprinting)
	{
		MovementComp->MaxWalkSpeed = SprintSpeed;
		OnSprintDelegate.Broadcast(SprintCost);
		return;
	}

	// Just started sprinting
	bIsSprinting = true;
	MovementComp->MaxWalkSpeed = SprintSpeed;
	SetCameraFOV(SprintFOV, FOVTransitionTime);
	OnSprintDelegate.Broadcast(SprintCost);
}

void UPlayerActtionsComponent::Walk()
{
	MovementComp->MaxWalkSpeed = WalkSpeed;

	SetCameraFOV(WalkFOV, FOVTransitionTime);
	
	bIsSprinting = false;
}


void UPlayerActtionsComponent::Roll()
{
	// --- CASE 1: Already rolling — perform "chain roll" teleport ---
	if (bIsRollActive)
	{
		const float ChainRollCost = RollCost * 1.25f;    // Slightly higher cost for chaining
		const float ChainRollDistance = 300.f;            // How far the player teleports

		if (IPlayerRef->HasEnoughStamina(ChainRollCost))
		{
			// Consume stamina
			OnRollDelegate.Broadcast(ChainRollCost);

			FVector ForwardDir = CharacterRef->GetActorForwardVector().GetSafeNormal();
			FVector StartLocation = CharacterRef->GetActorLocation();
			FVector EndLocation = StartLocation + ForwardDir * ChainRollDistance;

			// --- Trace ahead to check for obstacles (ignore enemies) ---
			FCollisionQueryParams TraceParams;
			TraceParams.AddIgnoredActor(CharacterRef); // ignore self
			TraceParams.bTraceComplex = false;

			FHitResult Hit;
			bool bHit = CharacterRef->GetWorld()->LineTraceSingleByChannel(
				Hit,
				StartLocation,
				EndLocation,
				ECC_Visibility,
				TraceParams
			);

			// If we hit something that's NOT an enemy, stop before it
			if (bHit && Hit.GetActor() && !Hit.GetActor()->ActorHasTag("Enemy"))
			{
				EndLocation = Hit.Location - ForwardDir * 10.f; // stop just before obstacle
			}

			// Teleport directly — no sweep (we already handled collisions)
			CharacterRef->SetActorLocation(EndLocation, false, nullptr, ETeleportType::TeleportPhysics);

			if (ChainRollEffect)
			{
				UGameplayStatics::SpawnEmitterAtLocation(
					GetWorld(),
					ChainRollEffect,
					EndLocation,
					CharacterRef->GetActorRotation(),
					FVector(1.0f),    // Scale of the effect
					true              // AutoDestroy when finished
				);
			}

//#if WITH_EDITOR
//			// Debug line to visualize teleport path
//			DrawDebugLine(GetWorld(), StartLocation, EndLocation, FColor::Cyan, false, 1.0f, 0, 2.0f);
//#endif

			// Optional: spawn a small trail or VFX here if desired
			return;
		}
		else
		{
			return; // Not enough stamina
		}
	}

	// --- CASE 2: Start normal roll ---
	if (!IPlayerRef->HasEnoughStamina(RollCost))
	{
		return;
	}

	bool bCancelledAttack = false;
	AMainCharacter* MainCharacterRef = Cast<AMainCharacter>(CharacterRef);

	// Handle attack canceling
	if (MainCharacterRef && MainCharacterRef->CombatComp && !MainCharacterRef->CombatComp->CanInterruptAnimation())
	{
		if (!IPlayerRef->HasEnoughStamina(MainCharacterRef->AnimCancelStaminaCost))
		{
			return;
		}
		bCancelledAttack = true;
	}

	bIsRollActive = true;

	OnRollDelegate.Broadcast(bCancelledAttack ? MainCharacterRef->AnimCancelStaminaCost : RollCost);

	// Determine roll direction
	FVector Direction = CharacterRef->GetCharacterMovement()->Velocity.Length() < 1 ?
		CharacterRef->GetActorForwardVector() :
		CharacterRef->GetLastMovementInputVector();

	FRotator NewRot = UKismetMathLibrary::MakeRotFromX(Direction);
	CharacterRef->SetActorRotation(NewRot);

	// Play random roll animation
	int RandomIndex = FMath::RandRange(0, RollAnimMontages.Num() - 1);
	float Duration = CharacterRef->PlayAnimMontage(RollAnimMontages[RandomIndex]);

	// Set timer to end roll
	FTimerHandle RollTimerHandle;
	CharacterRef->GetWorldTimerManager().SetTimer(
		RollTimerHandle,
		this,
		&UPlayerActtionsComponent::FinishRollAnim,
		Duration,
		false
	);
}


void UPlayerActtionsComponent::FinishRollAnim()
{
	bIsRollActive = false;
}

void UPlayerActtionsComponent::SetCameraFOV(float TargetFOV, float Duration)
{
	if (!FollowCamera || !CharacterRef) { return; }

	// clear any previous lerp
	CharacterRef->GetWorldTimerManager().ClearTimer(FOVHandle);

	float StartFOV = FollowCamera->FieldOfView;
	float TimeStep = 0.02f;
	int32 TotalSteps = FMath::Max(1, FMath::RoundToInt(Duration / TimeStep));
	int32 Step = 0;

	CharacterRef->GetWorldTimerManager().SetTimer(
		FOVHandle,
		[this, StartFOV, TargetFOV, TotalSteps, Step]() mutable
		{
			Step++;
			float Alpha = static_cast<float>(Step) / TotalSteps;
			float NewFOV = FMath::Lerp(StartFOV, TargetFOV, Alpha);

			FollowCamera->SetFieldOfView(NewFOV);

			if (Alpha >= 1.f)
			{
				CharacterRef->GetWorldTimerManager().ClearTimer(FOVHandle);
			}
		},
		TimeStep,
		true
	);
}





