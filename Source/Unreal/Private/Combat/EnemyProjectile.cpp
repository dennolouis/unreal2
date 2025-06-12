// Fill out your copyright notice in the Description page of Project Settings.


#include "Combat/EnemyProjectile.h"
#include "Particles/ParticleSystemComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/DamageEvents.h"

// Sets default values
AEnemyProjectile::AEnemyProjectile()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AEnemyProjectile::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AEnemyProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AEnemyProjectile::HandleBeginOverLapEvent(AActor* OtherActor)
{
	APawn* PawnRef{ Cast<APawn>(OtherActor) };

	if (!PawnRef || !PawnRef->IsPlayerControlled()) { return; }

	FindComponentByClass<UParticleSystemComponent>()
		->SetTemplate(HitTemplate);

	FindComponentByClass<UProjectileMovementComponent>()
		->StopMovementImmediately();

	FTimerHandle DeathTimerHandle{};

	GetWorldTimerManager().SetTimer(
		DeathTimerHandle,
		this,
		&AEnemyProjectile::DestroyProjectile,
		0.5f
	);

	FindComponentByClass<USphereComponent>()
		->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	FDamageEvent ProjectileAttackEvent{};

	PawnRef->TakeDamage(
		Damage,
		ProjectileAttackEvent,
		PawnRef->GetController(),
		this
	);
}

void AEnemyProjectile::DestroyProjectile()
{
	Destroy();
}

