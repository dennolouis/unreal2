# Blueprint Integration Guide - Dynamic Combo Rotation

## Overview
The C++ implementation is complete, but you may want to add optional Blueprint nodes to fine-tune the behavior. Here are some suggestions:

## Optional Blueprint Additions

### 1. Animation Montage Notify
You can add an **AnimNotify** to your attack animations to explicitly update the direction at a specific frame:

**Location**: In your attack animation montages, add a notify called at the frame where the attack should "lock in" its direction.

**Blueprint Function to Call**:
```
Combat Component ? Set Input Direction ? [Last Recorded Input]
```

This gives you frame-precise control over when the direction is sampled during the attack animation.

### 2. Configurable Rotation Tuning
If you want to add adjustable parameters in the editor, add these to `CombatComponent.h`:

```cpp
UPROPERTY(EditAnywhere, Category = "Combat|Rotation", meta = (ClampMin = "0.0", ClampMax = "100.0"))
float InputDirectionThreshold{ 10.0f };  // Minimum velocity magnitude to register input

UPROPERTY(EditAnywhere, Category = "Combat|Rotation", meta = (ClampMin = "0.0", ClampMax = "360.0"))
float RotationSmoothTime{ 0.1f };  // Time to smoothly interpolate rotation (optional)

UPROPERTY(EditAnywhere, Category = "Combat|Rotation")
bool bUseSmoothedRotation{ false };  // Toggle between instant and smoothed rotation
```

Then update the rotation logic to use these:

```cpp
else if (!LastInputDirection.IsNearlyZero())
{
    FRotator NewRotation = UKismetMathLibrary::MakeRotFromX(LastInputDirection);
    FRotator CurrentRotation = CharacterRef->GetActorRotation();

    if (bUseSmoothedRotation)
    {
        // Smoothly interpolate toward the input direction
        FRotator SmoothedRotation = FMath::RInterpTo(
            CurrentRotation,
            FRotator(0.0f, NewRotation.Yaw, 0.0f),
            1.0f,  // Delta time (1.0f for immediate in this context)
            1.0f / RotationSmoothTime
        );
        CharacterRef->SetActorRotation(SmoothedRotation);
    }
    else
    {
        CharacterRef->SetActorRotation(FRotator(0.0f, NewRotation.Yaw, 0.0f));
    }
}
```

### 3. Visual Debug in Editor
Add a Blueprint node to visualize the input direction during combat:

```cpp
UFUNCTION(BlueprintCallable)
void DebugDrawInputDirection(bool bEnabled, float DrawDuration = 0.1f)
{
    if (!bEnabled || CharacterRef == nullptr) return;

    FVector Start = CharacterRef->GetActorLocation();
    FVector End = Start + (LastInputDirection * 100.0f);

    DrawDebugLine(
        CharacterRef->GetWorld(),
        Start,
        End,
        FColor::Green,
        false,
        DrawDuration,
        0,
        2.0f
    );
}
```

### 4. Input Direction State Getter (Blueprint)
Add a way to query the current input state from Blueprint:

```cpp
UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Combat|Rotation")
FVector GetCurrentInputDirection() const { return LastInputDirection; }

UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Combat|Rotation")
bool HasInputDirection() const { return !LastInputDirection.IsNearlyZero(); }
```

## Tweaking in Editor

After building, you can adjust the threshold in `MainCharacter.cpp`:

```cpp
if (CurrentVelocity.SizeSquared() > 100.0f)  // Adjust this value (default: 100.0)
{
    // Higher value = needs more input to trigger direction change
    // Lower value = more responsive to small inputs
}
```

**Recommended values:**
- `50.0f` - Very responsive, picks up small movements
- `100.0f` - Default, balanced responsiveness  
- `200.0f` - Only large stick inputs register
- `500.0f` - Very strict, only full-movement inputs

## Testing Checklist

- [ ] Lock-on attacks face the target correctly
- [ ] Without lock-on, standing still: combo stays in one direction
- [ ] Without lock-on, moving forward: combo rotates to face forward
- [ ] Without lock-on, moving in different directions: combo chains rotate smoothly
- [ ] Rapid direction changes between attacks work as expected
- [ ] No stuttering or jerky rotations
- [ ] Works with both gamepad and keyboard input
- [ ] Works in both side-scroller and 3D camera modes

## Troubleshooting

**Issue**: Character not rotating with input
- **Solution**: Verify `SetInputDirection()` is being called in Tick(). Check the velocity threshold isn't too high.

**Issue**: Character rotating too aggressively
- **Solution**: Increase the velocity threshold in Tick() or add smoothed rotation using `FMath::RInterpTo()`.

**Issue**: Conflicts with lock-on system
- **Solution**: The code prioritizes lock-on. Make sure to call `EndLockOn()` before testing non-locked combat.

**Issue**: Rotation feels "sticky" or delayed
- **Solution**: The input direction is updated every frame. If you need more immediate response, you can add Blueprint input handling directly instead of relying on velocity.

## Alternative: Direct Input Handling (Advanced)

If you want more direct control over input direction (e.g., from gamepad), you can also set it directly in Blueprint:

```
Event: Input Action "Attack"
  ? Get Controller's Input Axis "MoveForward" and "MoveRight"
  ? Calculate direction from both axes
  ? Call Combat Component ? Set Input Direction
  ? Then call Combat Component ? Try Combo Attack
```

This gives you frame-perfect input direction instead of relying on velocity.
