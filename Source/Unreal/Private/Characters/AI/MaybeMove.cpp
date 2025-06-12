// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/AI/MaybeMove.h"
#include "AIController.h"
#include "GameFramework/Character.h"
#include "Kismet/KismetMathLibrary.h"

EBTNodeResult::Type UMaybeMove::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    ACharacter* CharacterRef{
        OwnerComp.GetAIOwner()->GetPawn<ACharacter>()
    };

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
        FTimerDelegate TimerCallback;
        TimerCallback.BindUFunction(this, FName("FinishTask"), &OwnerComp);

        CharacterRef->GetWorldTimerManager().SetTimer(
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
