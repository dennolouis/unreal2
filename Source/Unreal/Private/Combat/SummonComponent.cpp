// Fill out your copyright notice in the Description page of Project Settings.

#include "Combat/SummonComponent.h"
#include "Characters/Summons/GruxChargeSummon.h"
#include "Characters/StatsComponent.h"
#include "Combat/LockOnComponent.h"
#include "Interfaces/Mainplayer.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"

USummonComponent::USummonComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool USummonComponent::TrySummonGruxCharge()
{
	ACharacter* OwnerCharacter{ Cast<ACharacter>(GetOwner()) };
	if (!OwnerCharacter)
	{
		UE_LOG(LogTemp, Error, TEXT("[Summon] SummonComponent owner is not a Character"));
		return false;
	}

	if (!GruxSummonClass)
	{
		UE_LOG(LogTemp, Error, TEXT("[Summon] GruxSummonClass is not set on %s"), *OwnerCharacter->GetName());
		return false;
	}

	IMainplayer* PlayerRef{ Cast<IMainplayer>(OwnerCharacter) };
	if (PlayerRef && !PlayerRef->HasEnoughStamina(GruxStaminaCost))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Summon] Not enough stamina to summon Grux (cost %.1f)"), GruxStaminaCost);
		return false;
	}

	// Lock-on target (if any)
	AActor* TargetActor{ nullptr };
	if (ULockOnComponent* LockOn = OwnerCharacter->FindComponentByClass<ULockOnComponent>())
	{
		AActor* Current{ LockOn->GetCurrentTargetActor() };
		if (IsValid(Current))
		{
			TargetActor = Current;
		}
	}

	FVector Forward{ OwnerCharacter->GetActorForwardVector() };
	Forward.Z = 0.0;
	Forward = Forward.GetSafeNormal();

	const FVector SpawnLocation{ OwnerCharacter->GetActorLocation() + Forward * GruxSpawnDistance };
	const FVector FallbackLocation{ SpawnLocation + Forward * GruxChargeRange };

	FRotator SpawnRotation{ Forward.Rotation() };
	if (TargetActor)
	{
		FVector ToTarget{ TargetActor->GetActorLocation() - SpawnLocation };
		ToTarget.Z = 0.0;
		if (!ToTarget.IsNearlyZero())
		{
			SpawnRotation = ToTarget.Rotation();
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[Summon] Summoning Grux. Target=%s"), TargetActor ? *TargetActor->GetName() : TEXT("None (forward charge)"));

	const FTransform SpawnTransform{ SpawnRotation, SpawnLocation };

	AGruxChargeSummon* Summon{ GetWorld()->SpawnActorDeferred<AGruxChargeSummon>(
		GruxSummonClass,
		SpawnTransform,
		OwnerCharacter,
		OwnerCharacter,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn
	) };

	if (!Summon)
	{
		UE_LOG(LogTemp, Error, TEXT("[Summon] Failed to spawn Grux summon"));
		return false;
	}

	Summon->InitializeSummon(OwnerCharacter, TargetActor, FallbackLocation);
	Summon->FinishSpawning(SpawnTransform);

	if (UStatsComponent* Stats = OwnerCharacter->FindComponentByClass<UStatsComponent>())
	{
		Stats->ReduceStamina(GruxStaminaCost);
	}

	UE_LOG(LogTemp, Warning, TEXT("[Summon] Spawned %s at %s (stamina cost %.1f)"), *Summon->GetName(), *Summon->GetActorLocation().ToString(), GruxStaminaCost);
	return true;
}
