// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/AI/MaybeMove.h"
#include "AIController.h"
#include "GameFramework/Character.h"
#include "Kismet/KismetMathLibrary.h"

EBTNodeResult::Type UMaybeMove::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    // Check AI owner
    AAIController* AIOwner = OwnerComp.GetAIOwner();
    if (!IsValid(AIOwner)) { return EBTNodeResult::Failed; }

    ACharacter* CharacterRef = AIOwner->GetPawn<ACharacter>();
    if (!IsValid(CharacterRef)) { return EBTNodeResult::Failed; }

    double RandomValue{ UKismetMathLibrary::RandomFloat() };

    if (RandomValue < Threshold)
    {   
        if (MoveAnims.Num() == 0) return EBTNodeResult::Succeeded;
        
        int RandomIndex{
        FMath::RandRange(0, MoveAnims.Num() - 1)
        };

        UAnimMontage* SelectedMontage = MoveAnims[RandomIndex];
        if (!SelectedMontage) return EBTNodeResult::Succeeded;


        float AnimDuration = CharacterRef->PlayAnimMontage(SelectedMontage);
        if (AnimDuration <= 0.f)
        {
            return EBTNodeResult::Succeeded;
        }

        // Cache the timer to finish task later
        UWorld* World = CharacterRef->GetWorld();
        if (!IsValid(World))
        {
            return EBTNodeResult::Failed;
        }

        FTimerDelegate TimerCallback;
        // OwnerComp is a reference; take its pointer to pass to the delegate
        UBehaviorTreeComponent* OwnerCompPtr = &OwnerComp;
        TimerCallback.BindUFunction(this, FName("FinishTask"), OwnerCompPtr);

        World->GetTimerManager().SetTimer(
            FinishTimerHandle,
            TimerCallback,
            AnimDuration,
            false
        );

        return EBTNodeResult::InProgress;
    }
    else
    {
        return EBTNodeResult::Succeeded;
    }

	return EBTNodeResult::InProgress;
}

void UMaybeMove::FinishTask(UBehaviorTreeComponent* OwnerComp)
{
    FinishLatentTask(*OwnerComp, EBTNodeResult::Succeeded);
}
