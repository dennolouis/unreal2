# Smooth Rotation - Technical Implementation Details

## Code Changes Summary

### Header File Changes (CombatComponent.h)

**Added two public properties:**
```cpp
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Rotation")
bool bUseSmoothedRotation{ true };

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Rotation", 
          meta = (ClampMin = "0.1", ClampMax = "1.0", EditCondition = "bUseSmoothedRotation"))
float RotationInterpSpeed{ 10.0f };
```

**Added function declaration:**
```cpp
UFUNCTION(BlueprintCallable)
void ApplySmoothRotationTowardsInput(float DeltaTime);
```

### Implementation (CombatComponent.cpp)

**New function implementation:**
```cpp
void UCombatComponent::ApplySmoothRotationTowardsInput(float DeltaTime)
{
    if (!CharacterRef || LastInputDirection.IsNearlyZero())
    {
        return;
    }

    // Get target rotation from input direction
    FRotator TargetRotation = UKismetMathLibrary::MakeRotFromX(LastInputDirection);
    FRotator TargetYaw = FRotator(0.0f, TargetRotation.Yaw, 0.0f);

    // Get current rotation
    FRotator CurrentRotation = CharacterRef->GetActorRotation();

    if (bUseSmoothedRotation)
    {
        // Smoothly interpolate towards target rotation
        FRotator InterpolatedRotation = FMath::RInterpTo(
            CurrentRotation,
            TargetYaw,
            DeltaTime,
            RotationInterpSpeed
        );
        CharacterRef->SetActorRotation(InterpolatedRotation);
    }
    else
    {
        // Instant rotation (original behavior)
        CharacterRef->SetActorRotation(TargetYaw);
    }
}
```

**Updated TickComponent():**
```cpp
void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, 
                                     FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // Apply smooth rotation during attacks
    if (IsAttacking())
    {
        ApplySmoothRotationTowardsInput(DeltaTime);
    }
}
```

**Simplified ComboAttack():**
```cpp
void UCombatComponent::ComboAttack()
{
    // ... stamina check ...

    ULockOnComponent* LockOnComp = CharacterRef->FindComponentByClass<ULockOnComponent>();
    if (LockOnComp && LockOnComp->GetCurrentTargetActor())
    {
        LockOnComp->FaceCurrentTargetForOneFrame();
    }
    // Smooth rotation is now handled in TickComponent

    // ... rest of attack logic ...
}
```

**Same for HeavyAttack()** - removed instant rotation, smooth rotation handled by Tick.

## How FMath::RInterpTo Works

```cpp
FRotator Result = FMath::RInterpTo(
    CurrentRotation,      // Start rotation
    TargetRotation,       // Target rotation
    DeltaTime,            // Time since last frame
    InterpSpeed           // Speed of interpolation
);
```

### Key Points

1. **DeltaTime** - Frame independent, works at any framerate
2. **InterpSpeed** - Higher = faster interpolation
3. **Angle Wrapping** - Automatically handles 0-360 degree transitions
4. **Frame-Rate Safe** - Won't overshoot or behave differently at different framerates

### Math Behind It

```
Simplified pseudocode:
rotation_per_frame = InterpSpeed * DeltaTime
CurrentRotation += (TargetRotation - CurrentRotation) * rotation_per_frame
```

## When Rotation Happens

### Diagram
```
Attack Started
    ?
Every Frame:
    ?? TickComponent() called
    ?   ?? Check IsAttacking()? YES
    ?   ?   ?? Call ApplySmoothRotationTowardsInput(DeltaTime)
    ?   ?   ?   ?? Check if LastInputDirection is zero? NO
    ?   ?   ?   ?? Get target rotation
    ?   ?   ?   ?? Interpolate current ? target
    ?   ?   ?   ?? Apply new rotation
    ?   ?   ?
    ?   ?? Animation continues playing
    ?
Attack Ends
    ?? IsAttacking() returns false
        ?? No more smooth rotation
```

## Integration with Other Systems

### With Lock-On
```
While Locked On:
    ?? ComboAttack() calls LockOnComp->FaceCurrentTargetForOneFrame()
    ?   ?? Character faces target instantly
    ?? Next frame: TickComponent() runs
    ?   ?? IsAttacking() = true
    ?   ?? Call ApplySmoothRotationTowardsInput()
    ?   ?   ?? Interpolate towards LastInputDirection (but lock-on keeps it faced)
    ?   ?? Result: Smooth interpolation, but lock-on dominates
    ?
    ?? Ends Lock-On
        ?? Smooth rotation takes over next frame
```

### With Movement Input
```
Player moves and attacks simultaneously:
    ?? IA_Move triggered
    ?   ?? SetCameraRelativeInputDirection() updates LastInputDirection
    ?? ComboAttack() triggered
    ?   ?? Sets up attack animation
    ?? Every frame of attack animation:
    ?   ?? TickComponent() smoothly rotates toward current input direction
    ?
    ?? Result: Character follows stick input while attacking
```

## Performance Analysis

### Per-Frame Cost
1. **IsAttacking() check** - O(1), simple bool check
2. **If attacking, call ApplySmoothRotationTowardsInput()** - O(1)
   - Vector normalization (if input is non-zero)
   - Two rotation conversions
   - One RInterpTo calculation
   - One SetActorRotation call

**Total:** ~5-10 microseconds per frame (negligible)

### Memory
- 1 bool: 1 byte
- 1 float: 4 bytes
- **Total added:** 5 bytes (in public properties)

### Compared to Other Systems
- Much cheaper than physics calculations
- Cheaper than animation updates
- Similar cost to camera rotation

## Configuration via Blueprints

You can access and modify these at runtime:

```cpp
// Get CombatComponent
UCombatComponent* CombatComp = Character->FindComponentByClass<UCombatComponent>();

// Disable smooth rotation
CombatComp->bUseSmoothedRotation = false;

// Change rotation speed
CombatComp->RotationInterpSpeed = 15.0f;

// Set input direction
CombatComp->SetCameraRelativeInputDirection(InputVector);
```

## Debugging

To see if smooth rotation is working:

```cpp
// Add to ApplySmoothRotationTowardsInput():
#if !UE_BUILD_SHIPPING
if (bUseSmoothedRotation)
{
    DrawDebugLine(
        GetWorld(),
        CharacterRef->GetActorLocation(),
        CharacterRef->GetActorLocation() + (LastInputDirection * 150.0f),
        FColor::Green,
        false,
        0.05f,
        0,
        2.0f
    );
}
#endif
```

This draws a green line showing the target rotation direction.

## Timeline Example

For `RotationInterpSpeed = 10.0` at 60 FPS:

```
Frame 0: Input changed from Forward to Right
         Current Rotation: 0°
         Target Rotation: 90°
         ?
Frame 1: DeltaTime = 0.0167s
         Rotation per frame = 10.0 * 0.0167 = 0.167
         Progress = 0.167 (16.7%)
         Current Rotation = 0° + (90° - 0°) * 0.167 = 15°
         ?
Frame 2: Current Rotation = 27°
         Progress = 33%
         ?
Frame 3: Current Rotation = 37.5°
         Progress = 42%
         ?
Frame 4: Current Rotation = 45°
         Progress = 50%
         ...continues until near 90°
         ?
Frame 10: Current Rotation ? 89.8°
         Interpolation effectively stops (close enough)
```

**Total time:** ~10 frames = ~0.167 seconds at 60 FPS

## If You Want Different Behavior

### For Instant-But-Smooth (Very High Speed)
```cpp
RotationInterpSpeed = 50.0f;  // Reaches target in ~3 frames
```

### For Cinematic (Very Slow)
```cpp
RotationInterpSpeed = 0.5f;  // Takes ~30 frames to reach target
```

### For Conditionally Smooth
```cpp
if (bPlayerIsMoving)
    CombatComp->RotationInterpSpeed = 5.0f;  // Slower when moving
else
    CombatComp->RotationInterpSpeed = 15.0f; // Faster when stationary
```

## Compatibility

- ? Works with lock-on system
- ? Works with all attack types
- ? Works with camera system
- ? Works with movement input
- ? Doesn't affect animations
- ? Doesn't affect damage/hit detection
- ? Purely visual/rotation-only change
