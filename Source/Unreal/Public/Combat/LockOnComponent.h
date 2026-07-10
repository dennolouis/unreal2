// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LockOnComponent.generated.h"


DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_OneParam(
    FOnUpdatedTargetSignature,
    ULockOnComponent, OnUpdatedTargetDelegate,
    AActor*, NewTargetActorRef
);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UNREAL_API ULockOnComponent : public UActorComponent
{
    GENERATED_BODY()

    ACharacter* OwnerRef;

    APlayerController* Controller;

    class UCharacterMovementComponent* MovementComp;

    class USpringArmComponent* SpringArmComp;

public:    
    // Sets default values for this component's properties
    ULockOnComponent();

    AActor* CurrentTargetActor;

    UPROPERTY(BlueprintAssignable)
    FOnUpdatedTargetSignature OnUpdatedTargetDelegate;

    // Temporarily pause/resume control of the camera by this component.
    UFUNCTION(BlueprintCallable)
    void PauseCameraControl();

    UFUNCTION(BlueprintCallable)
    void ResumeCameraControl();

    // Whether the lock-on component is allowed to update the controller rotation
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    bool bCameraControlEnabled{ true };

protected:
    // Called when the game starts
    virtual void BeginPlay() override;

    UFUNCTION(BlueprintCallable)
    void StartLockOn(float Radius = 750.0f);


    UFUNCTION(BlueprintCallable)
    void ToggleLockOn(float Radius = 750.0f);

    UPROPERTY(EditAnywhere)
    double BreakDistance{ 1000.0 };

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    bool bIsSideScroller{ false };

    // Radius to search for nearby targets when switching
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float TargetSwitchRadius{ 1500.0f };

    // Threshold for joystick input to trigger target switch
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float JoystickSwitchThreshold{ 0.5f };

    // Angle threshold for considering a target as "next" target
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float AngleThreshold{ 45.0f };

private:
    // Array of nearby valid targets
    TArray<AActor*> NearbyTargets;

    // Buffer for joystick input to prevent constant switching
    float JoystickInputBuffer{ 0.0f };

    // Helper function to find all nearby valid targets
    void FindNearbyTargets();

    // Find the next target based on direction angle (0 = right/clockwise, 180 = left/counter-clockwise)
    AActor* FindNextTarget(float DirectionAngle);

    // Setup input bindings
    void SetupInput();

    // Handle target switch input from joystick or keyboard
    void OnTargetSwitchInput(float AxisValue);

public:    
    // Called every frame
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UFUNCTION(BlueprintCallable)
    void EndLockOn();

    UFUNCTION(BlueprintCallable)
    void FaceCurrentTargetForOneFrame();

    // Switch to the next target (clockwise when viewed from above)
    UFUNCTION(BlueprintCallable)
    void CycleNextTarget();

    // Switch to the previous target (counter-clockwise when viewed from above)
    UFUNCTION(BlueprintCallable)
    void CyclePreviousTarget();

    AActor* GetCurrentTargetActor() { return CurrentTargetActor; }
    
};

