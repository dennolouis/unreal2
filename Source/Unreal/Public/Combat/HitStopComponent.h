// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/TimerHandle.h"
#include "HitStopComponent.generated.h"

/**
 * Per-actor hit-stop: on a successful hit, briefly slows the owner (and its attached actors, e.g. weapons)
 * and the struck target (and its attached actors) to near zero via CustomTimeDilation.
 * The rest of the world keeps running at normal speed.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class UNREAL_API UHitStopComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHitStopComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitStop")
	bool bEnableHitStop{ true };

	// Freeze length for an attack with multiplier 1.0 (seconds)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitStop", meta = (ClampMin = "0.0"))
	float BaseDuration{ 0.06f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitStop", meta = (ClampMin = "0.0"))
	float MinDuration{ 0.04f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitStop", meta = (ClampMin = "0.0"))
	float MaxDuration{ 0.15f };

	// CustomTimeDilation applied to frozen actors (near zero avoids zero-delta edge cases)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitStop", meta = (ClampMin = "0.0001", ClampMax = "1.0"))
	float FreezeDilation{ 0.02f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitStop")
	bool bDebugLog{ true };

	// Freeze the owner and HitTarget for Clamp(BaseDuration * AttackMultiplier, MinDuration, MaxDuration)
	UFUNCTION(BlueprintCallable, Category = "HitStop")
	void TriggerHitStop(AActor* HitTarget, float AttackMultiplier = 1.0f);

	// Restore every frozen actor immediately
	UFUNCTION(BlueprintCallable, Category = "HitStop")
	void EndHitStop();

	UFUNCTION(BlueprintPure, Category = "HitStop")
	bool IsHitStopActive() const { return FrozenActors.Num() > 0; }

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void FreezeActorAndAttachments(AActor* Actor);
	void FreezeActor(AActor* Actor);

	// Frozen actor -> its CustomTimeDilation before the freeze
	TMap<TWeakObjectPtr<AActor>, float> FrozenActors;

	FTimerHandle HitStopTimerHandle;

	double HitStopEndTime{ 0.0 };
};
