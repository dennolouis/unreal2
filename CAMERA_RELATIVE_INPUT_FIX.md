# Camera-Relative Input Direction - Implementation Guide

## Problem Solved

? **Before**: Character rotated to world-space direction (Y input = Y axis rotation)  
? **After**: Character rotates relative to camera direction (natural WASD-style movement)

## New Function

A new Blueprint-callable function has been added to `CombatComponent`:

```cpp
SetCameraRelativeInputDirection(FVector InputDirection)
```

### How It Works

This function converts your raw input (which is typically in "forward/right" format) into world-space that's relative to where the camera is pointing.

**Input Format**: A 2D vector where:
- **X component** = Forward/Backward input (-1 to 1)
- **Y component** = Right/Left input (-1 to 1)

**Example inputs:**
- `(1, 0)` = Moving forward (relative to camera)
- `(0, 1)` = Moving right (relative to camera)
- `(-1, 0.5)` = Moving back-right
- `(0.7, 0.7)` = Moving forward-right diagonally

## How to Update Your Blueprint

In your Blueprint where you're handling the `IA_Move` action:

### Old Way (Incorrect - World Space)
```
IA_Move Input Value ? Set Input Direction ? [Direct World Value]
```

### New Way (Correct - Camera Relative)
```
IA_Move Input Value 
  ?
  [Create FVector: X = Value.Y (forward), Y = Value.X (right)]
  ?
  Combat Component ? Set Camera Relative Input Direction
```

## Step-by-Step Blueprint Instructions

1. **Get the movement input value** from your `IA_Move` action callback
   - This typically gives you an `FInputActionValue` with X and Y components

2. **Convert to FVector format**:
   - Create a new `FVector`
   - Set `X = Input.Value.Y` (Forward/Back axis)
   - Set `Y = Input.Value.X` (Right/Left axis)  ? Note the swap!
   - Set `Z = 0`

3. **Call the new function**:
   ```
   Get Combat Component ? Set Camera Relative Input Direction ? [Your FVector]
   ```

## Code Example (If Using C++)

If you're handling this in C++, here's how:

```cpp
// In your input callback for IA_Move
void AMainCharacter::OnMoveTriggered(const FInputActionValue& Value)
{
    const FVector2D InputValue = Value.Get<FVector2D>();

    // Create camera-relative input (swap X/Y based on your input mapping)
    FVector CameraRelativeInput(InputValue.Y, InputValue.X, 0.0f);

    if (CombatComp)
    {
        CombatComp->SetCameraRelativeInputDirection(CameraRelativeInput);
    }
}
```

## What Happens Internally

The function does this:

1. Gets the player controller's camera direction (Yaw rotation)
2. Calculates forward and right vectors based on camera orientation
3. Combines input as: `(CameraForward * ForwardInput) + (CameraRight * RightInput)`
4. Normalizes the result to get a pure direction
5. Stores it in `LastInputDirection` for attacks to use

## Expected Behavior After Fix

? Press W ? Character looks forward (relative to camera)  
? Press D ? Character looks right (relative to camera)  
? Press A ? Character looks left (relative to camera)  
? Press S ? Character looks backward (relative to camera)  
? Lock-on still works and takes priority  
? No input ? Combo continues in last facing direction  

## Testing Checklist

- [ ] Press W while attacking - character faces camera-forward
- [ ] Press A while attacking - character faces camera-left  
- [ ] Press D while attacking - character faces camera-right
- [ ] Move stick in various directions - smooth rotation follows input
- [ ] Lock-on mode - character still faces target (not affected by this change)
- [ ] Standing still during combo - maintains direction (no change)

## Input Axis Mapping Reference

If your input setup is different, you might need to adjust how you create the FVector:

**Standard WASD mapping:**
- W/Up = Forward (InputValue.Y)
- A/Left = Left (negative InputValue.X)
- S/Down = Backward (negative InputValue.Y)
- D/Right = Right (InputValue.X)

So the conversion would be:
```cpp
FVector CameraRelativeInput(InputValue.Y,  // Forward is X
                             InputValue.X,  // Right is Y
                             0.0f);
```

If your mapping is different, you might need:
```cpp
FVector CameraRelativeInput(-InputValue.Y,  // If inverted
                             -InputValue.X,  // If inverted
                             0.0f);
```

## Debugging

If it's still not working correctly:

1. **Check your input value format** - Print the InputValue before sending it
2. **Check input axis mapping** - Verify W/A/S/D are mapped correctly
3. **Check camera rotation** - Verify the camera is pointing in the expected direction
4. **Enable debug drawing** - Add a line in the Blueprint to draw the direction vector

You can also temporarily add this to see the direction:
```cpp
DrawDebugLine(GetWorld(), CharacterRef->GetActorLocation(), 
              CharacterRef->GetActorLocation() + (LastInputDirection * 100.0f),
              FColor::Green, false, 0.1f, 0, 2.0f);
```
 