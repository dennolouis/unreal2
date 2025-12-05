// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CameraLookComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UNREAL_API UCameraLookComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UCameraLookComponent();

    /** BP calls this from IA_Look */
    UFUNCTION(BlueprintCallable, Category = "Camera Look")
    void ApplyLookInput(FVector2D LookInput);

    // === CONFIG ===
    UPROPERTY(EditAnywhere, Category = "Camera")
    float MaxVerticalOffset = 50.f;

    UPROPERTY(EditAnywhere, Category = "Camera")
    float MaxYawRotation = 20.f;

    UPROPERTY(EditAnywhere, Category = "Camera")
    float VerticalInterpSpeed = 6.f;

    UPROPERTY(EditAnywhere, Category = "Camera")
    float RotationInterpSpeed = 6.f;

    UPROPERTY(EditAnywhere, Category = "Camera")
    float BaseYaw = 0.f;  // Default camera facing direction (often 0 or 90)

    UPROPERTY(EditAnywhere, Category = "Camera")
    float MinYaw = -20.f;

    UPROPERTY(EditAnywhere, Category = "Camera")
    float MaxYaw = 20.f;

private:

    class USpringArmComponent* CameraBoom = nullptr;

    float TargetVerticalOffset = 0.f;
    float TargetRelativeYaw = 0.f;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
};
