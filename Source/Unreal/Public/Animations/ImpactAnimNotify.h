#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "ImpactAnimNotify.generated.h"

class UNiagaraSystem;
class UCameraShakeBase;

UCLASS(meta=(DisplayName = "Impact Notify"))
class UNREAL_API UImpactAnimNotify : public UAnimNotify
{
    GENERATED_BODY()

public:
    // Optional camera shake to play on impact
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Impact")
    TSubclassOf<UCameraShakeBase> CameraShake;

    // Niagara system to spawn at impact location
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Impact")
    UNiagaraSystem* ImpactSystem;

    // Additional VFX system to spawn (e.g., sparks)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Impact")
    UNiagaraSystem* ExtraVFX;

    // Optional socket on the mesh to use for spawn location; if None will use component location
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Impact")
    FName SocketName = NAME_None;

    // Scale for spawned systems
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Impact")
    FVector SpawnScale = FVector::OneVector;

    // Whether to auto-destroy spawned systems (Niagara spawn has auto-destroy flag)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Impact")
    bool bAutoDestroy = true;

    virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;
};
