// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/PlayerActtionsComponent.h"
#include "GameFramework/Character.h"
#include "Interfaces/Mainplayer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Characters/MainCharacter.h"
#include "Combat/CombatComponent.h"
#include "Camera/CameraComponent.h"


// Sets default values for this component's properties
UPlayerActtionsComponent::UPlayerActtionsComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UPlayerActtionsComponent::BeginPlay()
{
	Super::BeginPlay();

	CharacterRef = GetOwner<ACharacter>();
	MovementComp = CharacterRef->GetCharacterMovement();
	FollowCamera = CharacterRef->FindComponentByClass<UCameraComponent>();

	if (!CharacterRef->Implements<UMainplayer>()) { return; }

	IPlayerRef = Cast<IMainplayer>(CharacterRef);
	
}


// Called every frame
void UPlayerActtionsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UPlayerActtionsComponent::Sprint()
{
	if (!IPlayerRef->HasEnoughStamina(SprintCost)) {
		Walk();
		return;
	}

	if (MovementComp->Velocity.Equals(FVector::ZeroVector, 1)) { return; }

	MovementComp->MaxWalkSpeed = SprintSpeed;


	OnSprintDelegate.Broadcast(SprintCost);

	if (!bIsSprinting)
	{
		SetCameraFOV(SprintFOV, FOVTransitionTime);
		bIsSprinting = true;
	}
}

void UPlayerActtionsComponent::Walk()
{
	MovementComp->MaxWalkSpeed = WalkSpeed;

	SetCameraFOV(WalkFOV, FOVTransitionTime);
	
	bIsSprinting = false;
}


void UPlayerActtionsComponent::Roll()
{
	if (bIsRollActive || !IPlayerRef->HasEnoughStamina(RollCost))
	{
		return;
	}

	bool bCanclledAttack{ false };

	AMainCharacter* MainCharacterRef{ Cast<AMainCharacter>(CharacterRef) };

	// Dash canel takes up more stamina
	if (MainCharacterRef && MainCharacterRef->CombatComp && !MainCharacterRef->CombatComp->CanInterruptAnimation()) 
	{
		if (!IPlayerRef->HasEnoughStamina(MainCharacterRef->AnimCancelStaminaCost))
		{
			return;
		}
		bCanclledAttack = true;
	}

	bIsRollActive = true;

	OnRollDelegate.Broadcast(bCanclledAttack ? MainCharacterRef->AnimCancelStaminaCost : RollCost);

	FVector Direction{
		CharacterRef->GetCharacterMovement()->Velocity.Length() < 1 ?
		CharacterRef->GetActorForwardVector() :
		CharacterRef->GetLastMovementInputVector()
	};

	FRotator NewRot{ UKismetMathLibrary::MakeRotFromX(Direction) };

	CharacterRef->SetActorRotation(NewRot);

	float Duration{ CharacterRef->PlayAnimMontage(RollAnimMontage) };
	FTimerHandle RollTimerHandle;

	CharacterRef->GetWorldTimerManager().SetTimer(
		RollTimerHandle,
		this,
		&UPlayerActtionsComponent::FinishRollAnim,
		Duration,
		false
	);
}

void UPlayerActtionsComponent::FinishRollAnim()
{
	bIsRollActive = false;
}

void UPlayerActtionsComponent::SetCameraFOV(float TargetFOV, float Duration)
{
	if (!FollowCamera || !CharacterRef) { return; }

	// clear any previous lerp
	CharacterRef->GetWorldTimerManager().ClearTimer(FOVHandle);

	float StartFOV = FollowCamera->FieldOfView;
	float TimeStep = 0.02f;
	int32 TotalSteps = FMath::Max(1, FMath::RoundToInt(Duration / TimeStep));
	int32 Step = 0;

	CharacterRef->GetWorldTimerManager().SetTimer(
		FOVHandle,
		[this, StartFOV, TargetFOV, TotalSteps, Step]() mutable
		{
			Step++;
			float Alpha = static_cast<float>(Step) / TotalSteps;
			float NewFOV = FMath::Lerp(StartFOV, TargetFOV, Alpha);

			FollowCamera->SetFieldOfView(NewFOV);

			if (Alpha >= 1.f)
			{
				CharacterRef->GetWorldTimerManager().ClearTimer(FOVHandle);
			}
		},
		TimeStep,
		true
	);
}





