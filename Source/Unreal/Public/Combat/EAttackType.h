// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

UENUM(BlueprintType)
enum EAttackType
{
	NoneSelected UMETA(DisplayName = "None Selected"),
    RightPunch UMETA(DisplayName = "Right Punch"),
    LeftPunch UMETA(DisplayName = "Left Punch"),
    RightKick UMETA(DisplayName = "Right Kick"),
    LeftKick UMETA(DisplayName = "Left Kick")
};
