// Fill out your copyright notice in the Description page of Project Settings.


#include "Combat/CombatComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/KismetMathLibrary.h"
#include "Interfaces/Mainplayer.h"
#include <Combat/LockOnComponent.h>

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
	
}


// Called every frame
void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
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

	ULockOnComponent* LockOnComp = CharacterRef->FindComponentByClass<ULockOnComponent>();
	if (LockOnComp && LockOnComp->GetCurrentTargetActor())
	{
		LockOnComp->FaceCurrentTargetForOneFrame();
	}

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
	else 
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

	ULockOnComponent* LockOnComp = CharacterRef->FindComponentByClass<ULockOnComponent>();
	if (LockOnComp && LockOnComp->GetCurrentTargetActor())
	{
		LockOnComp->FaceCurrentTargetForOneFrame();
	}

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
	int RandomIndex{
		FMath::RandRange(0, AttackAnimations.Num() - 1)
	};

	AnimDuration = CharacterRef->PlayAnimMontage(AttackAnimations[RandomIndex]);
}

void UCombatComponent::TryPlaySpecialAttack()
{
	if (bCanQueueNextAttack)
	{
		PlaySpecialAttack();
	}
}

void UCombatComponent::PlaySpecialAttack()
{
	if (CharacterRef->Implements<UMainplayer>())
	{
		IMainplayer* IPlayerRef{ Cast<IMainplayer>(CharacterRef) };

		// Check if the player has enough stamina for a heavy attack
		if (IPlayerRef && !IPlayerRef->HasEnoughStamina(SpecialAttackStaminaCost))
		{
			return;
		}
	}

	if (!bCanQueueNextAttack) { return; }

	bCanQueueNextAttack = false;

	ULockOnComponent* LockOnComp = CharacterRef->FindComponentByClass<ULockOnComponent>();
	if (LockOnComp && LockOnComp->GetCurrentTargetActor())
	{
		LockOnComp->FaceCurrentTargetForOneFrame();
	}

	float AttackAnimDuration = CharacterRef->PlayAnimMontage(SpecialAttack);

	// Broadcast the attack event and deduct stamina
	OnAttackPerformedDelegate.Broadcast(SpecialAttackStaminaCost);
}

void UCombatComponent::ResetComboCounter()
{
	ComboCounter = 0;
	HeavyComboCounter = 0;
}

bool UCombatComponent::IsAttacking() const
{
	return !bCanQueueNextAttack;
}

void UCombatComponent::StopAttackAnimation()
{
	if (CharacterRef) 
	{ 
		CharacterRef->StopAnimMontage(); 
		bCanQueueNextAttack = true; 
	}
}

