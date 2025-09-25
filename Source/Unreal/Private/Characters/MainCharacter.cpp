// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/MainCharacter.h"
#include "Animations/PlayerAnimInstance.h"
#include "Characters/StatsComponent.h"
#include "Characters/EStat.h"
#include "Combat/LockOnComponent.h"
#include "Combat/CombatComponent.h"
#include "Combat/TraceComponent.h"
#include "Combat/BlockComponent.h"
#include "Combat/Weapon.h"
#include "Combat/WeaponTraceComponent.h"
#include "Characters/PlayerActtionsComponent.h"
#include <Kismet/KismetMathLibrary.h>
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"


// Sets default values
AMainCharacter::AMainCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	StatsComp = CreateDefaultSubobject<UStatsComponent>(TEXT("Stats Component"));
	LockOnComp = CreateDefaultSubobject<ULockOnComponent>(TEXT("LockOn Component"));
	CombatComp = CreateDefaultSubobject<UCombatComponent>(TEXT("Combat Component"));
	TraceComp = CreateDefaultSubobject<UTraceComponent>(TEXT("Trace Component"));
	BlockComp = CreateDefaultSubobject<UBlockComponent>(TEXT("Block Component"));
	PlayerActionsComp = CreateDefaultSubobject<UPlayerActtionsComponent>(TEXT("Player Actions Component"));

	AuraEffectSpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("Gather Effect Spawn Point"));
	AuraEffectSpawnPoint->SetupAttachment(RootComponent);
}

// Called when the game starts or when spawned
void AMainCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	PlayerAnim = Cast<UPlayerAnimInstance>
		(GetMesh()->GetAnimInstance()
	);

	float WeaponStrength{ 10.0f };

	if (StatsComp && StatsComp->Stats.Contains(EStat::Strength))
	{
		WeaponStrength = StatsComp->Stats[EStat::Strength];
	}

	if (PrimaryWeaponClass)
	{
		// Spawn weapon and attach to hand socket
		PrimaryEquippedWeapon = GetWorld()->SpawnActor<AWeapon>(PrimaryWeaponClass);
		if (PrimaryEquippedWeapon)
		{
			PrimaryEquippedWeapon->AttachToComponent(GetMesh(),
				FAttachmentTransformRules::SnapToTargetNotIncludingScale,
				PrimaryWeaponSocket); // Change to your actual socket name

			PrimaryEquippedWeapon->WeaponTraceComp->SetActorToIgnore(this);
			PrimaryEquippedWeapon->WeaponTraceComp->SetWeaponStrength(WeaponStrength);
			PrimaryEquippedWeapon->SetCharacterRef(this);
		}
	}

	if (SecondaryWeaponClass)
	{
		// Spawn weapon and attach to hand socket
		SecondaryEquippedWeapon = GetWorld()->SpawnActor<AWeapon>(SecondaryWeaponClass);
		if (PrimaryEquippedWeapon)
		{
			SecondaryEquippedWeapon->AttachToComponent(GetMesh(),
				FAttachmentTransformRules::SnapToTargetNotIncludingScale,
				SecondaryWeaponSocket); // Change to your actual socket name

			SecondaryEquippedWeapon->WeaponTraceComp->SetActorToIgnore(this);
			SecondaryEquippedWeapon->WeaponTraceComp->SetWeaponStrength(WeaponStrength);
			SecondaryEquippedWeapon->SetCharacterRef(this);
		}
	}
}

// Called every frame
void AMainCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AMainCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

float AMainCharacter::GetDamage()
{
	return StatsComp->Stats[EStat::Strength];
}

bool AMainCharacter::HasEnoughStamina(float Cost)
{
	return StatsComp->Stats[EStat::Stamina] >= Cost;
}

bool AMainCharacter::SpecialGageFull()
{
	return StatsComp->Stats[EStat::SpecialGage] >= StatsComp->Stats[EStat::MaxSpecialGage];
}

void AMainCharacter::HandleDeath()
{
	PlayAnimMontage(DeathAnimMontage);
	DisableInput(GetController<APlayerController>());
}

void AMainCharacter::EndLockOnWithActor(AActor* ActorRef)
{
	if (LockOnComp->CurrentTargetActor != ActorRef) { return; }

	LockOnComp->EndLockOn();
}

bool AMainCharacter::CanTakeDamage(AActor* Opponent)
{
	if (PlayerActionsComp->bIsRollActive) { return false; }

	if (PlayerAnim && PlayerAnim->bIsBlocking)
	{
		return BlockComp->Check(Opponent);
	}

	return true;
}

void AMainCharacter::PlayHurtAnim(AActor* Attacker, TSubclassOf<class UCameraShakeBase> CameraShakeTemplate)
{
	if (Attacker)
	{
		// Get attacker and player positions
		FVector PlayerLocation = GetActorLocation();
		FVector AttackerLocation = Attacker->GetActorLocation();

		// Calculate rotation to face the attacker
		FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(PlayerLocation, AttackerLocation);

		// Apply rotation (only Yaw to prevent tilting)
		SetActorRotation(FRotator(0.0f, LookAtRotation.Yaw, 0.0f));
	}

	int RandomIndex{
		FMath::RandRange(0, HurtAnimMontages.Num() - 1)
	};

	if (CombatComp)
	{
		CombatComp->ClearBufferedInput();
	}

	PlayAnimMontage(HurtAnimMontages[RandomIndex]);

	if (CameraShakeTemplate)
	{
		GetController<APlayerController>()
			->ClientStartCameraShake(CameraShakeTemplate);
	}
}

void AMainCharacter::CustomJump()
{
	if ((CombatComp && !CombatComp->CanInterruptAnimation()) || IsPlayingHurtAnimation())
	{
		CombatComp->ClearBufferedInput();

		IMainplayer* IPlayerRef{ Cast<IMainplayer>(this) };

		if (IPlayerRef && !IPlayerRef->HasEnoughStamina(AnimCancelStaminaCost))
		{
			return;
		}

		if (StatsComp)
		{
			StatsComp->ReduceStamina(AnimCancelStaminaCost);
		}
	}

	StopAnimMontage();
	Super::Jump();
}

void AMainCharacter::TryToStopAnimation()
{
	if (!CombatComp || !PlayerActionsComp) return;

	if (CombatComp->CanInterruptAnimation() && !PlayerActionsComp->bIsRollActive && !IsPlayingHurtAnimation())
	{
		StopAnimMontage();
		CombatComp->ResetComboCounter();
	}
}

void AMainCharacter::StartSwordAttack(bool PrimeWeapon, float AttackMultipler)
{
	if (PrimeWeapon && PrimaryEquippedWeapon)
	{
		PrimaryEquippedWeapon->WeaponTraceComp->SetHitMultiplier(AttackMultipler);
		PrimaryEquippedWeapon->WeaponTraceComp->StartAttack();
	}

	else if (SecondaryEquippedWeapon)
	{
		SecondaryEquippedWeapon->WeaponTraceComp->SetHitMultiplier(AttackMultipler);
		SecondaryEquippedWeapon->WeaponTraceComp->StartAttack();
	}
}

void AMainCharacter::StopSwordAttack()
{
	if (PrimaryEquippedWeapon)
	{
		PrimaryEquippedWeapon->WeaponTraceComp->StopAttack();
	}

	if (SecondaryEquippedWeapon)
	{
		SecondaryEquippedWeapon->WeaponTraceComp->StopAttack();
	}
}

bool AMainCharacter::IsPlayingHurtAnimation() const
{
	if (!GetMesh() || !GetMesh()->GetAnimInstance()) return false;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	UAnimMontage* CurrentMontage = AnimInstance->GetCurrentActiveMontage();

	// Check if the current montage is in the HurtAnimMontages array
	return HurtAnimMontages.Contains(CurrentMontage);
}

void AMainCharacter::PlayAuraFX()
{
	if (!AuraEffect || !AuraEffectSpawnPoint) return;

	NiagaraEffectComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
		AuraEffect,                                 // Niagara System
		AuraEffectSpawnPoint,                       // Attach to this component
		NAME_None,                                    // Attach point name (socket/bone)
		FVector::ZeroVector,                          // Location offset
		FRotator::ZeroRotator,                        // Rotation offset
		EAttachLocation::KeepRelativeOffset,          // Maintain relative location
		true,                                         // Auto activate
		true,                                         // Auto destroy
		ENCPoolMethod::None,                          // Pooling method
		true                                          // PreCull check
	);
}

void AMainCharacter::StopAuraFX()
{
	if (NiagaraEffectComponent && NiagaraEffectComponent->IsActive())
	{
		NiagaraEffectComponent->Deactivate();
	}
}

void AMainCharacter::PlaySpecialAuraFX()
{
	if (StatsComp->GetStatPercentage(EStat::SpecialGage, EStat::MaxSpecialGage) < 1) return;

	if (!SpecialAuraEffect || !AuraEffectSpawnPoint) return;

	NiagaraSpecialEffectComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
		SpecialAuraEffect,                                 // Niagara System
		AuraEffectSpawnPoint,                       // Attach to this component
		NAME_None,                                    // Attach point name (socket/bone)
		FVector::ZeroVector,                          // Location offset
		FRotator::ZeroRotator,                        // Rotation offset
		EAttachLocation::KeepRelativeOffset,          // Maintain relative location
		true,                                         // Auto activate
		true,                                         // Auto destroy
		ENCPoolMethod::None,                          // Pooling method
		true                                          // PreCull check
	);
}

void AMainCharacter::StopSpecialAuraFX()
{
	if (NiagaraSpecialEffectComponent && NiagaraSpecialEffectComponent->IsActive())
	{
		NiagaraSpecialEffectComponent->Deactivate();
	}
}

