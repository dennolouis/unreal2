// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ECameraMode.h"
#include "CameraModeComponent.generated.h"

USTRUCT(BlueprintType)
struct FCameraModeSettings
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    float TargetArmLength = 400.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    FVector RelativeLocation = FVector(0.f, 0.f, 9.f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    FRotator RelativeRotation = FRotator(0.f, 0.f, 0.f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    bool bUsePawnControlRotation = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    bool bDetachCamera = false;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UNREAL_API UCameraModeComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UCameraModeComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	

    /** The camera boom and follow camera references */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    class USpringArmComponent* CameraBoom;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    class UCameraComponent* FollowCamera;

    /** Current camera mode */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    TEnumAsByte<ECameraMode> CurrentCameraMode;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    FCameraModeSettings SoulsLikeCameraSettings;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    FCameraModeSettings TopDownSideCameraSettings;

    /** Switch camera mode */
    UFUNCTION(BlueprintCallable, Category = "Camera")
    void SwitchCameraMode();

    /** Apply a specific mode */
    UFUNCTION(BlueprintCallable, Category = "Camera")
    void ApplyCameraMode(ECameraMode NewMode);

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		
};
