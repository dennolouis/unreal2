// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/AI/BTT_ChargeAttack.h"
#include "AIController.h"
#include "GameFramework/Character.h"
#include "Animations/BossAnimInstance.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Characters/EEnemyState.h"

void UBTT_ChargeAttack::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	bool IsReadyToCharge{
		OwnerComp.GetBlackboardComponent()
		->GetValueAsBool(TEXT("IsReadyToCharge"))
	};

	if (IsReadyToCharge)
	{
		OwnerComp.GetBlackboardComponent()
			->SetValueAsBool(
				TEXT("IsReadyToCharge"), false
			);
		ChargeAtPlayer();
	}

	if (bIsFinished)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Boss] Finishing Attack..."));
	}
	if (!bIsFinished) { return; }

	
	ControllerRef->ReceiveMoveCompleted.Remove(MoveCompleteDelegated);
}

UBTT_ChargeAttack::UBTT_ChargeAttack()
{
	bNotifyTick = true;

	MoveCompleteDelegated.BindUFunction(
		this, "HandleMoveCompleted"
	);
}

EBTNodeResult::Type UBTT_ChargeAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	ControllerRef = OwnerComp.GetAIOwner();
	CharacterRef = ControllerRef->GetCharacter();
	BossAnimRef = Cast<UBossAnimInstance>( CharacterRef->GetMesh()->GetAnimInstance());

	OwnerCompRef = &OwnerComp;
	BlackboardCompRef = OwnerComp.GetBlackboardComponent();

	BossAnimRef->bIsCharging = true;

	OwnerComp.GetBlackboardComponent()
		->SetValueAsBool(
			TEXT("IsReadyToCharge"), false
		);

	bIsFinished = false;

	return EBTNodeResult::InProgress;
}

void UBTT_ChargeAttack::ChargeAtPlayer()
{

	UE_LOG(LogTemp, Warning, TEXT("[Boss Charge] OrientRotationToMovement: %d"), CharacterRef->GetCharacterMovement()->bOrientRotationToMovement);
	UE_LOG(LogTemp, Warning, TEXT("[Boss Charge] UseControllerRotationYaw: %d"), CharacterRef->bUseControllerRotationYaw);
	UE_LOG(LogTemp, Warning, TEXT("[Boss Charge] MaxWalkSpeed: %f"), CharacterRef->GetCharacterMovement()->MaxWalkSpeed);

	APawn* PlayerRef{
		GetWorld()->GetFirstPlayerController()->GetPawn()
	};

	FVector PlayerLocation{ PlayerRef->GetActorLocation() };

	FAIMoveRequest MoveRequest{ PlayerLocation };
	MoveRequest.SetUsePathfinding(true);
	MoveRequest.SetAcceptanceRadius(AcceptableRadius);

	ControllerRef->MoveTo(MoveRequest);
	ControllerRef->SetFocus(PlayerRef);

	ControllerRef->ReceiveMoveCompleted.AddUnique(MoveCompleteDelegated);

	OriginalWalkSpeed = CharacterRef->GetCharacterMovement()
		->MaxWalkSpeed;

	CharacterRef->GetCharacterMovement()
		->MaxWalkSpeed = ChargeWalkSpeed;
	
}

void UBTT_ChargeAttack::HandleMoveCompleted()
{
	if (ControllerRef)
	{
		ControllerRef->StopMovement();
		ControllerRef->ClearFocus(EAIFocusPriority::Gameplay);
	}

	BossAnimRef->bIsCharging = false;

	FTimerHandle AttackTimerHandle;

	CharacterRef->GetWorldTimerManager().SetTimer(
		AttackTimerHandle,
		this,
		&UBTT_ChargeAttack::FinishAttackTask,
		1.0f,
		false
	);

	CharacterRef->GetCharacterMovement()
		->MaxWalkSpeed = OriginalWalkSpeed;
}

void UBTT_ChargeAttack::FinishAttackTask()
{
	UE_LOG(LogTemp, Warning, TEXT("[Boss] FinishAttackTask CALLED"));

	bIsFinished = true;

	if (BlackboardCompRef)
	{
		BlackboardCompRef->SetValueAsEnum(TEXT("CurrentState"), EEnemyState::Melee);
	}

	if (OwnerCompRef)
	{
		FinishLatentTask(*OwnerCompRef, EBTNodeResult::Succeeded);
	}
}
