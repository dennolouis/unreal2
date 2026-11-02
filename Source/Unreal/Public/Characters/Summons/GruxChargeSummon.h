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

	// Called by the spawner between SpawnActorDeferred and FinishSpawning
	UFUNCTION(BlueprintCallable, Category = "Summon")
	void InitializeSummon(AActor* InSummoner, AActor* InTarget, FVector InFallbackLocation);

	// Called from the anim blueprint StartCharge notify
	UFUNCTION(BlueprintCallable, Category = "Summon")
	void StartChargeMove();

	UFUNCTION(BlueprintCallable, Category = "Summon")
	void FinishCharge();

	UFUNCTION(BlueprintPure, Category = "Summon")
	AActor* GetSummoner() const { return Summoner.Get(); }

	virtual float GetDamage() override { return Damage; }

protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void HandleMoveCompleted(FAIRequestID RequestID, EPathFollowingResult::Type Result);

	UFUNCTION()
	void HandleChargeTimeout();

private:
	TWeakObjectPtr<AActor> Summoner;

	TWeakObjectPtr<AActor> Target;

	FVector FallbackLocation{ FVector::ZeroVector };

	float OriginalWalkSpeed{ 0.0f };

	bool bChargeStarted{ false };

	bool bChargeFinished{ false };

	FTimerHandle ChargeTimeoutHandle;

	class UBossAnimInstance* GetBossAnim() const;

	class AAIController* GetAIController() const;
};
