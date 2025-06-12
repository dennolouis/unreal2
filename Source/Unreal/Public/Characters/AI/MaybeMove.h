// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "MaybeMove.generated.h"

/**
 * 
 */
UCLASS()
class UNREAL_API UMaybeMove : public UBTTaskNode
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	TArray<UAnimMontage*> MoveAnims;

	FTimerHandle FinishTimerHandle;

public:
	virtual EBTNodeResult::Type ExecuteTask(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory
	) override;

	UPROPERTY(EditAnywhere)
	double Threshold{ 0.3 };
	
protected:
	UFUNCTION()
	void FinishTask(UBehaviorTreeComponent* OwnerComp);
};
