// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interfaces/Mainplayer.h"
#include "Interfaces/Fighter.h"
#include "MainCharacter.generated.h"

UCLASS()
class UNREAL_API AMainCharacter : public ACharacter, public IMainplayer, public IFighter
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	UAnimMontage* DeathAnimMontage;

	UPROPERTY(EditAnywhere)
	TArray<UAnimMontage*> HurtAnimMontages;

public:
	// Sets default values for this character's properties
	AMainCharacter();

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	class UStatsComponent* StatsComp;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	class ULockOnComponent* LockOnComp;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	class UCombatComponent* CombatComp;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	class UTraceComponent* TraceComp;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	class UBlockComponent* BlockComp;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	class UPlayerActtionsComponent* PlayerActionsComp;

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


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(BlueprintReadOnly)
	class UPlayerAnimInstance* PlayerAnim;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual float GetDamage() override;

	virtual bool HasEnoughStamina(float Cost) override;

	UFUNCTION(BlueprintCallable)
	void HandleDeath();

	virtual void EndLockOnWithActor(AActor* ActorRef) override;

	virtual bool CanTakeDamage(AActor* Opponent) override;

	UFUNCTION(BlueprintCallable)
	void PlayHurtAnim(AActor* Attacker, TSubclassOf<class UCameraShakeBase> CameraShakeTemplate);

	UFUNCTION(BlueprintCallable)
	void CustomJump();

	UFUNCTION(BlueprintCallable)
	void TryToStopAnimation();

	UFUNCTION(BlueprintCallable)
	void StartSwordAttack(bool PrimeWeapon, float AttackMultiplier);

	UFUNCTION(BlueprintCallable)
	void StopSwordAttack();

	UFUNCTION(BlueprintCallable)
	bool IsPlayingHurtAnimation() const;
};
