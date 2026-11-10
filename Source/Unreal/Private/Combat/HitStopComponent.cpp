// Fill out your copyright notice in the Description page of Project Settings.

#include "Combat/HitStopComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "TimerManager.h"

UHitStopComponent::UHitStopComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UHitStopComponent::TriggerHitStop(AActor* HitTarget, float AttackMultiplier)
{
	if (!bEnableHitStop) { return; }

	UWorld* World{ GetWorld() };
	AActor* OwnerActor{ GetOwner() };
	if (!World || !OwnerActor) { return; }

	const float SafeMultiplier{ FMath::Max(AttackMultiplier, 0.0f) };
	const float SafeMax{ FMath::Max(MaxDuration, MinDuration) };
	const float Duration{ FMath::Clamp(BaseDuration * SafeMultiplier, MinDuration, SafeMax) };

	if (Duration <= 0.0f) { return; }

	FreezeActorAndAttachments(OwnerActor);

	if (IsValid(HitTarget))
	{
		FreezeActorAndAttachments(HitTarget);
	}

	// World timer runs on global game time, unaffected by per-actor CustomTimeDilation
	FTimerManager& TimerManager{ World->GetTimerManager() };
	const double NewEndTime{ World->GetTimeSeconds() + Duration };

	if (!TimerManager.IsTimerActive(HitStopTimerHandle) || NewEndTime > HitStopEndTime)
	{
		HitStopEndTime = NewEndTime;
		TimerManager.SetTimer(HitStopTimerHandle, this, &UHitStopComponent::EndHitStop, Duration, false);
	}

	if (bDebugLog)
	{
		UE_LOG(LogTemp, Log, TEXT("[HitStop] Freeze %s + %s for %.3fs (mult %.2f, dilation %.3f, frozen actors %d)"),
			*GetNameSafe(OwnerActor), *GetNameSafe(HitTarget), Duration, AttackMultiplier, FreezeDilation, FrozenActors.Num());
	}
}

void UHitStopComponent::EndHitStop()
{
	if (UWorld* World{ GetWorld() })
	{
		World->GetTimerManager().ClearTimer(HitStopTimerHandle);
	}

	int32 RestoredCount{ 0 };

	for (const TPair<TWeakObjectPtr<AActor>, float>& Pair : FrozenActors)
	{
		if (AActor* Actor{ Pair.Key.Get() })
		{
			Actor->CustomTimeDilation = Pair.Value;
			++RestoredCount;
		}
	}

	FrozenActors.Empty();
	HitStopEndTime = 0.0;

	if (bDebugLog && RestoredCount > 0)
	{
		UE_LOG(LogTemp, Log, TEXT("[HitStop] Restored %d actors"), RestoredCount);
	}
}

void UHitStopComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (FrozenActors.Num() > 0)
	{
		EndHitStop();
	}

	Super::EndPlay(EndPlayReason);
}

void UHitStopComponent::FreezeActorAndAttachments(AActor* Actor)
{
	if (!IsValid(Actor)) { return; }

	FreezeActor(Actor);

	TArray<AActor*> AttachedActors;
	Actor->GetAttachedActors(AttachedActors, true, true);

	for (AActor* Attached : AttachedActors)
	{
		FreezeActor(Attached);
	}
}

void UHitStopComponent::FreezeActor(AActor* Actor)
{
	if (!IsValid(Actor)) { return; }

	const TWeakObjectPtr<AActor> Key{ Actor };

	// Only remember the pre-freeze value the first time, so refreshes don't store the frozen value
	if (!FrozenActors.Contains(Key))
	{
		FrozenActors.Add(Key, Actor->CustomTimeDilation);
	}

	Actor->CustomTimeDilation = FreezeDilation;
}
