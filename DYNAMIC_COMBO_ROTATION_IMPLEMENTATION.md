# Dynamic Combo Rotation Implementation - Summary

## Overview
This feature enables your character to change attack direction during combo chains when **NOT locked on** to a target, implementing a system similar to the dynamic combat found in hack-and-slash action games. When locked on, the original behavior is preserved (character faces the target for one frame at the start of each attack).

## What Changed

### 1. **CombatComponent.h** (Public/Combat/)
Added a new member variable to track the player's current input direction:
```cpp
FVector LastInputDirection{ FVector::ZeroVector };
```

Added a public setter function for combat to receive input direction updates:
```cpp
UFUNCTION(BlueprintCallable)
void SetInputDirection(FVector InputDirection) { LastInputDirection = InputDirection; }
```

### 2. **CombatComponent.cpp** (Private/Combat/)

#### Modified `ComboAttack()`:
Now checks if locked on:
- **If locked on**: Uses the original behavior (face target for one frame)
- **If NOT locked on**: Rotates character towards the player's current movement input direction

```cpp
ULockOnComponent* LockOnComp = CharacterRef->FindComponentByClass<ULockOnComponent>();
if (LockOnComp && LockOnComp->GetCurrentTargetActor())
{
    LockOnComp->FaceCurrentTargetForOneFrame();
}
else if (!LastInputDirection.IsNearlyZero())
{
    // Not locked on and player has input direction - rotate character towards input
    FRotator NewRotation = UKismetMathLibrary::MakeRotFromX(LastInputDirection);
    CharacterRef->SetActorRotation(FRotator(0.0f, NewRotation.Yaw, 0.0f));
}
```

#### Modified `HeavyAttack()`:
Applied the same logic for heavy attacks to ensure consistency.

### 3. **MainCharacter.cpp** (Private/Characters/)

#### Updated `Tick()` function:
Now actively tracks movement input and sends it to the CombatComponent:

```cpp
void AMainCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Update the combat component with current movement direction input
    if (CombatComp)
    {
        FVector CurrentVelocity = GetCharacterMovement()->Velocity;

        // Only update if there's meaningful movement input
        if (CurrentVelocity.SizeSquared() > 100.0f)
        {
            CurrentVelocity.Z = 0.0f; // Only care about horizontal direction
            FVector InputDirection = CurrentVelocity.GetSafeNormal();
            CombatComp->SetInputDirection(InputDirection);
        }
    }
}
```

#### Added include:
Added `#include "GameFramework/CharacterMovementComponent.h"` to access character movement velocity.

## How It Works

1. **Every Frame**: MainCharacter::Tick() reads the character's current velocity from CharacterMovementComponent
2. **Direction Calculation**: The velocity is normalized to get a pure direction vector (ignoring height)
3. **Combat Sync**: This direction is sent to CombatComponent via `SetInputDirection()`
4. **Attack Rotation**: When an attack is initiated:
   - If locked on ? character faces the target (original behavior)
   - If NOT locked on ? character rotates to face the last recorded input direction
   - If no input ? character maintains current facing direction

## Gameplay Benefits

? **Multi-enemy combat**: You can now switch between enemies mid-combo by moving the stick in different directions  
? **Smooth flow**: Allows for fluid repositioning without breaking combo chains  
? **Input-responsive**: The character's facing direction updates based on real-time player input  
? **Lock-on preservation**: Lock-on mechanic still works exactly as before  

## Technical Details

### Input Direction Threshold
The implementation uses a `SizeSquared() > 100.0f` threshold to avoid registering noise from minimal input. This prevents the character from rotating based on stick drift or slight unintended movements.

### Rotation Application
Only the **Yaw (Z-axis rotation)** is applied to keep the character upright:
```cpp
CharacterRef->SetActorRotation(FRotator(0.0f, NewRotation.Yaw, 0.0f));
```

### When Input Direction is Not Set
If the player is stationary (no movement input), `LastInputDirection` remains the last movement direction. The character won't rotate until new input is provided, maintaining the attack's direction consistency.

## Testing Recommendations

1. **Lock-on mode**: Verify attacks still face the target as before
2. **No lock-on, stationary**: Verify combo chains stay in one direction
3. **No lock-on, moving**: Verify character rotates to face movement direction between attacks
4. **Rapid direction changes**: Test that switching stick direction mid-combo updates character facing smoothly
5. **Multiple enemies**: Test combat flow when fighting several enemies

## Future Enhancements

- Add configurable rotation speed for smoother transitions
- Add optional screen-space or camera-relative input direction
- Add input buffer window to allow early direction input before next attack starts
- Add option to normalize input from gamepad vs keyboard input differently
