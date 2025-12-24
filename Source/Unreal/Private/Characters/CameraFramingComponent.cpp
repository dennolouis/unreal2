#include "Characters/CameraFramingComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Characters/CameraFramingComponent.h"


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

    // --- Find camera components by name (as you specified) ---
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

    // Disable any spring arm behavior that could fight us
    SpringArm->bEnableCameraLag = false;
    SpringArm->bEnableCameraRotationLag = false;
    SpringArm->bInheritPitch = false;
    SpringArm->bInheritYaw = false;
    SpringArm->bInheritRoll = false;

    // Cache defaults
    InitialCameraRootLocation = CameraRoot->GetComponentLocation();
    InitialArmLength = SpringArm->TargetArmLength;

    bHasCachedDefaults = true;
    bWasFramingTarget = false;
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

    if (!TargetActor)
    {
        if (bWasFramingTarget)
        {
            ReturnToDefaultCamera(DeltaTime);
        }

        bWasFramingTarget = false;
        return;
    }

    bWasFramingTarget = true;
    UpdateFramingCamera(DeltaTime);
}

void UCameraFramingComponent::UpdateFramingCamera(float DeltaTime)
{
    const FVector PlayerPos = PlayerActor->GetActorLocation();
    const FVector TargetPos = TargetActor->GetActorLocation();

    // --------------------------------------------------
    // WORLD-SPACE CENTER (XYZ SAFE)
    // --------------------------------------------------
    FVector DesiredCenter = (PlayerPos + TargetPos) * 0.5f;

    // Optional: give player more horizontal authority
    DesiredCenter = FMath::Lerp(
        PlayerPos,
        DesiredCenter,
        TargetHorizontalInfluence
    );

    // --------------------------------------------------
    // MOVE CAMERA ROOT (POSITION ONLY)
    // --------------------------------------------------
    FVector CurrentCamLocation = CameraRoot->GetComponentLocation();

    FVector DesiredCameraLocation(
        DesiredCenter.X,
        DesiredCenter.Y,
        CurrentCamLocation.Z // keep authored height
    );

    FVector SmoothedLocation = FMath::VInterpTo(
        CurrentCamLocation,
        DesiredCameraLocation,
        DeltaTime,
        HorizontalSmoothSpeed
    );

    CameraRoot->SetWorldLocation(SmoothedLocation);

    // --------------------------------------------------
    // ZOOM (BASED ON DISTANCE)
    // --------------------------------------------------
    const float Distance = FVector::Distance(PlayerPos, TargetPos);

    const float DesiredArmLength = FMath::Clamp(
        Distance * ZoomMultiplier,
        MinZoom,
        MaxZoom
    );

    SpringArm->TargetArmLength = FMath::FInterpTo(
        SpringArm->TargetArmLength,
        DesiredArmLength,
        DeltaTime,
        ZoomSmoothSpeed
    );
}

void UCameraFramingComponent::ReturnToDefaultCamera(float DeltaTime)
{
    FVector SmoothedLocation = FMath::VInterpTo(
        CameraRoot->GetComponentLocation(),
        InitialCameraRootLocation,
        DeltaTime,
        HorizontalSmoothSpeed
    );

    CameraRoot->SetWorldLocation(SmoothedLocation);

    SpringArm->TargetArmLength = FMath::FInterpTo(
        SpringArm->TargetArmLength,
        InitialArmLength,
        DeltaTime,
        ZoomSmoothSpeed
    );
}

