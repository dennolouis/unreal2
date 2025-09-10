// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/FTraceSockets.h"
#include "Combat/EAttackType.h"
#include "TraceComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE(
	FOnSuccessfulTraceHitSignature,
	UTraceComponent, OnSuccessfulTraceHitDelegate
);


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UNREAL_API UTraceComponent : public UActorComponent
{
	GENERATED_BODY()

	USkeletalMeshComponent* SkeletalComp;

	UPROPERTY(EditAnywhere)
	//TArray<FTraceSockets> Sockets;
	TMap<TEnumAsByte<EAttackType>, FTraceSockets> Sockets;

	UPROPERTY(EditAnywhere)
	double BoxCollisionLength{ 30.0 };

	UPROPERTY(EditAnywhere)
	bool bDebugMode{ false };

	TArray<AActor*> TargetsToIgnore;

	UPROPERTY(EditAnywhere)
	UParticleSystem* HitParticleTemplate;

	EAttackType CurrentAttack;

public:	
	// Sets default values for this component's properties
	UTraceComponent();

	UPROPERTY(BlueprintAssignable)
	FOnSuccessfulTraceHitSignature OnSuccessfulTraceHitDelegate;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable)
	void HandleResetAttack();

	UFUNCTION(BlueprintCallable)
	void StartAttack(EAttackType AttackType);

	UFUNCTION(BlueprintCallable)
	void EndAttack() { bIsAttacking = false; HandleResetAttack(); }

	UPROPERTY(VisibleAnywhere)
	bool bIsAttacking{ false };

};
