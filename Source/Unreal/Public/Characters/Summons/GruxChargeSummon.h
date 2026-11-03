// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interfaces/Fighter.h"
#include "Navigation/PathFollowingComponent.h"
#include "AITypes.h"
#include "GruxChargeSummon.generated.h"

/**
 * Player-summoned Grux that performs the boss charge attack (roar -> run -> DoublePain) and then despawns.
 * Movement is started by the anim blueprint StartCharge notify calling StartChargeMove().
 */
UCLASS()
class UNREAL_API AGruxChargeSummon : public ACharacter, public IFighter
{
	GENERATED_BODY()

public:
	AGruxChargeSummon();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Summon")
	class UTraceComponent* TraceComp;

	// Damage dealt by the charge hit (read by UTraceComponent via IFighter::GetDamage)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Summon|Charge")
	float Damage{ 25.0f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Summon|Charge")
	float ChargeWalkSpeed{ 2000.0f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Summon|Charge")
	float AcceptanceRadius{ 100.0f };

	// Seconds the summon stays alive after the charge ends (covers the DoublePain attack)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Summon|Charge")
	float DespawnDelayAfterAttack{ 1.2f };

	// Safety: if the move never completes, force the attack after this many seconds of charging
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Summon|Charge")
	float MaxChargeDuration{ 5.0f };

	// One-shot effect spawned at the summon's feet when it arrives (e.g. black lightning strike)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Summon|VFX")
	class UNiagaraSystem* SpawnVFX{ nullptr };

	// One-shot effect spawned at the summon's feet when it leaves (e.g. collapsing smoke column)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Summon|VFX")
	class UNiagaraSystem* DespawnVFX{ nullptr };

	// Seconds after spawning (SpawnVFX plays) before the body starts dissolving in
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Summon|VFX", meta = (ClampMin = "0.0"))
	float SpawnRevealDelay{ 0.15f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Summon|VFX", meta = (ClampMin = "0.0"))
	float FadeInDuration{ 0.6f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Summon|VFX", meta = (ClampMin = "0.0"))
	float FadeOutDuration{ 0.9f };

	// Material scalar driven on every mesh slot: 0 = visible, 1 = fully dissolved
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Summon|VFX")
	FName DissolveParamName{ TEXT("Dissolve") };

	// Called by the spawner between SpawnActorDeferred and FinishSpawning
	UFUNCTION(BlueprintCallable, Category = "Summon")
	void InitializeSummon(AActor* InSummoner, AActor* InTarget, FVector InFallbackLocation);

	// Called from the anim blueprint StartCharge notify
	UFUNCTION(BlueprintCallable, Category = "Summon")
	void StartChargeMove();

	UFUNCTION(BlueprintCallable, Category = "Summon")
	void FinishCharge();

	// Stops the flames, plays DespawnVFX, dissolves the body and destroys the summon
	UFUNCTION(BlueprintCallable, Category = "Summon")
	void BeginDespawn();

	UFUNCTION(BlueprintPure, Category = "Summon")
	AActor* GetSummoner() const { return Summoner.Get(); }

	virtual float GetDamage() override { return Damage; }

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void HandleMoveCompleted(FAIRequestID RequestID, EPathFollowingResult::Type Result);

	UFUNCTION()
	void HandleChargeTimeout();

	UFUNCTION()
	void StartReveal();

private:
	TWeakObjectPtr<AActor> Summoner;

	TWeakObjectPtr<AActor> Target;

	FVector FallbackLocation{ FVector::ZeroVector };

	float OriginalWalkSpeed{ 0.0f };

	bool bChargeStarted{ false };

	bool bChargeFinished{ false };

	FTimerHandle ChargeTimeoutHandle;

	FTimerHandle RevealHandle;

	FTimerHandle DespawnHandle;

	UPROPERTY(Transient)
	TArray<TObjectPtr<class UMaterialInstanceDynamic>> DissolveMIDs;

	float CurrentDissolve{ 1.0f };

	float FadeFrom{ 1.0f };

	float FadeTo{ 0.0f };

	float FadeDuration{ 0.0f };

	float FadeElapsed{ 0.0f };

	bool bFading{ false };

	bool bDespawning{ false };

	void StartFade(float From, float To, float Duration);

	void SetDissolve(float Value);

	void SetNiagaraComponentsActive(bool bActive, bool bImmediate);

	FVector GetFeetLocation() const;

	class UBossAnimInstance* GetBossAnim() const;

	class AAIController* GetAIController() const;
};
