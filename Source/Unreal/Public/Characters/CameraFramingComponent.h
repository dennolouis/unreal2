#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CameraFramingComponent.generated.h"

class USceneComponent;
class USpringArmComponent;

UCLASS(ClassGroup = (Camera), meta = (BlueprintSpawnableComponent))
class UNREAL_API UCameraFramingComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UCameraFramingComponent();

    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction
    ) override;

    // --- Actors ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Framing")
    AActor* PlayerActor = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Framing")
    AActor* TargetActor = nullptr;

    // --- Camera Components ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Framing")
    USceneComponent* CameraRoot = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Framing")
    USpringArmComponent* SpringArm = nullptr;

    UPROPERTY(EditAnywhere, Category = "Camera Framing")
    float ScreenPaddingFactor = 1.2f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    FRotator CameraBoomRotationOffset = FRotator::ZeroRotator;

    // --- Zoom ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Framing|Zoom")
    float ZoomMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Framing|Zoom")
    float MinZoom = 600.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Framing|Zoom")
    float MaxZoom = 1600.f;

    // --- Smoothing ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Framing|Smoothing")
    float HorizontalSmoothSpeed = 6.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Framing|Smoothing")
    float VerticalSmoothSpeed = 4.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Framing|Smoothing")
    float ZoomSmoothSpeed = 5.0f;


    // --- Composition ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Framing|Composition")
    float TargetHorizontalInfluence = 0.3f; // 0 = player only, 1 = full midpoint

protected:
    virtual void BeginPlay() override;

private:
    FVector InitialCameraRootLocation;
    float InitialArmLength = 0.f;
    bool bHasCachedDefaults = false;
    bool bWasFramingTarget = false;

    FVector SmoothedCenterCamSpace;

    void UpdateFramingCamera(float DeltaTime);
    void ReturnToDefaultCamera(float DeltaTime);
};
