// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LockOnComponent.generated.h"


DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_OneParam(
	FOnUpdatedTargetSignature,
	ULockOnComponent, OnUpdatedTargetDelegate,
	AActor*, NewTargetActorRef
);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UNREAL_API ULockOnComponent : public UActorComponent
{
	GENERATED_BODY()

	ACharacter* OwnerRef;

	APlayerController* Controller;

	class UCharacterMovementComponent* MovementComp;

	class USpringArmComponent* SpringArmComp;

public:	
	// Sets default values for this component's properties
	ULockOnComponent();

	AActor* CurrentTargetActor;

	UPROPERTY(BlueprintAssignable)
	FOnUpdatedTargetSignature OnUpdatedTargetDelegate;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable)
	void StartLockOn(float Radius = 750.0f);


	UFUNCTION(BlueprintCallable)
	void ToggleLockOn(float Radius = 750.0f);

	UPROPERTY(EditAnywhere)
	double BreakDistance{ 1000.0 };

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable)
	void EndLockOn();

	UFUNCTION(BlueprintCallable)
	void FaceCurrentTargetForOneFrame();

	AActor* GetCurrentTargetActor() { return CurrentTargetActor; }
	
};
