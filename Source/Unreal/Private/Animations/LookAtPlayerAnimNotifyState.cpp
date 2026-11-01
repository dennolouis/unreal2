// Fill out your copyright notice in the Description page of Project Settings.


#include "Animations/LookAtPlayerAnimNotifyState.h"
#include "Characters/LookAtPlayerComponent.h"
#include "GameFramework/Actor.h"

void ULookAtPlayerAnimNotifyState::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	AActor* OwnerRef{ IsValid(MeshComp) ? MeshComp->GetOwner() : nullptr };

	if (!IsValid(OwnerRef))
	{
		UE_LOG(LogTemp, Warning, TEXT("[LookAtPlayer] NotifyBegin ignored: mesh '%s' has no valid owner"), *GetNameSafe(MeshComp));
		return;
	}

	ULookAtPlayerComponent* RotationComp{
		OwnerRef->FindComponentByClass<ULookAtPlayerComponent>()
	};

	if (!IsValid(RotationComp))
	{
		UE_LOG(LogTemp, Warning, TEXT("[LookAtPlayer] NotifyBegin on %s: no ULookAtPlayerComponent, look-at will not run"), *GetNameSafe(OwnerRef));
		return;
	}

	RotationComp->bCanRotate = true;
	UE_LOG(LogTemp, Log, TEXT("[LookAtPlayer] NotifyBegin on %s: rotation enabled"), *GetNameSafe(OwnerRef));
}

void ULookAtPlayerAnimNotifyState::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	AActor* OwnerRef{ IsValid(MeshComp) ? MeshComp->GetOwner() : nullptr };

	if (!IsValid(OwnerRef))
	{
		UE_LOG(LogTemp, Warning, TEXT("[LookAtPlayer] NotifyEnd ignored: mesh '%s' has no valid owner"), *GetNameSafe(MeshComp));
		return;
	}

	ULookAtPlayerComponent* RotationComp{	
		OwnerRef->FindComponentByClass<ULookAtPlayerComponent>()
	};

	if (!IsValid(RotationComp))
	{
		UE_LOG(LogTemp, Warning, TEXT("[LookAtPlayer] NotifyEnd on %s: no ULookAtPlayerComponent, look-at never ran"), *GetNameSafe(OwnerRef));
		return;
	}

	RotationComp->bCanRotate = false;
	UE_LOG(LogTemp, Log, TEXT("[LookAtPlayer] NotifyEnd on %s: rotation disabled"), *GetNameSafe(OwnerRef));
}
