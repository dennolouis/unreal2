// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/AI/BTS_PlayerDistance.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"

void UBTS_PlayerDistance::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	const AAIController* AIOwner{ OwnerComp.GetAIOwner() };
	const APawn* OwnedPawn{ AIOwner ? AIOwner->GetPawn() : nullptr };
	UBlackboardComponent* Blackboard{ OwnerComp.GetBlackboardComponent() };

	const UWorld* World{ GetWorld() };
	const APlayerController* PlayerController{ World ? World->GetFirstPlayerController() : nullptr };
	const APawn* PlayerPawn{ PlayerController ? PlayerController->GetPawn() : nullptr };

	// The player pawn is null while the player is dead / not yet (re)possessed.
	// Keep the last Distance value instead of dereferencing a null pawn.
	if (!OwnedPawn || !PlayerPawn || !Blackboard)
	{
		return;
	}

	const float Distance{
		static_cast<float>(FVector::Distance(
			OwnedPawn->GetActorLocation(),
			PlayerPawn->GetActorLocation()
		))
	};

	Blackboard->SetValueAsFloat(TEXT("Distance"), Distance);
}
