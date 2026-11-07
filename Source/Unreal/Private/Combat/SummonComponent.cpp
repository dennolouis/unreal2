// Fill out your copyright notice in the Description page of Project Settings.

#include "Combat/SummonComponent.h"
#include "Characters/Summons/GruxChargeSummon.h"
#include "Characters/StatsComponent.h"
#include "Combat/LockOnComponent.h"
#include "CollisionQueryParams.h"
#include "Interfaces/Enemy.h"
#include "Interfaces/Mainplayer.h"
#include "GameFramework/Character.h"
#include "Engine/OverlapResult.h"
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
	const TCHAR* TargetSelectionMode{ TEXT("ForwardFallback") };
	if (ULockOnComponent* LockOn = OwnerCharacter->FindComponentByClass<ULockOnComponent>())
	{
		AActor* Current{ LockOn->GetCurrentTargetActor() };
		if (IsValid(Current))
		{
			TargetActor = Current;
			TargetSelectionMode = TEXT("LockOn");
		}
	}

	if (!TargetActor)
	{
		TargetActor = FindNearbyGruxTarget(OwnerCharacter);
		if (TargetActor)
		{
			TargetSelectionMode = TEXT("AutoTarget");
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

	const double TargetDistance{ TargetActor ? FVector::Distance(OwnerCharacter->GetActorLocation(), TargetActor->GetActorLocation()) : 0.0 };
	UE_LOG(LogTemp, Warning, TEXT("[Summon] Summoning Grux. Mode=%s Target=%s Distance=%.1fcm SearchRadius=%.1fcm"),
		TargetSelectionMode, *GetNameSafe(TargetActor), TargetDistance, GruxAutoTargetRadius);

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

AActor* USummonComponent::FindNearbyGruxTarget(const ACharacter* OwnerCharacter) const
{
	UWorld* World{ GetWorld() };
	if (!World || !IsValid(OwnerCharacter) || GruxAutoTargetRadius <= 0.0f)
	{
		return nullptr;
	}

	const FVector Origin{ OwnerCharacter->GetActorLocation() };

	TArray<FOverlapResult> Overlaps;
	const FCollisionQueryParams QueryParams{ FName{ TEXT("GruxAutoTarget") }, false, OwnerCharacter };
	// The Boolean return reports blocking hits; touch-only overlaps are candidates too.
	World->OverlapMultiByChannel(
		Overlaps, Origin, FQuat::Identity, ECC_GameTraceChannel1,
		FCollisionShape::MakeSphere(GruxAutoTargetRadius), QueryParams);

	TSet<AActor*> SeenActors;
	AActor* BestTarget{ nullptr };
	const double RadiusSquared{ FMath::Square(static_cast<double>(GruxAutoTargetRadius)) };
	double BestDistanceSquared{ RadiusSquared };
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Candidate{ Overlap.GetActor() };
		if (!IsValid(Candidate) || Candidate == OwnerCharacter || !Candidate->Implements<UEnemy>() || SeenActors.Contains(Candidate))
		{
			continue;
		}
		SeenActors.Add(Candidate);

		const FVector ToCandidate{ Candidate->GetActorLocation() - Origin };
		const double DistanceSquared{ ToCandidate.SizeSquared() };
		// Overlapping collision bounds can extend inside the radius while the actor origin is outside it.
		if (DistanceSquared > RadiusSquared || (BestTarget && DistanceSquared >= BestDistanceSquared))
		{
			continue;
		}

		if (const UStatsComponent* Stats = Candidate->FindComponentByClass<UStatsComponent>())
		{
			const float* HealthValue{ Stats->Stats.Find(EStat::Health) };
			if (HealthValue && *HealthValue <= 0.0f)
			{
				continue;
			}
		}

		BestTarget = Candidate;
		BestDistanceSquared = DistanceSquared;
	}

	UE_LOG(LogTemp, Log, TEXT("[Summon] AutoTarget scan: overlaps=%d enemies=%d chosen=%s"),
		Overlaps.Num(), SeenActors.Num(), *GetNameSafe(BestTarget));

	return BestTarget;
}
