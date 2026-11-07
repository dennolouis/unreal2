// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SummonComponent.generated.h"

class ACharacter;
class AGruxChargeSummon;

/**
 * Player summon abilities. Currently implements the Grux charge summon.
 */
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UNREAL_API USummonComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USummonComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Summon|Grux")
	TSubclassOf<AGruxChargeSummon> GruxSummonClass;

	// Distance in front of the owner the Grux spawns at
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Summon|Grux", meta = (ClampMin = "0.0"))
	float GruxSpawnDistance{ 250.0f };

	// Radius in cm to find the nearest enemy in front when there is no lock-on target.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Summon|Grux", meta = (ClampMin = "0.0"))
	float GruxAutoTargetRadius{ 750.0f };

	// How far forward the Grux charges when neither lock-on nor auto-targeting finds a target.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Summon|Grux", meta = (ClampMin = "0.0"))
	float GruxChargeRange{ 1500.0f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Summon|Grux", meta = (ClampMin = "0.0"))
	float GruxStaminaCost{ 30.0f };

	// Spawns the Grux and starts its charge. Returns false if it could not be summoned.
	UFUNCTION(BlueprintCallable, Category = "Summon|Grux")
	bool TrySummonGruxCharge();

private:
	AActor* FindNearbyGruxTarget(const ACharacter* OwnerCharacter) const;
};
