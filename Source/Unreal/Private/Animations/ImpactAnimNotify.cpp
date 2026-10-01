#include "Animations/ImpactAnimNotify.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SkeletalMeshComponent.h"

#if ENGINE_MAJOR_VERSION > 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 8)
void UImpactAnimNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
#else
void UImpactAnimNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
#endif
{
    if (!MeshComp) return;

    AActor* Owner = MeshComp->GetOwner();
    if (!Owner) return;

    FVector Location = MeshComp->GetComponentLocation();
    FRotator Rotation = MeshComp->GetComponentRotation();

    if (SocketName != NAME_None && MeshComp->DoesSocketExist(SocketName))
    {
        Location = MeshComp->GetSocketLocation(SocketName);
        Rotation = MeshComp->GetSocketRotation(SocketName);
    }

    // Play camera shake on the player controller's camera
    if (CameraShake)
    {
        if (APlayerController* PC = Owner->GetWorld()->GetFirstPlayerController())
        {
            PC->ClientStartCameraShake(CameraShake);
        }
    }

    // Spawn impact system
    if (ImpactSystem)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            Owner->GetWorld(),
            ImpactSystem,
            Location,
            Rotation,
            SpawnScale,
            bAutoDestroy
        );
    }

    // Spawn extra VFX
    if (ExtraVFX)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            Owner->GetWorld(),
            ExtraVFX,
            Location,
            Rotation,
            SpawnScale,
            bAutoDestroy
        );
    }
}
