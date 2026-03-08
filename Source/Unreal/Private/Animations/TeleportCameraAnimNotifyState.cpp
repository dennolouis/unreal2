#include "Animations/TeleportCameraAnimNotifyState.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/KismetMathLibrary.h"

void UTeleportCameraAnimNotifyState::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration)
{
    if (!MeshComp) return;

    ACharacter* OwnerChar = Cast<ACharacter>(MeshComp->GetOwner());
    if (!OwnerChar) return;

    SpringArm = OwnerChar->FindComponentByClass<USpringArmComponent>();
    if (!SpringArm.IsValid()) return;

    bInitialized = true;
    Duration = TotalDuration;
    Elapsed = 0.0f;
    InitialTargetOffset = SpringArm->TargetOffset;
    InitialArmLength = SpringArm->TargetArmLength;

    // compute target offset to the character's right
    FVector Right = OwnerChar->GetActorRightVector();
    TargetOffset = InitialTargetOffset + Right * SideOffset;
    TargetArmLength = InitialArmLength + ArmLengthOffset;

    // compute rotation target for the spring arm (relative rotation)
    InitialRelativeRotation = SpringArm->GetRelativeRotation();
    TargetRelativeRotation = InitialRelativeRotation + FRotator(0.0f, RotationYawDegrees, 0.0f);
}

void UTeleportCameraAnimNotifyState::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime)
{
    if (!bInitialized || !SpringArm.IsValid()) return;
    if (Duration <= 0.0f) return;

    Elapsed += FrameDeltaTime;
    float Alpha = Elapsed / Duration;

    // blend in and out using the configured fractions
    float BlendInEnd = FMath::Clamp(BlendInFraction, 0.0f, 0.5f);
    float BlendOutStart = 1.0f - FMath::Clamp(BlendOutFraction, 0.0f, 0.5f);

    float BlendAlpha = 1.0f;
    if (Alpha < BlendInEnd)
    {
        BlendAlpha = Alpha / BlendInEnd;
    }
    else if (Alpha > BlendOutStart)
    {
        BlendAlpha = (1.0f - Alpha) / (1.0f - BlendOutStart);
    }

    BlendAlpha = FMath::Clamp(BlendAlpha, 0.0f, 1.0f);

    FVector NewOffset = FMath::Lerp(InitialTargetOffset, TargetOffset, BlendAlpha);
    float NewArmLength = FMath::Lerp(InitialArmLength, TargetArmLength, BlendAlpha);

    // interpolate rotation via quaternions for smooth slerp
    FQuat StartQuat = InitialRelativeRotation.Quaternion();
    FQuat EndQuat = TargetRelativeRotation.Quaternion();
    FQuat NewQuat = FQuat::Slerp(StartQuat, EndQuat, BlendAlpha);
    FRotator NewRot = NewQuat.Rotator();

    SpringArm->TargetOffset = NewOffset;
    SpringArm->TargetArmLength = NewArmLength;
    SpringArm->SetRelativeRotation(NewRot);
}

void UTeleportCameraAnimNotifyState::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
    if (!bInitialized || !SpringArm.IsValid()) return;

    // restore original values
    SpringArm->TargetOffset = InitialTargetOffset;
    SpringArm->TargetArmLength = InitialArmLength;
    SpringArm->SetRelativeRotation(InitialRelativeRotation);

    bInitialized = false;
    SpringArm = nullptr;
}
