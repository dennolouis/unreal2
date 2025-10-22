// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/PlayerActtionsComponent.h"
#include "GameFramework/Character.h"
#include "Interfaces/Mainplayer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Characters/MainCharacter.h"
#include "Combat/CombatComponent.h"
#include "Camera/CameraComponent.h"


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
	// --- CASE 1: Already rolling, attempt "chain roll" ---
	if (bIsRollActive)
	{
		// Define cost and teleport distance
		const float ChainRollCost = RollCost * 1.25f; // slightly more expensive, optional
		const float ChainRollDistance = 300.f; // tweak to your liking

		if (IPlayerRef->HasEnoughStamina(ChainRollCost))
		{
			// Spend stamina and perform small forward teleport
			OnRollDelegate.Broadcast(ChainRollCost);

			FVector ForwardDir = CharacterRef->GetActorForwardVector().GetSafeNormal();
			FVector TeleportTarget = CharacterRef->GetActorLocation() + ForwardDir * ChainRollDistance;

			// Optional: ensure no collision issue
			FHitResult Hit;
			CharacterRef->SetActorLocation(TeleportTarget, true, &Hit, ETeleportType::TeleportPhysics);

			// Optional: Add camera shake or short trail VFX here

			// Optional: reset roll timer if you want to “extend” roll duration
			// CharacterRef->GetWorldTimerManager().ClearTimer(RollTimerHandle);
			// CharacterRef->GetWorldTimerManager().SetTimer(RollTimerHandle, this, &UPlayerActtionsComponent::FinishRollAnim, Duration, false);

			return;
		}
		else
		{
			// Not enough stamina, ignore input
			return;
		}
	}

	// --- CASE 2: Normal roll start ---
	if (!IPlayerRef->HasEnoughStamina(RollCost))
	{
		return;
	}

	bool bCancelledAttack = false;
	AMainCharacter* MainCharacterRef = Cast<AMainCharacter>(CharacterRef);

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

	FVector Direction = CharacterRef->GetCharacterMovement()->Velocity.Length() < 1 ?
		CharacterRef->GetActorForwardVector() :
		CharacterRef->GetLastMovementInputVector();

	FRotator NewRot = UKismetMathLibrary::MakeRotFromX(Direction);
	CharacterRef->SetActorRotation(NewRot);

	int RandomIndex = FMath::RandRange(0, RollAnimMontages.Num() - 1);
	float Duration = CharacterRef->PlayAnimMontage(RollAnimMontages[RandomIndex]);

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





