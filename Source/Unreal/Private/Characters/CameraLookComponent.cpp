#include "Characters/CameraLookComponent.h"
#include "GameFramework/Actor.h"
#include "Components/SceneComponent.h"

UCameraLookComponent::UCameraLookComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UCameraLookComponent::BeginPlay()
{
    Super::BeginPlay();

    // Option 1: Find by name (recommended for predictable setup)
    if (!CameraRootName.IsNone())
    {
        CameraRoot = Cast<USceneComponent>(
            GetOwner()->GetDefaultSubobjectByName(CameraRootName)
        );
    }

    // Option 2: fallback - first scene component (optional)
    if (!CameraRoot)
    {
        CameraRoot = GetOwner()->FindComponentByClass<USceneComponent>();
    }

    if (!CameraRoot)
    {
        UE_LOG(LogTemp, Warning, TEXT("CameraLookComponent: No CameraRoot found."));
    }
}

void UCameraLookComponent::ApplyLookInput(FVector2D LookInput)
{
    if (!GetOwner()) return;

    // Character right/left direction
    const FVector Right = GetOwner()->GetActorRightVector();

    // Vertical offset always uses world up
    const FVector Up = FVector::UpVector;

    // Build world-space target offset
    TargetOffset =
        (Right * LookInput.X * MaxLookOffset) +
        (Up * LookInput.Y * MaxLookOffset);
}

void UCameraLookComponent::TickComponent(
    float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!CameraRoot || !GetOwner()) return;

    // Where the boom *should* be (world space)
    const FVector TargetWorldLocation =
        GetOwner()->GetActorLocation() + TargetOffset;

    // Current boom world location
    const FVector CurrentLocation = CameraRoot->GetComponentLocation();

    // Smooth interpolate
    const FVector NewLocation = FMath::VInterpTo(
        CurrentLocation,
        TargetWorldLocation,
        DeltaTime,
        InterpSpeed
    );

    CameraRoot->SetWorldLocation(NewLocation);
}
