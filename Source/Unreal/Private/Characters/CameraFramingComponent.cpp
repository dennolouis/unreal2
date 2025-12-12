// CameraFramingComponent.cpp

#include "Characters/CameraFramingComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/KismetMathLibrary.h"

UCameraFramingComponent::UCameraFramingComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UCameraFramingComponent::BeginPlay()
{
    Super::BeginPlay();

    AActor* Owner = GetOwner();

    // --- Find player ---
    PlayerActor = Owner;

    if (!PlayerActor)
    {
        UE_LOG(LogTemp, Error, TEXT("[CameraFramingComponent] No Player Pawn found!"));
    }

    if (!TargetActor)
    {
        UE_LOG(LogTemp, Warning, TEXT("[CameraFramingComponent] No TargetActor assigned. The camera will only follow Player!"));
    }

    // --- Find SpringArm in owner ---
    SpringArm = Owner->FindComponentByClass<USpringArmComponent>();

    if (!SpringArm)
    {
        UE_LOG(LogTemp, Error, TEXT("[CameraFramingComponent] No SpringArmComponent found on %s!"), *Owner->GetName());
    }
}

void UCameraFramingComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction
)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!PlayerActor || !TargetActor || !SpringArm)
        return;

    FVector PlayerPos = PlayerActor->GetActorLocation();
    FVector TargetPos = TargetActor->GetActorLocation();

    // Midpoint between them
    FVector Midpoint = (PlayerPos + TargetPos) * 0.5f;

    // Desired rotation to look at midpoint
    FRotator DesiredRot = UKismetMathLibrary::FindLookAtRotation(
        SpringArm->GetComponentLocation(),
        Midpoint
    );

    FRotator SmoothRot = FMath::RInterpTo(
        SpringArm->GetComponentRotation(),
        DesiredRot,
        DeltaTime,
        RotationSmoothSpeed
    );

    SpringArm->SetWorldRotation(SmoothRot);

    // Distance → adjust arm length
    float Distance = FVector::Distance(PlayerPos, TargetPos);
    float DesiredArmLength = FMath::Clamp(Distance * ZoomMultiplier, MinZoom, MaxZoom);
    SpringArm->TargetArmLength = DesiredArmLength;
}
