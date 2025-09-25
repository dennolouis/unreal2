#include "Combat/WeaponTraceComponent.h"
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/DamageType.h" // Include for damage handling
#include "Engine/DamageEvents.h"

// Constructor
UWeaponTraceComponent::UWeaponTraceComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

// BeginPlay: Find the weapon hitbox
void UWeaponTraceComponent::BeginPlay()
{
    Super::BeginPlay();

    AActor* OwnerActor = GetOwner();
    if (OwnerActor)
    {
        WeaponHitbox = OwnerActor->FindComponentByClass<UBoxComponent>();
    }
}

// StartAttack: Detect hits using the hitbox
void UWeaponTraceComponent::StartAttack()
{
    if (!WeaponHitbox) return;
    
    bIsAttacking = true;
    TargetsToIgnore.Empty();
}

// StopAttack: Reset hit tracking
void UWeaponTraceComponent::StopAttack()
{
    bIsAttacking = false;
    TargetsToIgnore.Empty();
}

void UWeaponTraceComponent::SetWeaponHitBox(UBoxComponent* HitBox)
{
    WeaponHitbox = HitBox;
}

void UWeaponTraceComponent::HandleTrace()
{
    if (!WeaponHitbox || !bIsAttacking || IsWeaponClashing()) return;

    FVector BoxExtent = WeaponHitbox->GetScaledBoxExtent();
    FVector Start = WeaponHitbox->GetComponentLocation();
    FVector End = Start;

    TArray<FHitResult> OutHits;
    FCollisionShape Box = FCollisionShape::MakeBox(BoxExtent);
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActors(TargetsToIgnore);
    QueryParams.AddIgnoredActor(GetOwner());
    QueryParams.AddIgnoredActor(ActorToIgnore);

    bool bHit = GetWorld()->SweepMultiByChannel(
        OutHits, Start, End, WeaponHitbox->GetComponentQuat(), ECollisionChannel::ECC_GameTraceChannel1, Box, QueryParams
    );

    if (bDebugMode)
    {
        FColor DebugColor = bHit ? FColor::Green : FColor::Red;
        DrawDebugBox(GetWorld(), Start, BoxExtent, WeaponHitbox->GetComponentQuat(), DebugColor, false, 1.0f, 0, 2.0f);
    }

    for (const FHitResult& Hit : OutHits)
    {
        AActor* HitActor = Hit.GetActor();
        if (!HitActor || TargetsToIgnore.Contains(HitActor)) continue;

        FDamageEvent DamageEvent; // Proper instantiation
        HitActor->TakeDamage(WeaponStrength * HitMultiplier, DamageEvent, GetOwner()->GetInstigatorController(), GetOwner());
        TargetsToIgnore.AddUnique(HitActor);

        if (HitParticleTemplate)
        {
            UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), HitParticleTemplate, Hit.ImpactPoint);
        }

        OnSuccessfulHitDelegate.Broadcast();
    }
}

bool UWeaponTraceComponent::IsWeaponClashing()
{
    FVector BoxExtent = WeaponHitbox->GetScaledBoxExtent();
    FVector Start = WeaponHitbox->GetComponentLocation();
    FVector End = Start;

    TArray<FHitResult> OutHits;
    FCollisionShape Box = FCollisionShape::MakeBox(BoxExtent);
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActors(TargetsToIgnore);
    QueryParams.AddIgnoredActor(GetOwner());
    QueryParams.AddIgnoredActor(ActorToIgnore);

    TArray<FHitResult> OutClashHits;
    bool bClashHit = GetWorld()->SweepMultiByChannel(
        OutClashHits, Start, End, WeaponHitbox->GetComponentQuat(), ECC_Visibility, Box, QueryParams
    );

    for (const FHitResult& Hit : OutClashHits)
    {
        AActor* HitActor = Hit.GetActor();
        if (!HitActor) continue;

        // ✅ Check if we hit another weapon
        UWeaponTraceComponent* OtherWeaponTrace = HitActor->FindComponentByClass<UWeaponTraceComponent>();
        if (OtherWeaponTrace && OtherWeaponTrace->GetIsAttacking())
        {
            // Trigger clash logic
            UE_LOG(LogTemp, Warning, TEXT("Weapon Clash Detected between %s and %s"),
                *GetOwner()->GetName(), *HitActor->GetName());
            
            OnSuccessfulHitDelegate.Broadcast();
            OnWeaponClashDelegate.Broadcast(HitActor);
            
            return true;
        }
    }


    return false;
}
