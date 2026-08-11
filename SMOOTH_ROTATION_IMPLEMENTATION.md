# Smooth Rotation Implementation - Complete Guide

## Overview

Added smooth rotation interpolation to combat attacks when not locked on. Instead of snapping to the input direction instantly, the character now smoothly rotates to face new directions.

## What Changed

### CombatComponent.h
Added two public properties to control smooth rotation behavior:

```cpp
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Rotation")
bool bUseSmoothedRotation{ true };

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Rotation", 
          meta = (ClampMin = "0.1", ClampMax = "1.0", EditCondition = "bUseSmoothedRotation"))
float RotationInterpSpeed{ 10.0f };
```

And added a new function:
```cpp
UFUNCTION(BlueprintCallable)
void ApplySmoothRotationTowardsInput(float DeltaTime);
```

### CombatComponent.cpp

**1. New `ApplySmoothRotationTowardsInput()` function:**
- Uses `FMath::RInterpTo()` to smoothly interpolate rotation
- Only applies when not locked on
- Respects the `bUseSmoothedRotation` and `RotationInterpSpeed` settings

**2. Updated `TickComponent()`:**
- Now calls `ApplySmoothRotationTowardsInput()` during attacks
- Smooth rotation happens every frame while attacking

**3. Simplified `ComboAttack()` and `HeavyAttack()`:**
- Removed instant rotation logic
- Lock-on still takes priority
- Smooth rotation is handled automatically via Tick

## How It Works

### Flow
```
During Attack Animation:
  ?
TickComponent() runs every frame
  ?
IsAttacking() returns true?
  ?
Call ApplySmoothRotationTowardsInput(DeltaTime)
  ?
Check if smoothed rotation is enabled
  ?
Interpolate from current rotation to target rotation
  ?
Apply the smoothly interpolated rotation
```

### Rotation Calculation
```cpp
// Get target rotation from input direction
FRotator TargetRotation = MakeRotFromX(LastInputDirection);

// Smoothly interpolate towards target
FRotator Result = RInterpTo(CurrentRotation, TargetRotation, DeltaTime, Speed);

// Apply the smoothly interpolated rotation
Character->SetActorRotation(Result);
```

## Configuration in Editor

After building, you'll see two new settings in the CombatComponent:

### `bUseSmoothedRotation` (Default: true)
- **true**: Smooth rotation via interpolation (new behavior)
- **false**: Instant rotation (original snap behavior)

### `RotationInterpSpeed` (Default: 10.0f, Range: 0.1 - 1.0)
This controls how **smoothly** (slowly) the character rotates:

- **0.1** = Very slow, sluggish rotation (like heavy armor)
- **2.0** = Moderate rotation (balanced)
- **5.0** = Fast rotation (quick response)
- **10.0** = Very fast rotation (almost instant, but still smooth)

**How to adjust:**
- Want snappier response? ? Increase the value
- Want slower, more theatrical rotation? ? Decrease the value

## Behavior

### With Smooth Rotation Enabled (Default)
```
Player presses D (right input)
  ?
Character smoothly rotates right over ~0.1 seconds
  ?
Next input can be processed before rotation completes
  ?
Creates fluid, responsive combat
```

### With Smooth Rotation Disabled
```
Player presses D
  ?
Character instantly faces right (original behavior)
  ?
May feel less polished but more responsive
```

## Testing

1. **Build the project** ?
2. **Open CombatComponent in editor**
3. **Try different RotationInterpSpeed values:**
   - 1.0: Very slow
   - 5.0: Medium
   - 10.0: Fast
   - 20.0: Very fast (near-instant but smooth)

4. **Test in gameplay:**
   - Move stick in different directions during combo
   - Watch the smooth rotation between attacks
   - Verify lock-on still works (higher priority)
   - Try with `bUseSmoothedRotation = false` to compare

## Blueprint Usage

You can toggle smooth rotation from Blueprint:

```
Event: Some Input
  ?
Get Combat Component
  ?
Set bUseSmoothedRotation ? [false]  (disable smooth rotation)
```

Or adjust speed:
```
Get Combat Component
  ?
Set RotationInterpSpeed ? [20.0]  (faster rotation)
```

## Technical Details

### FMath::RInterpTo()
The function uses Unreal's built-in `RInterpTo` which:
- Handles angle wrapping (0-360 degrees correctly)
- Provides smooth interpolation over time
- Stops interpolating when target is reached
- Uses DeltaTime for frame-rate independent movement

### When Smooth Rotation Occurs
Smooth rotation only happens:
- When NOT locked on to a target
- When an attack is in progress (`IsAttacking()` returns true)
- Every frame during the attack animation

### Lock-On Priority
Lock-on still has priority - if you lock on during an attack:
1. `ComboAttack()` calls `LockOnComp->FaceCurrentTargetForOneFrame()`
2. This overrides the smooth rotation for that frame
3. You face the target instantly
4. System returns to smooth rotation next attack if still attacking

## Performance Impact

**Very minimal:**
- Only 1 rotation interpolation per frame during attacks
- Uses simple math (RInterpTo is highly optimized)
- No allocations or expensive operations
- Negligible CPU/memory overhead

## Common Adjustments

### Problem: Rotation feels too "snappy"
**Solution:** Decrease `RotationInterpSpeed`
- Try 5.0 or even lower
- More gradual feeling

### Problem: Rotation feels too "sluggish"
**Solution:** Increase `RotationInterpSpeed`
- Try 15.0 or 20.0
- Snappier response

### Problem: Want instant rotation (like original)
**Solution:** Set `bUseSmoothedRotation` to false
- Character will rotate instantly again

### Problem: Rotation looks jittery
**Cause:** RotationInterpSpeed is too high relative to DeltaTime
**Solution:** Decrease RotationInterpSpeed or increase frequency of input updates

## Next Steps

1. **Adjust RotationInterpSpeed** to match your game's feel
2. **Playtest** with various values to find the sweet spot
3. **Consider** saving a preset configuration (e.g., "Combat Rotation - Fast" vs "Combat Rotation - Slow")
4. **Test** with other features (rolling, special attacks, etc.)

## Related Settings

These other systems interact with smooth rotation:
- **Lock-On System** - Takes priority over smooth rotation
- **Character Movement** - Velocity affects input direction updates
- **Animation Montages** - Duration affects how much rotation completes
- **Camera Position** - Influences perceived rotation speed

## Rollback

To disable this feature completely:
- Set `bUseSmoothedRotation = false` in editor
- Or delete the `ApplySmoothRotationTowardsInput()` calls from `TickComponent()`

## Files Modified

- `Source/Unreal/Public/Combat/CombatComponent.h` - Added settings and function
- `Source/Unreal/Private/Combat/CombatComponent.cpp` - Implemented smooth rotation logic
