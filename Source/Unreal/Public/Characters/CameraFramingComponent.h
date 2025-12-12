// CameraFramingComponent.h

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CameraFramingComponent.generated.h"

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

    // Actors to frame
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    AActor* PlayerActor;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    AActor* TargetActor;

    // Camera parts
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    class USpringArmComponent* SpringArm;

    // Settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float ZoomMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MinZoom = 300.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MaxZoom = 1500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float RotationSmoothSpeed = 5.0f;

protected:
    virtual void BeginPlay() override;
};
