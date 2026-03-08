// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "TeleportCameraAnimNotifyState.generated.h"

class USpringArmComponent;

UCLASS(meta=(DisplayName = "Teleport Camera State"))
class UNREAL_API UTeleportCameraAnimNotifyState : public UAnimNotifyState
{
    GENERATED_BODY()

public:
    // Side offset in world units applied along the character's right vector
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TeleportCamera")
    float SideOffset = 400.0f;

    // Yaw rotation to apply to the spring arm for side view (degrees). Positive rotates to the right.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TeleportCamera")
    float RotationYawDegrees = 45.0f;

    // Amount to change the spring arm length (negative to zoom in)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TeleportCamera")
    float ArmLengthOffset = -300.0f;

    // Fraction of the notify duration used to blend in
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TeleportCamera", meta=(ClampMin=0.0, ClampMax=0.5))
    float BlendInFraction = 0.15f;

    // Fraction of the notify duration used to blend out
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TeleportCamera", meta=(ClampMin=0.0, ClampMax=0.5))
    float BlendOutFraction = 0.15f;

protected:
    // runtime state
    FVector InitialTargetOffset;
    float InitialArmLength{0.0f};
    FVector TargetOffset;
    float TargetArmLength{0.0f};
    FRotator InitialRelativeRotation;
    FRotator TargetRelativeRotation;
    float Elapsed{0.0f};
    float Duration{0.0f};
    bool bInitialized{false};
    TWeakObjectPtr<USpringArmComponent> SpringArm;

public:
    virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration) override;
    virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime) override;
    virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;
};
