// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/BossCharacter.h"
#include "Characters/StatsComponent.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Combat/CombatComponent.h"
#include "Combat/Weapon.h"
#include "Combat/TraceComponent.h"
#include "Combat/WeaponTraceComponent.h"
#include "Characters/MainCharacter.h"
#include "Characters/HealthDropComponent.h"
#include "BrainComponent.h"
#include "Components/CapsuleComponent.h"
#include "Interfaces/Mainplayer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"

// Sets default values
ABossCharacter::ABossCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	StatsComp = CreateDefaultSubobject<UStatsComponent>(TEXT("Stats Componet"));
	CombatComp = CreateDefaultSubobject<UCombatComponent>(TEXT("Combat Componet"));
	TraceComp = CreateDefaultSubobject<UTraceComponent>(TEXT("Trace Component"));
	HealthDropComp = CreateDefaultSubobject<UHealthDropComponent>(TEXT("Health Drop Component"));

	GatherEffectSpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("Gather Effect Spawn Point"));
	GatherEffectSpawnPoint->SetupAttachment(RootComponent);

}

// Called when the game starts or when spawned
void ABossCharacter::BeginPlay()
{
	Super::BeginPlay();

	ControllerRef = GetController<AAIController>();

	BlackboardComp = ControllerRef
		->GetBlackboardComponent();

	BlackboardComp->SetValueAsEnum(
		TEXT("CurrentState"),
		InitialState
	);
	
	GetWorld()->GetFirstPlayerController()
		->GetPawn<AMainCharacter>()
		->StatsComp
		->OnZeroHealthDelegate.
		AddDynamic(this, &ABossCharacter::HandlePlayerDeath);

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
void ABossCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void ABossCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void ABossCharacter::DetectPawn(APawn* DetectedPawn, APawn* PawnToDetect)
{
	EEnemyState CurrentState{
		static_cast<EEnemyState>(BlackboardComp->GetValueAsEnum(TEXT("CurrentState")))
	};

	if (DetectedPawn != PawnToDetect || CurrentState != EEnemyState::Idle) { return; }

	BlackboardComp->SetValueAsEnum(
		TEXT("CurrentState"),
		EEnemyState::Range
	);
}

float ABossCharacter::GetDamage()
{
	return StatsComp->Stats[EStat::Strength];
}

void ABossCharacter::Attack()
{
	CombatComp->RandomAttack();
}

float ABossCharacter::GetAnimDuration()
{
	return CombatComp->AnimDuration;
}

float ABossCharacter::GetMeleeRange()
{
	return StatsComp->Stats[EStat::MeleeRange];
}

void ABossCharacter::HandlePlayerDeath()
{
	ControllerRef->GetBlackboardComponent()
		->SetValueAsEnum(
			TEXT("CurrentState"), EEnemyState::GameOver
		);
}

void ABossCharacter::HandleDeath()
{
	float Duration{ PlayAnimMontage(DeathAnim) };
	
	ControllerRef->GetBrainComponent()
		->StopLogic("defeated");

	FindComponentByClass<UCapsuleComponent>()
		->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	FTimerHandle DestroyTimerHandle;

	GetWorld()->GetTimerManager().SetTimer(
		DestroyTimerHandle,
		this,
		&ABossCharacter::FinishDeathAnim,
		Duration,
		false
	);

	IMainplayer* PlayerRef{
		GetWorld()->GetFirstPlayerController()
		->GetPawn<IMainplayer>()
	};

	if (!PlayerRef) { return; }

	PlayerRef->EndLockOnWithActor(this);
}

void ABossCharacter::FinishDeathAnim()
{
	if (PrimaryEquippedWeapon)
		PrimaryEquippedWeapon->Destroy();

	if (SecondaryEquippedWeapon)
		SecondaryEquippedWeapon->Destroy();

	Destroy();
}

void ABossCharacter::StartSwordAttack(bool PrimeWeapon, float AttackMultipler)
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

void ABossCharacter::StopSwordAttack()
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

void ABossCharacter::PerformSpecialAttack_Implementation()
{
	UE_LOG(LogTemp, Warning, TEXT("[%s] Performing default special attack."), *GetName());
}

void ABossCharacter::PlayParticleGatherFX()
{
	if (!GatherEffect || !GatherEffectSpawnPoint) return;

	GatherEffectComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
		GatherEffect,                                 // Niagara System
		GatherEffectSpawnPoint,                       // Attach to this component
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

void ABossCharacter::StopParticleGatherFX()
{
	if (GatherEffectComponent && GatherEffectComponent->IsActive())
	{
		GatherEffectComponent->Deactivate();
	}
}

void ABossCharacter::PlayHurtAnim()
{
	if (StatsComp && StatsComp->Stats[EStat::Health] <= 0)
	{
		return;
	}

	int RandomIndex{
		FMath::RandRange(0, HurtAnimMontages.Num() - 1)
	};

	PlayAnimMontage(HurtAnimMontages[RandomIndex]);
}

