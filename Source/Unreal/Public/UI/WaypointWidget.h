// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WaypointWidget.generated.h"

class UImage;
class UTextBlock;
class UWidget;

/**
 * Presentation base for the waypoint indicator. UWaypointComponent drives it; the Widget Blueprint
 * provides the visuals (MarkerRoot / ArrowImage / DistanceText) and optional show/hide polish.
 * MarkerRoot must be a Canvas Panel child anchored at (0,0) with alignment (0.5,0.5): its render
 * translation is the marker's screen position in Slate units.
 */
UCLASS()
class UNREAL_API UWaypointWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void UpdateIndicator(const FVector2D& ScreenPosition, float ArrowAngleDegrees);
	void SetOnScreenState(bool bOnScreen);
	void SetDistanceMeters(int32 Meters);
	void ShowIndicator();
	void HideIndicator();

	UFUNCTION(BlueprintPure, Category = "Waypoint")
	bool IsIndicatorShown() const { return bIndicatorShown; }

	UFUNCTION(BlueprintPure, Category = "Waypoint")
	int32 GetDisplayedMeters() const { return DisplayedMeters; }

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(BlueprintReadOnly, Category = "Waypoint", meta = (BindWidget))
	TObjectPtr<UWidget> MarkerRoot;

	UPROPERTY(BlueprintReadOnly, Category = "Waypoint", meta = (BindWidgetOptional))
	TObjectPtr<UImage> ArrowImage;

	UPROPERTY(BlueprintReadOnly, Category = "Waypoint", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DistanceText;

	// Format for the distance label; {0} is the distance in meters.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Waypoint")
	FText DistanceFormat = NSLOCTEXT("Waypoint", "DistanceFormat", "{0} m");

	// Called when the indicator should appear. Default: nothing extra (MarkerRoot is made visible).
	UFUNCTION(BlueprintNativeEvent, Category = "Waypoint")
	void OnIndicatorShown();

	// Called when the indicator should disappear. Default: collapses MarkerRoot.
	UFUNCTION(BlueprintNativeEvent, Category = "Waypoint")
	void OnIndicatorHidden();

	UFUNCTION(BlueprintImplementableEvent, Category = "Waypoint")
	void OnOnScreenStateChanged(bool bOnScreen);

	UFUNCTION(BlueprintImplementableEvent, Category = "Waypoint")
	void OnDistanceChanged(int32 Meters);

private:
	bool bIndicatorShown{ false };
	int32 DisplayedMeters{ -1 };
};
