#include "Characters/CameraFramingComponent.h"

#include "Components/SceneComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Actor.h"

UCameraFramingComponent::UCameraFramingComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UCameraFramingComponent::BeginPlay()
{
    Super::BeginPlay();

    PlayerActor = GetOwner();

    if (!PlayerActor)
    {
        UE_LOG(LogTemp, Error, TEXT("[CameraFramingComponent] No owning actor."));
        return;
    }

    // --- Find camera components by name ---
    CameraRoot = Cast<USceneComponent>(
        PlayerActor->GetDefaultSubobjectByName(TEXT("CameraRoot"))
    );

    SpringArm = Cast<USpringArmComponent>(
        PlayerActor->GetDefaultSubobjectByName(TEXT("CameraBoom"))
    );

    if (!CameraRoot || !SpringArm)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("[CameraFramingComponent] CameraRoot or CameraBoom not found.")
        );
        return;
    }

    const FRotator FinalBoomRotation =
        SpringArm->GetRelativeRotation() + CameraBoomRotationOffset;

    SpringArm->SetRelativeRotation(FinalBoomRotation);

    // Disable spring arm behavior that could fight us
    SpringArm->bEnableCameraLag = false;
    SpringArm->bEnableCameraRotationLag = false;
    SpringArm->bInheritPitch = false;
    SpringArm->bInheritYaw = false;
    SpringArm->bInheritRoll = false;

    // Cache defaults
    InitialCameraRootLocation = CameraRoot->GetComponentLocation();
    InitialArmLength = SpringArm->TargetArmLength;

    bHasCachedDefaults = true;
}

void UCameraFramingComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction
)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bHasCachedDefaults || !CameraRoot || !SpringArm || !PlayerActor)
        return;

    UpdateFramingCamera(DeltaTime);
}

void UCameraFramingComponent::UpdateFramingCamera(float DeltaTime)
{
    const FVector PlayerPos = PlayerActor->GetActorLocation();

    // --------------------------------------------------
    // DETERMINE DESIRED CENTER
    // --------------------------------------------------
    FVector DesiredCenter = PlayerPos;

    if (TargetActor)
    {
        const FVector TargetPos = TargetActor->GetActorLocation();

        FVector Midpoint = (PlayerPos + TargetPos) * 0.5f;

        // Player retains some authority (prevents flip/kick)
        DesiredCenter = FMath::Lerp(
            PlayerPos,
            Midpoint,
            TargetHorizontalInfluence
        );
    }

    // --------------------------------------------------
    // MOVE CAMERA ROOT (XYZ POSITION, FIXED HEIGHT)
    // --------------------------------------------------
    const FVector CurrentCamLocation = CameraRoot->GetComponentLocation();

    const FVector SmoothedLocation = FMath::VInterpTo(
        CurrentCamLocation,
        DesiredCenter,
        DeltaTime,
        HorizontalSmoothSpeed
    );

    CameraRoot->SetWorldLocation(SmoothedLocation);

    // --------------------------------------------------
    // ZOOM
    // --------------------------------------------------
    float DesiredArmLength = InitialArmLength;

    if (TargetActor)
    {
        const float Distance = FVector::Distance(
            PlayerPos,
            TargetActor->GetActorLocation()
        );

        const float PaddedDistance = Distance * ScreenPaddingFactor;

        DesiredArmLength = FMath::Clamp(
            PaddedDistance * ZoomMultiplier,
            MinZoom,
            MaxZoom
        );
    }

    SpringArm->TargetArmLength = FMath::FInterpTo(
        SpringArm->TargetArmLength,
        DesiredArmLength,
        DeltaTime,
        ZoomSmoothSpeed
    );
}
