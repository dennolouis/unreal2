# Blueprint Setup - Visual Guide

## TL;DR

Instead of calling:
```
Combat Component ? Set Input Direction
```

Call:
```
Combat Component ? Set Camera Relative Input Direction
```

With the input value properly formatted.

---

## Complete Blueprint Flow

Your Blueprint should look like this:

### Option 1: Simple Vector Construction
```
Enhanced Input Value (from IA_Move)
    |
    v
[Struct - Break InputActionValue]  ? Get X, Y components
    |
    v
[Make Vector]
    X: Input.Y (forward/backward)
    Y: Input.X (right/left)
    Z: 0.0
    |
    v
Get Owner (MainCharacter)
    |
    v
Get CombatComponent
    |
    v
Set Camera Relative Input Direction [Vector]
```

### Option 2: More Detailed Blueprint (Node Names)
```
Event Input (IA_Move)
    ?
Get Value
    ?
Print to create temp FInputActionValue
    ?
Construct Vector
    - X Pin: Get .Y from input
    - Y Pin: Get .X from input
    - Z Pin: 0
    ?
Cast to MainCharacter
    ?
Get Combat Component
    ?
Call Set Camera Relative Input Direction
    - Input Direction: [Your Vector]
```

---

## If Your Input Is Already a FVector2D

If you're already converting to FVector2D, just pass it like this:

```cpp
// C++ Example
FVector2D InputVec = GetMovementInput();  // Returns (-1 to 1, -1 to 1)

FVector CameraRelativeVec(InputVec.Y, InputVec.X, 0.0f);
CombatComponent->SetCameraRelativeInputDirection(CameraRelativeVec);
```

In Blueprint:
```
Movement Input (FVector2D)
    ?
Make Vector(InputVec.Y, InputVec.X, 0.0)
    ?
Set Camera Relative Input Direction
```

---

## Comparison: Before vs After

### ? Old (Incorrect) Blueprint
```
IA_Move
  ?
Get Value.X and Value.Y
  ?
Make Vector(X, Y, 0)
  ?
Combat Component ? Set Input Direction ? WRONG FUNCTION
```

**Result**: Character rotates to world-space cardinal directions

### ? New (Correct) Blueprint
```
IA_Move
  ?
Get Value.X and Value.Y
  ?
Make Vector(Y, X, 0) ? SWAPPED!
  ?
Combat Component ? Set Camera Relative Input Direction ? CORRECT FUNCTION
```

**Result**: Character rotates relative to camera orientation

---

## Testing Your Blueprint Setup

Add this debugging sequence after calling SetCameraRelativeInputDirection:

```
Set Camera Relative Input Direction
  ?
Print String: "Input Direction Set"
  ?
Get Combat Component ? Get Last Input Direction (getter)
  ?
Print String: Direction Value
```

You should see the direction being updated in the output log as you move.

---

## Common Mistakes to Avoid

? **Mistake 1**: Passing input directly without converting
```
IA_Move ? Set Camera Relative Input Direction  ? WRONG (wrong type)
```

? **Correct**:
```
IA_Move ? Make Vector ? Set Camera Relative Input Direction
```

? **Mistake 2**: Not swapping X/Y
```
Make Vector(Input.X, Input.Y, 0)  ? WRONG (not swapped)
```

? **Correct**:
```
Make Vector(Input.Y, Input.X, 0)  ? RIGHT (swapped)
```

? **Mistake 3**: Using old function
```
Combat Component ? Set Input Direction  ? OLD FUNCTION
```

? **Correct**:
```
Combat Component ? Set Camera Relative Input Direction  ? NEW FUNCTION
```

---

## Input Value Type Reference

If you're unsure what type your IA_Move callback returns:

### FInputActionValue (EnhancedInput Plugin)
```
FInputActionValue
    ?? Value (FVector)
    ?? X: float
    ?? Y: float
```

### FVector2D (If Using This Type)
```
FVector2D
    ?? X: float
    ?? Y: float
```

Both can be converted to a FVector by swapping:
```
FVector(InputValue.Y, InputValue.X, 0.0f)
```

---

## If You Want to Verify It's Working

Add debug visualization:

```cpp
// In CombatComponent.cpp, add to ComboAttack():

#if !UE_BUILD_SHIPPING
if (!LastInputDirection.IsNearlyZero())
{
    DrawDebugLine(
        GetWorld(),
        CharacterRef->GetActorLocation(),
        CharacterRef->GetActorLocation() + (LastInputDirection * 150.0f),
        FColor::Green,
        false,
        0.05f,
        0,
        3.0f
    );
}
#endif
```

This will draw a green line showing your current input direction. If it matches your stick/WASD input, it's working!

---

## Next Steps

1. Update your Blueprint with the new function
2. Test with WASD/Gamepad sticks
3. Verify character rotates relative to camera
4. Move to fixing the second issue (input always being set)
