// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/WaypointWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"

void UWaypointWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	SetVisibility(ESlateVisibility::HitTestInvisible);
	if (MarkerRoot)
	{
		MarkerRoot->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (ArrowImage)
	{
		ArrowImage->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UWaypointWidget::UpdateIndicator(const FVector2D& ScreenPosition, float ArrowAngleDegrees)
{
	if (MarkerRoot)
	{
		MarkerRoot->SetRenderTranslation(ScreenPosition);
	}
	if (ArrowImage)
	{
		ArrowImage->SetRenderTransformAngle(ArrowAngleDegrees);
	}
}

void UWaypointWidget::SetOnScreenState(bool bOnScreen)
{
	if (ArrowImage)
	{
		ArrowImage->SetVisibility(bOnScreen ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
	OnOnScreenStateChanged(bOnScreen);
}

void UWaypointWidget::SetDistanceMeters(int32 Meters)
{
	DisplayedMeters = Meters;
	if (DistanceText)
	{
		DistanceText->SetText(FText::Format(DistanceFormat, FText::AsNumber(Meters)));
	}
	OnDistanceChanged(Meters);
}

void UWaypointWidget::ShowIndicator()
{
	if (bIndicatorShown)
	{
		return;
	}
	bIndicatorShown = true;
	if (MarkerRoot)
	{
		MarkerRoot->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	OnIndicatorShown();
}

void UWaypointWidget::HideIndicator()
{
	if (!bIndicatorShown)
	{
		return;
	}
	bIndicatorShown = false;
	OnIndicatorHidden();
}

void UWaypointWidget::OnIndicatorShown_Implementation()
{
}

void UWaypointWidget::OnIndicatorHidden_Implementation()
{
	if (MarkerRoot)
	{
		MarkerRoot->SetVisibility(ESlateVisibility::Collapsed);
	}
}
