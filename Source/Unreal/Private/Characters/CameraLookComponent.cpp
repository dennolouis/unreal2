#include "Characters/CameraLookComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/SceneComponent.h"

UCameraLookComponent::UCameraLookComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UCameraLookComponent::BeginPlay()
{
    Super::BeginPlay();

    // Find the CameraBoom automatically
    CameraBoom = GetOwner()->FindComponentByClass<USpringArmComponent>();

    if (!CameraBoom)
    {
        UE_LOG(LogTemp, Warning, TEXT("CameraLookComponent: No SpringArm found on Owner!"));
    }
}

void UCameraLookComponent::ApplyLookInput(FVector2D LookInput)
{
    // Clamp input to avoid mouse flick oversized values
    float X = FMath::Clamp(LookInput.X, -1.f, 1.f);
    float Y = FMath::Clamp(LookInput.Y, -1.f, 1.f);

    // Vertical: convert to target offset
    TargetVerticalOffset = Y * MaxVerticalOffset;

    // Horizontal: convert to desired yaw rotation
    TargetRelativeYaw = X * MaxYawRotation;
}

void UCameraLookComponent::TickComponent(
    float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!CameraBoom)
        return;

    // ====== 1. Vertical Camera Offset (socket offset) ======
    FVector CurrentOffset = CameraBoom->SocketOffset;

    float NewZ = FMath::FInterpTo(
        CurrentOffset.Z,
        TargetVerticalOffset,
        DeltaTime,
        VerticalInterpSpeed
    );

    float NewY = FMath::FInterpTo(
        CurrentOffset.Y,
        TargetRelativeYaw,
        DeltaTime,
        VerticalInterpSpeed
    );

    CurrentOffset.Z = NewZ;
	CurrentOffset.Y = NewY;
    CameraBoom->SocketOffset = CurrentOffset;
}
