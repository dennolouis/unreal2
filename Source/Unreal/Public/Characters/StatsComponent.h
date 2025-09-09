// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Characters/EStat.h"
#include "StatsComponent.generated.h"


DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_TwoParams(
	FOnHealthPercentUpdateSignature,
	UStatsComponent, OnHealthPercentUpdateDelegate,
	float, Percentage,
	AActor*, Opponent
);

DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_OneParam(
	FOnStaminaPercentUpdateSignature,
	UStatsComponent, OnStaminaPercentUpdateDelegate,
	float, Percentage
);

DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE(
	FOnZeroHealthSignature,
	UStatsComponent, OnZeroHealthDelegate
);

DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_OneParam(
	FOnSpecialGagePercentUpdateSignature,
	UStatsComponent, OnSpecialGagePercentUpdateDelegate,
	float, Percentage
);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UNREAL_API UStatsComponent : public UActorComponent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	double StaminaRegenRate{ 10.0 };

	UPROPERTY(VisibleAnywhere)
	bool bCanRegen{ true };

	UPROPERTY(EditAnywhere)
	float StaminaDelayDuration{ 2.0f };

public:	
	// Sets default values for this component's properties
	UStatsComponent();

	UPROPERTY(EditAnywhere)
	TMap<TEnumAsByte<EStat>, float> Stats;

	UPROPERTY(BlueprintAssignable)
	FOnHealthPercentUpdateSignature OnHealthPercentUpdateDelegate;

	UPROPERTY(BlueprintAssignable)
	FOnStaminaPercentUpdateSignature OnStaminaPercentUpdateDelegate;

	UPROPERTY(BlueprintAssignable)
	FOnSpecialGagePercentUpdateSignature OnSpecialGagePercentUpdateDelegate;

	UPROPERTY(BlueprintAssignable)
	FOnZeroHealthSignature OnZeroHealthDelegate;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable)
	void ReduceHealth(float Amount, AActor* Opponent);

	UFUNCTION(BlueprintCallable)
	void IncreaseHealth(float Amount);

	UFUNCTION(BlueprintCallable)
	void IncreaseSpecialGage(float Amount);

	UFUNCTION(BlueprintCallable)
	void ReduceStamina(float Amount);

	UFUNCTION(BlueprintCallable)
	void RegenStamina();

	UFUNCTION()
	void EnableRegen();

	UFUNCTION(BlueprintPure)
	float GetStatPercentage(EStat Current, EStat Max);
};
