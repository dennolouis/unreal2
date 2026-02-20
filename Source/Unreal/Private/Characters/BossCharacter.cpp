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

	Tags.Add(FName("Enemy"));

}

// Called when the game starts or when spawned
void ABossCharacter::BeginPlay()
{
	Super::BeginPlay();

    ControllerRef = GetController<AAIController>();

    if (ControllerRef)
    {
        BlackboardComp = ControllerRef->GetBlackboardComponent();
        if (BlackboardComp)
        {
            BlackboardComp->SetValueAsEnum(TEXT("CurrentState"), InitialState);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[%s] Controller has no BlackboardComponent."), *GetName());
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[%s] No AIController found."), *GetName());
    }

    // Safely bind to the player's OnZeroHealth delegate if possible
    APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
    if (!PC)
    {
        UE_LOG(LogTemp, Warning, TEXT("[%s] No PlayerController found."), *GetName());
    }
    else
    {
        AMainCharacter* MainChar = PC->GetPawn<AMainCharacter>();
        if (!MainChar)
        {
            UE_LOG(LogTemp, Warning, TEXT("[%s] No MainCharacter found."), *GetName());
        }
        else if (MainChar->StatsComp)
        {
            MainChar->StatsComp->OnZeroHealthDelegate.AddDynamic(this, &ABossCharacter::HandlePlayerDeath);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[%s] MainCharacter has no StatsComp."), *GetName());
        }
    }

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
			PrimaryEquippedWeapon->SetOwner(this);
			PrimaryEquippedWeapon->AttachToComponent(GetMesh(),
				FAttachmentTransformRules::SnapToTargetNotIncludingScale,
				PrimaryWeaponSocket); // Change to your actual socket name

			if (PrimaryEquippedWeapon->WeaponTraceComp)
			{
				PrimaryEquippedWeapon->WeaponTraceComp->SetActorToIgnore(this);
				PrimaryEquippedWeapon->WeaponTraceComp->SetWeaponStrength(WeaponStrength);
			}
			PrimaryEquippedWeapon->SetCharacterRef(this);
		}
	}

	if (SecondaryWeaponClass)
	{
		// Spawn weapon and attach to hand socket
		SecondaryEquippedWeapon = GetWorld()->SpawnActor<AWeapon>(SecondaryWeaponClass);
		if (SecondaryEquippedWeapon)
		{
			SecondaryEquippedWeapon->SetOwner(this);
			SecondaryEquippedWeapon->AttachToComponent(GetMesh(),
				FAttachmentTransformRules::SnapToTargetNotIncludingScale,
				SecondaryWeaponSocket); // Change to your actual socket name

			if (SecondaryEquippedWeapon->WeaponTraceComp)
			{
				SecondaryEquippedWeapon->WeaponTraceComp->SetActorToIgnore(this);
				SecondaryEquippedWeapon->WeaponTraceComp->SetWeaponStrength(WeaponStrength);
			}
			SecondaryEquippedWeapon->SetCharacterRef(this);
		}
	}
}

void ABossCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Stop AI logic and unpossess controller to ensure no behavior tree callbacks run after pawn is gone
    if (ControllerRef)
    {
        if (UBrainComponent* Brain = ControllerRef->GetBrainComponent())
        {
            Brain->StopLogic(TEXT("EndPlay"));
        }

        ControllerRef->UnPossess();
    }

    // Destroy any spawned weapons so they don't persist after level streaming unload
    if (IsValid(PrimaryEquippedWeapon))
    {
        PrimaryEquippedWeapon->Destroy();
        PrimaryEquippedWeapon = nullptr;
    }

    if (IsValid(SecondaryEquippedWeapon))
    {
        SecondaryEquippedWeapon->Destroy();
        SecondaryEquippedWeapon = nullptr;
    }

    // Stop any active gather FX
    if (GatherEffectComponent && GatherEffectComponent->IsActive())
    {
        GatherEffectComponent->Deactivate();
    }

    // Remove any dynamic delegate binding to the player to avoid callbacks into this object after it's destroyed
    if (GetWorld())
    {
        APlayerController* PC = GetWorld()->GetFirstPlayerController();
        if (PC)
        {
            AMainCharacter* MainChar = PC->GetPawn<AMainCharacter>();
            if (MainChar && MainChar->StatsComp)
            {
                MainChar->StatsComp->OnZeroHealthDelegate.RemoveDynamic(this, &ABossCharacter::HandlePlayerDeath);
            }
        }
    }

    // Clear blackboard reference to be safe
    BlackboardComp = nullptr;
    ControllerRef = nullptr;

    Super::EndPlay(EndPlayReason);
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
    if (!BlackboardComp)
    {
        UE_LOG(LogTemp, Warning, TEXT("[%s] DetectPawn called but BlackboardComp is null."), *GetName());
        return;
    }

    EEnemyState CurrentState{ static_cast<EEnemyState>(BlackboardComp->GetValueAsEnum(TEXT("CurrentState"))) };

    if (DetectedPawn != PawnToDetect || CurrentState != EEnemyState::Idle) { return; }

    BlackboardComp->SetValueAsEnum(TEXT("CurrentState"), EEnemyState::Range);
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
    if (ControllerRef && ControllerRef->GetBlackboardComponent())
    {
        ControllerRef->GetBlackboardComponent()->SetValueAsEnum(TEXT("CurrentState"), EEnemyState::GameOver);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[%s] HandlePlayerDeath called but ControllerRef or Blackboard is null."), *GetName());
    }
}

void ABossCharacter::HandleDeath()
{
	float Duration{ PlayAnimMontage(DeathAnim) };
    
    if (ControllerRef && ControllerRef->GetBrainComponent())
    {
        ControllerRef->GetBrainComponent()->StopLogic("defeated");
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[%s] HandleDeath called but ControllerRef or BrainComponent is null."), *GetName());
    }

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

