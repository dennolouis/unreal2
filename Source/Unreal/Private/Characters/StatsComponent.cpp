// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/StatsComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Interfaces/Fighter.h"

// Sets default values for this component's properties
UStatsComponent::UStatsComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UStatsComponent::BeginPlay()
{
	Super::BeginPlay();
	
}


// Called every frame
void UStatsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UStatsComponent::ReduceHealth(float Amount, AActor* Opponent)
{
	if (Stats[EStat::Health] <= 0) { return; }

	IFighter* FighterRef{ GetOwner<IFighter>() };

	if (!FighterRef->CanTakeDamage(Opponent)) { return; }

	Stats[EStat::Health] -= Amount;

	Stats[EStat::Health] = UKismetMathLibrary::FClamp(
		Stats[EStat::Health],
		0,
		Stats[EStat::MaxHealth]
	);
	
	OnHealthPercentUpdateDelegate.Broadcast(
		GetStatPercentage(EStat::Health, EStat::MaxHealth),
		Opponent
	);

	if (Stats[EStat::Health] <= 0) 
	{ 
		OnZeroHealthDelegate.Broadcast();
	}
}

void UStatsComponent::IncreaseHealth(float Amount)
{
	Stats[EStat::Health] += Amount;

	Stats[EStat::Health] = UKismetMathLibrary::FClamp(
		Stats[EStat::Health],
		0,
		Stats[EStat::MaxHealth]
	);

	OnHealthPercentUpdateDelegate.Broadcast(
		GetStatPercentage(EStat::Health, EStat::MaxHealth),
		GetOwner()
	);
}

void UStatsComponent::IncreaseSpecialGage(float Amount)
{
	Stats[EStat::SpecialGage] += Amount;

	Stats[EStat::SpecialGage] = UKismetMathLibrary::FClamp(
		Stats[EStat::SpecialGage],
		0,
		Stats[EStat::MaxSpecialGage]
	);

	OnSpecialGagePercentUpdateDelegate.Broadcast(
		GetStatPercentage(EStat::SpecialGage, EStat::MaxSpecialGage)
	);
}

void UStatsComponent::ReduceStamina(float Amount)
{
	Stats[EStat::Stamina] -= Amount;

	Stats[EStat::Stamina] = UKismetMathLibrary::FClamp(
		Stats[EStat::Stamina],
		0,
		Stats[EStat::MaxStamina]
	);

	bCanRegen = false;

	FLatentActionInfo FunctionInfo{
		0,
		100,
		TEXT("EnableRegen"),
		this
	};

	UKismetSystemLibrary::RetriggerableDelay(
		GetWorld(),
		StaminaDelayDuration,
		FunctionInfo
	);

	OnStaminaPercentUpdateDelegate.Broadcast(
		GetStatPercentage(EStat::Stamina, EStat::MaxStamina)
	);
}

void UStatsComponent::RegenStamina()
{
	if (!bCanRegen) { return; }

	Stats[EStat::Stamina] = UKismetMathLibrary::FInterpTo_Constant(
		Stats[EStat::Stamina],
		Stats[EStat::MaxStamina],
		GetWorld()->DeltaTimeSeconds,
		StaminaRegenRate
	);

	OnStaminaPercentUpdateDelegate.Broadcast(
		GetStatPercentage(EStat::Stamina, EStat::MaxStamina)
	);
}

void UStatsComponent::EnableRegen()
{
	bCanRegen = true;
}

float UStatsComponent::GetStatPercentage(EStat Current, EStat Max)
{
	return Stats[Current] / Stats[Max];
}

