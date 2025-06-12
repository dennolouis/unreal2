#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WeaponTraceComponent.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class UNREAL_API UWeaponTraceComponent : public UActorComponent
{
    GENERATED_BODY()

private:
    UPROPERTY(EditAnywhere, Category = "Weapon Trace")
    TArray<AActor*> TargetsToIgnore;

    AActor* ActorToIgnore;

    UPROPERTY(EditAnywhere, Category = "Weapon Trace")
    bool bDebugMode = false;

    UPROPERTY(EditAnywhere, Category = "Weapon Trace")
    UParticleSystem* HitParticleTemplate;

    UPROPERTY(VisibleAnywhere)
    class UBoxComponent* WeaponHitbox;

    UPROPERTY(VisibleAnywhere)
    bool bIsAttacking{ false };

    float WeaponStrength{ 10.0f };
    float HitMultiplier{ 1.0f };

public:
    UWeaponTraceComponent();

protected:
    virtual void BeginPlay() override;

public:
    UFUNCTION(BlueprintCallable)
    void StartAttack();

    UFUNCTION(BlueprintCallable)
    void StopAttack();

    UFUNCTION(BlueprintCallable)
    void SetWeaponHitBox(UBoxComponent* HitBox);

    UFUNCTION(BlueprintCallable)
    void HandleTrace();

    UFUNCTION(BlueprintCallable)
    void SetHitMultiplier(float Multipler) { HitMultiplier = Multipler; }

    void SetActorToIgnore(AActor* Ignore) { ActorToIgnore = Ignore; };
    void SetWeaponStrength(float Strength) { WeaponStrength = Strength; }

};
