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

    /** How far camera pans (in UU) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Look")
    float MaxLookOffset = 120.f;

    /** How quickly the camera moves to the target */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Look")
    float InterpSpeed = 7.f;

    /** Optional: specify a component name to search for (ex: CameraRoot) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Look")
    FName CameraRootName = TEXT("CameraRoot");

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    /** Cached CameraRoot (SceneComponent) */
    UPROPERTY()
    USceneComponent* CameraRoot;

    FVector TargetOffset;
};
