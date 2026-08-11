// Fill out your copyright notice in the Description page of Project Settings.


#include "Combat/CombatComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/KismetMathLibrary.h"
#include "Interfaces/Mainplayer.h"
#include <Combat/LockOnComponent.h>
#include "TimerManager.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Characters/PlayerActtionsComponent.h"

// Sets default values for this component's properties
UCombatComponent::UCombatComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	CharacterRef = GetOwner<ACharacter>();

	// Cache the LockOnComponent reference to avoid expensive FindComponentByClass every frame
	if (CharacterRef)
	{
		LockOnComponentRef = CharacterRef->FindComponentByClass<ULockOnComponent>();
	}
}

void UCombatComponent::SetCameraRelativeInputDirection(FVector InputDirection)
{
	if (!CharacterRef) { return; }

	// InputDirection should be a 2D vector (X, Y) representing movement input
	// We need to convert it from camera-relative space to world space

	if (InputDirection.IsNearlyZero())
	{
		LastInputDirection = FVector::ZeroVector;
		return;
	}

	// Get the player controller and camera
	APlayerController* PlayerController = Cast<APlayerController>(CharacterRef->GetController());
	if (!PlayerController) { return; }

	// Get camera forward and right vectors
	FVector CameraForward = PlayerController->GetControlRotation().Vector();
	FVector CameraRight = FRotator(0.0f, PlayerController->GetControlRotation().Yaw, 0.0f).RotateVector(FVector::RightVector);

	// Zero out Z component for horizontal-only calculation
	CameraForward.Z = 0.0f;
	CameraRight.Z = 0.0f;
	CameraForward.Normalize();
	CameraRight.Normalize();

	// Convert input to world space based on camera orientation
	// InputDirection.X = forward/back, InputDirection.Y = right/left
	FVector WorldInputDirection = (CameraForward * InputDirection.X) + (CameraRight * InputDirection.Y);
	WorldInputDirection.Z = 0.0f;
	WorldInputDirection.Normalize();

	LastInputDirection = WorldInputDirection;
}

void UCombatComponent::ApplySmoothRotationTowardsInput(float DeltaTime)
{
	if (!CharacterRef || LastInputDirection.IsNearlyZero())
	{
		return;
	}

	// Don't apply smooth rotation if locked on
	if (IsLockedOn())
	{
		return;
	}

	// Don't apply smooth rotation if this is the finisher attack
	if (IsFinisherAttack())
	{
		return;
	}

	// Get target rotation from input direction
	FRotator TargetRotation = UKismetMathLibrary::MakeRotFromX(LastInputDirection);
	FRotator TargetYaw = FRotator(0.0f, TargetRotation.Yaw, 0.0f);

	// Get current rotation
	FRotator CurrentRotation = CharacterRef->GetActorRotation();

	if (bUseSmoothedRotation)
	{
		// Smoothly interpolate towards target rotation
		// Using normalized Yaw difference to handle angle wrapping (0-360 degrees)
		FRotator InterpolatedRotation = FMath::RInterpTo(
			CurrentRotation,
			TargetYaw,
			DeltaTime,
			RotationInterpSpeed
		);
		CharacterRef->SetActorRotation(InterpolatedRotation);
	}
	else
	{
		// Instant rotation (original behavior)
		CharacterRef->SetActorRotation(TargetYaw);
	}
}

// Called every frame
void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Apply smooth rotation during attacks
	if (IsAttacking())
	{
		ApplySmoothRotationTowardsInput(DeltaTime);
	}
}

void UCombatComponent::ComboAttack()
{
	if (CharacterRef->Implements<UMainplayer>())
	{
		IMainplayer* IPlayerRef{ Cast<IMainplayer>(CharacterRef) };

		if (IPlayerRef && !IPlayerRef->HasEnoughStamina(StaminaCost))
		{
			return;
		}
	}

	if (!bCanQueueNextAttack) { return; }

	if (LockOnComponentRef && LockOnComponentRef->GetCurrentTargetActor())
	{
		LockOnComponentRef->FaceCurrentTargetForOneFrame();
	}
	// Smooth rotation is now handled in TickComponent via ApplySmoothRotationTowardsInput

	bCanQueueNextAttack = false;
	bAttackInputBuffered = false;

	float AttackAnimDuration = CharacterRef->PlayAnimMontage(AttackAnimations[ComboCounter]);

	ComboCounter++;

	int MaxCombo{ AttackAnimations.Num() };

	if (ComboCounter >= MaxCombo)
	{
		// End of combo, wait until reset notify or timer
		ComboCounter = 0;
		bCanQueueNextAttack = false;
		bAttackInputBuffered = false;
	}

	OnAttackPerformedDelegate.Broadcast(StaminaCost);
}

void UCombatComponent::TryComboAttack()
{
	if (bCanQueueNextAttack)
	{
		ComboAttack();
	}
	else if (ComboCounter != 0)
	{
		bAttackInputBuffered = true;
	}
}

void UCombatComponent::HeavyAttack()
{
	if (CharacterRef->Implements<UMainplayer>())
	{
		IMainplayer* IPlayerRef{ Cast<IMainplayer>(CharacterRef) };

		// Check if the player has enough stamina for a heavy attack
		if (IPlayerRef && !IPlayerRef->HasEnoughStamina(HeavyStaminaCost))
		{
			return;
		}
	}

	if (!bCanQueueNextAttack) { return; }

	bCanQueueNextAttack = false;

	if (LockOnComponentRef && LockOnComponentRef->GetCurrentTargetActor())
	{
		LockOnComponentRef->FaceCurrentTargetForOneFrame();
	}
	// Smooth rotation is now handled in TickComponent via ApplySmoothRotationTowardsInput

	// Play the appropriate animation based on movement state
	UAnimMontage* SelectedAnimation = bIsMoving ? MovingHeavyAttackAnimation : StandingHeavyAttackAnimation;
	float AttackAnimDuration = CharacterRef->PlayAnimMontage(SelectedAnimation);

	// Broadcast the attack event and deduct stamina
	OnAttackPerformedDelegate.Broadcast(HeavyStaminaCost);
}


void UCombatComponent::HandleResetAttack()
{
	bCanQueueNextAttack = true;

	if (bAttackInputBuffered) 
	{
		bAttackInputBuffered = false;
		ComboAttack();
	}
}

void UCombatComponent::RandomAttack()
{
	// 1. Ensure the array has elements to avoid division by zero or negative indexing
	if (AttackAnimations.IsEmpty() || !CharacterRef)
	{
		return;
	}

	// 2. Generate a safe random index
	int32 RandomIndex = FMath::RandRange(0, AttackAnimations.Num() - 1);

	// 3. Optional safeguard: Check if the element itself is not null
	if (AttackAnimations[RandomIndex])
	{
		AnimDuration = CharacterRef->PlayAnimMontage(AttackAnimations[RandomIndex]);
	}
}

void UCombatComponent::TryPlaySpecialAttack()
{
	if (bCanQueueNextAttack)
	{
		PlaySpecialAttack();
	}
}

void UCombatComponent::TryTeleportSpecialAttack()
{
    if (!bCanQueueNextAttack) return;

    if (CharacterRef->Implements<UMainplayer>())
    {
        IMainplayer* IPlayerRef{ Cast<IMainplayer>(CharacterRef) };

        if (IPlayerRef && !IPlayerRef->SpecialGageFull())
        {
            return;
        }
    }

    PlayTeleportSpecialAttack();
}

void UCombatComponent::PlayCombatSound()
{
    if (CombatSoundCue) {
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), CombatSoundCue, CharacterRef->GetActorLocation());
    }
}

void UCombatComponent::PlaySpecialAttack()
{
	if (CharacterRef->Implements<UMainplayer>())
	{
		IMainplayer* IPlayerRef{ Cast<IMainplayer>(CharacterRef) };          

		// Check if the player has enough stamina for a heavy attack
		if (IPlayerRef && !IPlayerRef->SpecialGageFull())
		{
			return;
		}
	}

	if (!bCanQueueNextAttack) { return; }

	bCanQueueNextAttack = false;

	if (LockOnComponentRef && LockOnComponentRef->GetCurrentTargetActor())
	{
		LockOnComponentRef->FaceCurrentTargetForOneFrame();
	}

	float AttackAnimDuration = CharacterRef->PlayAnimMontage(SpecialAttack);

	// Broadcast the attack event and deduct special gage
	OnSpecialAttackDelegate.Broadcast();

}

void UCombatComponent::PlayTeleportSpecialAttack()
{
    if (!bCanQueueNextAttack) { return; }

	AActor* Target = LockOnComponentRef ? LockOnComponentRef->GetCurrentTargetActor() : nullptr;

	if (!IsValid(Target))
	{
		// No target, just play regular special
		PlaySpecialAttack();
		return;
	}

	// Set pending state instead of scheduling a timer so an AnimNotify can trigger the teleport at the correct frame
	PendingTeleportTarget = Target;
	bTeleportPending = true;

	// Play the prep montage; the AnimNotify should call ExecuteTeleportSpecialAttack() when ready
	if (TeleportPrepMontage && CharacterRef)
	{
		// Pause lock-on camera control while the prep montage and teleport sequence are running
		if (LockOnComponentRef)
		{
			LockOnComponentRef->PauseCameraControl();
		}

		CharacterRef->PlayAnimMontage(TeleportPrepMontage);
	}
	else
	{
		// if no prep montage, just execute immediately
		ExecuteTeleportSpecialAttack();
	}
}

void UCombatComponent::ResetComboCounter()
{
	ComboCounter = 0;
	HeavyComboCounter = 0;
}

void UCombatComponent::ExecuteTeleportSpecialAttack()
{
    if (!bTeleportPending || !PendingTeleportTarget.IsValid() || !IsValid(CharacterRef))
    {
        bTeleportPending = false;
        PendingTeleportTarget = nullptr;
        return;
    }

    AActor* Target = PendingTeleportTarget.Get();
    // clear pending flag immediately to avoid re-entry
    bTeleportPending = false;
    PendingTeleportTarget = nullptr;

    // Re-run server-side checks similar to PlayTeleportSpecialAttack
    if (!IsValid(Target)) return;

    bCanQueueNextAttack = false;

    FVector TargetLoc = Target->GetActorLocation();
    FVector Above = TargetLoc + FVector(0, 0, TeleportHeight);

    FVector DirToPlayer = CharacterRef->GetActorLocation() - TargetLoc;
    DirToPlayer.Z = 0.0f;
    FVector Forward = DirToPlayer.GetSafeNormal();
    if (Forward.IsNearlyZero())
    {
        Forward = Target->GetActorForwardVector();
        Forward.Z = 0.0f;
        Forward = Forward.GetSafeNormal();
    }

    FVector TeleLoc = Above + Forward * TeleportForwardOffset;

    FHitResult HitResult;
    FCollisionQueryParams Params(TEXT("TeleportSweep"), false, CharacterRef);
    bool bBlocked = GetWorld()->SweepSingleByChannel(HitResult, TeleLoc, TeleLoc, FQuat::Identity,
        ECC_Pawn, FCollisionShape::MakeSphere(34.0f), Params);

    if (bBlocked)
    {
        TeleLoc.Z += 200.0f;
    }

    CharacterRef->TeleportTo(TeleLoc, CharacterRef->GetActorRotation());

    UPlayerActtionsComponent* ActionsComp = CharacterRef->FindComponentByClass<UPlayerActtionsComponent>();
    if (ActionsComp)
    {
        if (UParticleSystem* FX = ActionsComp->GetChainRollEffect())
        {
            UGameplayStatics::SpawnEmitterAtLocation(
                GetWorld(),
                FX,
                TeleLoc,
                CharacterRef->GetActorRotation(),
                FVector(1.0f),
                true
            );
        }

        if (USoundBase* Snd = ActionsComp->GetTeleportSound())
        {
            UGameplayStatics::PlaySoundAtLocation(GetWorld(), Snd, TeleLoc);
        }
    }

    UAnimMontage* ChosenMontage = TeleportSpecialAttack ? TeleportSpecialAttack : SpecialAttack;
    if (IsValid(CharacterRef) && ChosenMontage)
    {
        CharacterRef->PlayAnimMontage(ChosenMontage);
    }

    OnSpecialAttackDelegate.Broadcast();

    float Recovery = ChosenMontage ? ChosenMontage->GetPlayLength() : 0.5f;
    if (Recovery <= 0.0f) Recovery = 0.5f;
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().SetTimer(TeleportSpecialTimerHandle, [this]() {
            bCanQueueNextAttack = true;
            // Resume lock-on camera control when the teleport special attack completes
            if (CharacterRef)
			{
				if (LockOnComponentRef)
				{
					LockOnComponentRef->ResumeCameraControl();
				}
			}
		}, Recovery, false);
	}
}

bool UCombatComponent::IsAttacking() const
{
	return !bCanQueueNextAttack;
}

bool UCombatComponent::IsFinisherAttack() const
{
	return ComboCounter == 0;
}

bool UCombatComponent::IsLockedOn() const
{
	if (!LockOnComponentRef) { return false; }

	return IsValid(LockOnComponentRef->GetCurrentTargetActor());
}

void UCombatComponent::StopAttackAnimation()
{
	if (CharacterRef) 
	{ 
		CharacterRef->StopAnimMontage(); 
		bCanQueueNextAttack = true; 
	}
}

