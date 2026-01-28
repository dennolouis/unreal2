// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interfaces/Enemy.h"
#include "Characters/EEnemyState.h"
#include "Interfaces/Fighter.h"
#include "BasicEnemy.generated.h"

UCLASS()
class UNREAL_API ABasicEnemy : public ACharacter, public IEnemy, public IFighter
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	TEnumAsByte<EEnemyState> InitialState;

	UPROPERTY(EditAnywhere)
	UAnimMontage* DeathAnim;

	UPROPERTY(EditAnywhere)
	TArray<UAnimMontage*> HurtAnimMontages;

public:
	// Sets default values for this character's properties
	ABasicEnemy();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UStatsComponent* StatsComp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UCombatComponent* CombatComp;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	class UTraceComponent* TraceComp;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TSubclassOf<class AWeapon> PrimaryWeaponClass;

	UPROPERTY(BlueprintReadWrite)
	class AWeapon* PrimaryEquippedWeapon;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TSubclassOf<class AWeapon> SecondaryWeaponClass;

	UPROPERTY(BlueprintReadWrite)
	class AWeapon* SecondaryEquippedWeapon;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FName PrimaryWeaponSocket;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FName SecondaryWeaponSocket;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effects")
	USceneComponent* GatherEffectSpawnPoint;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effects")
	class UNiagaraComponent* GatherEffectComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effects")
	class UNiagaraSystem* GatherEffect;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	class UHealthDropComponent* HealthDropComp;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	class UStateTreeComponent* StateTreeComp;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UFUNCTION(BlueprintCallable)
	void DetectPawn(APawn* DetectedPawn, APawn* PawnToDetect);

	virtual float GetDamage() override;

	virtual void Attack() override;

	virtual float GetAnimDuration() override;

	virtual float GetMeleeRange() override;

	UFUNCTION()
	void HandlePlayerDeath();

	UFUNCTION(BlueprintCallable)
	void HandleDeath();

	UFUNCTION()
	void FinishDeathAnim();

	UFUNCTION(BlueprintCallable)
	void StartSwordAttack(bool PrimeWeapon, float AttackMultiplier);

	UFUNCTION(BlueprintCallable)
	void StopSwordAttack();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Boss|Combat")
	void PerformSpecialAttack();
	virtual void PerformSpecialAttack_Implementation();

	UFUNCTION(BlueprintCallable)
	void PlayParticleGatherFX();

	UFUNCTION(BlueprintCallable)
	void StopParticleGatherFX();

	UFUNCTION(BlueprintCallable)
	void PlayHurtAnim();
};


