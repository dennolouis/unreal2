// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/BasicEnemy.h"
#include "Characters/StatsComponent.h"
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
#include "AIController.h"
#include "Components/StateTreeComponent.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
ABasicEnemy::ABasicEnemy()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	StatsComp = CreateDefaultSubobject<UStatsComponent>(TEXT("Stats Componet"));
	CombatComp = CreateDefaultSubobject<UCombatComponent>(TEXT("Combat Componet"));
	TraceComp = CreateDefaultSubobject<UTraceComponent>(TEXT("Trace Component"));
	HealthDropComp = CreateDefaultSubobject<UHealthDropComponent>(TEXT("Health Drop Component"));

	GatherEffectSpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("Gather Effect Spawn Point"));
	GatherEffectSpawnPoint->SetupAttachment(RootComponent);

	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = AAIController::StaticClass();

	StateTreeComp = CreateDefaultSubobject<UStateTreeComponent>(TEXT("StateTreeComp"));

	Tags.Add(FName("Enemy"));

}

// Called when the game starts or when spawned
void ABasicEnemy::BeginPlay()
{
	Super::BeginPlay();

	GetWorld()->GetFirstPlayerController()
		->GetPawn<AMainCharacter>()
		->StatsComp
		->OnZeroHealthDelegate.
		AddDynamic(this, &ABasicEnemy::HandlePlayerDeath);

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
		if (SecondaryEquippedWeapon)
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
void ABasicEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void ABasicEnemy::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void ABasicEnemy::DetectPawn(APawn* DetectedPawn, APawn* PawnToDetect)
{

}

float ABasicEnemy::GetDamage()
{
	return StatsComp->Stats[EStat::Strength];
}

void ABasicEnemy::Attack()
{
	CombatComp->RandomAttack();
}

float ABasicEnemy::GetAnimDuration()
{
	return CombatComp->AnimDuration;
}

float ABasicEnemy::GetMeleeRange()
{
	return StatsComp->Stats[EStat::MeleeRange];
}

void ABasicEnemy::HandlePlayerDeath()
{

}

void ABasicEnemy::HandleDeath()
{
	float Duration{ PlayAnimMontage(DeathAnim) };


	FindComponentByClass<UCapsuleComponent>()
		->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	FTimerHandle DestroyTimerHandle;

	GetWorld()->GetTimerManager().SetTimer(
		DestroyTimerHandle,
		this,
		&ABasicEnemy::FinishDeathAnim,
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

void ABasicEnemy::FinishDeathAnim()
{
	if (PrimaryEquippedWeapon)
		PrimaryEquippedWeapon->Destroy();

	if (SecondaryEquippedWeapon)
		SecondaryEquippedWeapon->Destroy();

	Destroy();
}

void ABasicEnemy::StartSwordAttack(bool PrimeWeapon, float AttackMultipler)
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

void ABasicEnemy::StopSwordAttack()
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

void ABasicEnemy::PerformSpecialAttack_Implementation()
{
	UE_LOG(LogTemp, Warning, TEXT("[%s] Performing default special attack."), *GetName());
}

void ABasicEnemy::PlayParticleGatherFX()
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

void ABasicEnemy::StopParticleGatherFX()
{
	if (GatherEffectComponent && GatherEffectComponent->IsActive())
	{
		GatherEffectComponent->Deactivate();
	}
}

void ABasicEnemy::PlayHurtAnim()
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

