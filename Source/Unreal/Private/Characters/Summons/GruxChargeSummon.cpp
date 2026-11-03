// Fill out your copyright notice in the Description page of Project Settings.

#include "Characters/Summons/GruxChargeSummon.h"
#include "AIController.h"
#include "Animations/BossAnimInstance.h"
#include "Combat/TraceComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Materials/MaterialInstanceDynamic.h"

AGruxChargeSummon::AGruxChargeSummon()
{
	// Tick is only enabled while a dissolve fade is running
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	TraceComp = CreateDefaultSubobject<UTraceComponent>(TEXT("Trace Component"));

	AIControllerClass = AAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	bUseControllerRotationYaw = false;
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->bOrientRotationToMovement = true;
	}

	// Keep the summon out of the combat/lock-on trace channel so the player can't lock onto it
	// and enemy hit traces skip it.
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->InitCapsuleSize(34.0f, 88.0f);
		Capsule->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Ignore);
	}
	if (USkeletalMeshComponent* SkelMesh = GetMesh())
	{
		SkelMesh->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Ignore);
	}
}

void AGruxChargeSummon::InitializeSummon(AActor* InSummoner, AActor* InTarget, FVector InFallbackLocation)
{
	Summoner = InSummoner;
	Target = InTarget;
	FallbackLocation = InFallbackLocation;

	if (IsValid(InSummoner))
	{
		if (TraceComp)
		{
			TraceComp->AddActorToAlwaysIgnore(InSummoner);
		}
		if (UCapsuleComponent* Capsule = GetCapsuleComponent())
		{
			Capsule->IgnoreActorWhenMoving(InSummoner, true);
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[Summon] %s initialized. Summoner=%s Target=%s Fallback=%s"),
		*GetName(), *GetNameSafe(InSummoner), *GetNameSafe(InTarget), *InFallbackLocation.ToString());
}

void AGruxChargeSummon::BeginPlay()
{
	Super::BeginPlay();

	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		OriginalWalkSpeed = Move->MaxWalkSpeed;
	}

	if (!GetAIController())
	{
		SpawnDefaultController();
	}

	if (USkeletalMeshComponent* SkelMesh = GetMesh())
	{
		if (!SkelMesh->GetAnimInstance() && SkelMesh->GetAnimClass() && SkelMesh->GetSkeletalMeshAsset())
		{
			// Anim instance may not exist yet for a freshly spawned summon - force initialization
			SkelMesh->InitAnim(true);
		}

		UE_LOG(LogTemp, Warning, TEXT("[Summon] %s mesh=%s animClass=%s animInstance=%s"),
			*GetName(),
			*GetNameSafe(SkelMesh->GetSkeletalMeshAsset()),
			*GetNameSafe(SkelMesh->GetAnimClass()),
			SkelMesh->GetAnimInstance() ? *SkelMesh->GetAnimInstance()->GetClass()->GetName() : TEXT("None"));
	}

	// Shadow arrival: hide the body, play the strike VFX, then dissolve in
	if (USkeletalMeshComponent* SkelMesh = GetMesh())
	{
		DissolveMIDs.Reset();
		for (int32 Index = 0; Index < SkelMesh->GetNumMaterials(); ++Index)
		{
			if (UMaterialInstanceDynamic* MID = SkelMesh->CreateDynamicMaterialInstance(Index))
			{
				DissolveMIDs.Add(MID);
			}
		}
	}
	SetDissolve(1.0f);
	SetNiagaraComponentsActive(false, true);

	if (SpawnVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, SpawnVFX, GetFeetLocation(), GetActorRotation());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[Summon] %s has no SpawnVFX set"), *GetName());
	}

	UE_LOG(LogTemp, Warning, TEXT("[Summon] %s hidden for arrival (%d dissolve MIDs), reveal in %.2fs"), *GetName(), DissolveMIDs.Num(), SpawnRevealDelay);

	if (SpawnRevealDelay > 0.0f)
	{
		GetWorldTimerManager().SetTimer(RevealHandle, this, &AGruxChargeSummon::StartReveal, SpawnRevealDelay, false);
	}
	else
	{
		StartReveal();
	}

	if (UBossAnimInstance* Anim = GetBossAnim())
	{
		// Starts the roar -> TravelMode_Start (StartCharge notify) -> TravelMode_Fwd sequence
		Anim->bIsCharging = true;
		UE_LOG(LogTemp, Warning, TEXT("[Summon] %s spawned at %s, charge wind-up started"), *GetName(), *GetActorLocation().ToString());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[Summon] %s has no UBossAnimInstance - starting charge move immediately"), *GetName());
		StartChargeMove();
	}
}

void AGruxChargeSummon::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AAIController* AIC = GetAIController())
	{
		AIC->ReceiveMoveCompleted.RemoveDynamic(this, &AGruxChargeSummon::HandleMoveCompleted);
	}

	GetWorldTimerManager().ClearTimer(ChargeTimeoutHandle);
	GetWorldTimerManager().ClearTimer(RevealHandle);
	GetWorldTimerManager().ClearTimer(DespawnHandle);

	UE_LOG(LogTemp, Warning, TEXT("[Summon] %s despawned"), *GetName());

	Super::EndPlay(EndPlayReason);
}

void AGruxChargeSummon::StartChargeMove()
{
	if (bChargeStarted || bChargeFinished) { return; }
	bChargeStarted = true;

	AAIController* AIC = GetAIController();
	if (!AIC)
	{
		UE_LOG(LogTemp, Error, TEXT("[Summon] %s StartChargeMove: no AIController"), *GetName());
		FinishCharge();
		return;
	}

	const bool bHasTarget{ Target.IsValid() };
	const FVector Destination{ bHasTarget ? Target->GetActorLocation() : FallbackLocation };

	FAIMoveRequest MoveRequest{ Destination };
	MoveRequest.SetUsePathfinding(true);
	MoveRequest.SetAcceptanceRadius(AcceptanceRadius);
	MoveRequest.SetAllowPartialPath(true);
	MoveRequest.SetProjectGoalLocation(true);

	AIC->ReceiveMoveCompleted.AddUniqueDynamic(this, &AGruxChargeSummon::HandleMoveCompleted);

	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->MaxWalkSpeed = ChargeWalkSpeed;
	}

	if (bHasTarget)
	{
		AIC->SetFocus(Target.Get());
	}

	const FPathFollowingRequestResult Result{ AIC->MoveTo(MoveRequest) };

	UE_LOG(LogTemp, Warning, TEXT("[Summon] %s charge move started -> %s (%s). RequestResult=%d"),
		*GetName(), *Destination.ToString(), bHasTarget ? *Target->GetName() : TEXT("forward fallback"), static_cast<int32>(Result.Code));

	if (Result.Code == EPathFollowingRequestResult::Failed)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Summon] %s move request failed, attacking in place"), *GetName());
		FinishCharge();
		return;
	}

	if (Result.Code == EPathFollowingRequestResult::AlreadyAtGoal)
	{
		FinishCharge();
		return;
	}

	GetWorldTimerManager().SetTimer(ChargeTimeoutHandle, this, &AGruxChargeSummon::HandleChargeTimeout, MaxChargeDuration, false);
}

void AGruxChargeSummon::HandleMoveCompleted(FAIRequestID RequestID, EPathFollowingResult::Type Result)
{
	UE_LOG(LogTemp, Warning, TEXT("[Summon] %s move completed. Result=%d Location=%s"),
		*GetName(), static_cast<int32>(Result), *GetActorLocation().ToString());
	FinishCharge();
}

void AGruxChargeSummon::HandleChargeTimeout()
{
	UE_LOG(LogTemp, Warning, TEXT("[Summon] %s charge timed out after %.1fs"), *GetName(), MaxChargeDuration);
	FinishCharge();
}

void AGruxChargeSummon::FinishCharge()
{
	if (bChargeFinished) { return; }
	bChargeFinished = true;

	GetWorldTimerManager().ClearTimer(ChargeTimeoutHandle);

	if (AAIController* AIC = GetAIController())
	{
		AIC->ReceiveMoveCompleted.RemoveDynamic(this, &AGruxChargeSummon::HandleMoveCompleted);
		AIC->StopMovement();
		AIC->ClearFocus(EAIFocusPriority::Gameplay);
	}

	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->MaxWalkSpeed = OriginalWalkSpeed;
	}

	// Leaving the charge state plays DoublePain (the attack with trace notifies)
	if (UBossAnimInstance* Anim = GetBossAnim())
	{
		Anim->bIsCharging = false;
	}

	GetWorldTimerManager().SetTimer(DespawnHandle, this, &AGruxChargeSummon::BeginDespawn, FMath::Max(DespawnDelayAfterAttack, 0.01f), false);

	UE_LOG(LogTemp, Warning, TEXT("[Summon] %s charge finished at %s, attacking; despawn in %.2fs"),
		*GetName(), *GetActorLocation().ToString(), DespawnDelayAfterAttack);
}

void AGruxChargeSummon::StartReveal()
{
	if (bDespawning) { return; }

	UE_LOG(LogTemp, Warning, TEXT("[Summon] %s reveal started (%.2fs dissolve-in)"), *GetName(), FadeInDuration);

	SetNiagaraComponentsActive(true, false);
	StartFade(CurrentDissolve, 0.0f, FadeInDuration);
}

void AGruxChargeSummon::BeginDespawn()
{
	if (bDespawning) { return; }
	bDespawning = true;

	GetWorldTimerManager().ClearTimer(RevealHandle);
	GetWorldTimerManager().ClearTimer(DespawnHandle);
	GetWorldTimerManager().ClearTimer(ChargeTimeoutHandle);

	UE_LOG(LogTemp, Warning, TEXT("[Summon] %s despawn started at %s (%.2fs dissolve-out)"), *GetName(), *GetActorLocation().ToString(), FadeOutDuration);

	// Let the flames finish their life instead of popping off
	SetNiagaraComponentsActive(false, false);

	// Freeze in place and stop blocking anything while collapsing into smoke
	if (AAIController* AIC = GetAIController())
	{
		AIC->StopMovement();
	}
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->StopMovementImmediately();
		Move->DisableMovement();
	}
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (DespawnVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, DespawnVFX, GetFeetLocation(), GetActorRotation());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[Summon] %s has no DespawnVFX set"), *GetName());
	}

	StartFade(CurrentDissolve, 1.0f, FadeOutDuration);

	// Safety net in case the fade never completes
	SetLifeSpan(FadeOutDuration + 1.0f);
}

void AGruxChargeSummon::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bFading) { return; }

	FadeElapsed += DeltaSeconds;
	const float Alpha{ FadeDuration > KINDA_SMALL_NUMBER ? FMath::Clamp(FadeElapsed / FadeDuration, 0.0f, 1.0f) : 1.0f };
	SetDissolve(FMath::Lerp(FadeFrom, FadeTo, Alpha));

	if (Alpha >= 1.0f)
	{
		bFading = false;
		SetActorTickEnabled(false);

		if (bDespawning)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Summon] %s dissolve-out complete, destroying"), *GetName());
			Destroy();
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[Summon] %s fully revealed"), *GetName());
		}
	}
}

void AGruxChargeSummon::StartFade(float From, float To, float Duration)
{
	FadeFrom = From;
	FadeTo = To;
	FadeDuration = Duration;
	FadeElapsed = 0.0f;

	if (Duration <= KINDA_SMALL_NUMBER)
	{
		SetDissolve(To);
		bFading = false;
		if (bDespawning)
		{
			Destroy();
		}
		return;
	}

	bFading = true;
	SetActorTickEnabled(true);
}

void AGruxChargeSummon::SetDissolve(float Value)
{
	CurrentDissolve = Value;
	for (UMaterialInstanceDynamic* MID : DissolveMIDs)
	{
		if (MID)
		{
			MID->SetScalarParameterValue(DissolveParamName, Value);
		}
	}
}

void AGruxChargeSummon::SetNiagaraComponentsActive(bool bActive, bool bImmediate)
{
	TInlineComponentArray<UNiagaraComponent*> NiagaraComps(this);
	for (UNiagaraComponent* Comp : NiagaraComps)
	{
		if (!Comp) { continue; }

		if (bActive)
		{
			Comp->Activate(true);
		}
		else if (bImmediate)
		{
			Comp->DeactivateImmediate();
		}
		else
		{
			Comp->Deactivate();
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[Summon] %s set %d Niagara component(s) %s"), *GetName(), NiagaraComps.Num(), bActive ? TEXT("active") : TEXT("inactive"));
}

FVector AGruxChargeSummon::GetFeetLocation() const
{
	const float HalfHeight{ GetCapsuleComponent() ? GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 0.0f };
	return GetActorLocation() - FVector(0.0, 0.0, HalfHeight);
}

UBossAnimInstance* AGruxChargeSummon::GetBossAnim() const
{
	return GetMesh() ? Cast<UBossAnimInstance>(GetMesh()->GetAnimInstance()) : nullptr;
}

AAIController* AGruxChargeSummon::GetAIController() const
{
	return Cast<AAIController>(GetController());
}
