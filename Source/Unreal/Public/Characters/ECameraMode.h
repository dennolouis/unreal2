// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

UENUM(BlueprintType)
enum ECameraMode : uint8
{
	ThirdPerson UMETA(DisplayName = "Third Person"),
	TopDownSide UMETA(DisplayName = "Top Down Side")
};
