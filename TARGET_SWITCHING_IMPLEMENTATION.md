# Lock-On Component Target Switching Implementation Guide

## Overview
The `ULockOnComponent` has been enhanced to support target switching through multiple input methods:
- **Gamepad**: Right joystick (X-axis) left/right
- **Keyboard**: `Q` (previous) / `E` (next) or custom action keys
- **Mouse**: Keyboard keys when in lock-on mode

## Architecture

### Key Properties (Configurable in Blueprint)

| Property | Default | Description |
|----------|---------|-------------|
| `TargetSwitchRadius` | 1500.0f | Radius to search for nearby targets |
| `JoystickSwitchThreshold` | 0.5f | Input buffer threshold to trigger target switch |
| `AngleThreshold` | 45.0f | Angular range to consider targets as "next" |

### Core Methods

#### `FindNearbyTargets()`
- Searches for all valid enemies within `TargetSwitchRadius`
- Uses sphere sweep collision to find candidates
- Excludes the current target
- Stores results in `NearbyTargets` array

#### `FindNextTarget(float DirectionAngle)`
- Takes a direction angle (positive = clockwise/right, negative = counter-clockwise/left)
- Finds the closest target in the specified angular direction
- Only considers targets within `AngleThreshold`
- Returns the best matching target or `nullptr`

**Angle Calculation**:
- Calculates angular distance from the current target to candidates
- When searching right (positive angle): prefers targets on the right side
- When searching left (negative angle): prefers targets on the left side
- Uses `FMath::FindDeltaAngleDegrees()` for proper angle wrapping

#### `CycleNextTarget()`
- Switches to the next target in the clockwise direction
- Calls `FindNextTarget(45.0f)`
- Updates owner reference and broadcasts delegate

#### `CyclePreviousTarget()`
- Switches to the previous target in the counter-clockwise direction
- Calls `FindNextTarget(-45.0f)`
- Updates owner reference and broadcasts delegate

#### `SetupInput()`
- Called during `BeginPlay()`
- Binds input actions to target switching methods
- Binds joystick axis input to `OnTargetSwitchInput()`

**Input Bindings Required**:
- **Axis**: `RightStickX` (gamepad right joystick X-axis)
- **Action**: `TargetSwitchLeft` (keyboard/mouse - previous target)
- **Action**: `TargetSwitchRight` (keyboard/mouse - next target)

#### `OnTargetSwitchInput(float AxisValue)`
- Handles continuous joystick input
- Accumulates input in `JoystickInputBuffer`
- When buffer exceeds `JoystickSwitchThreshold`:
  - Positive value: calls `CycleNextTarget()`
  - Negative value: calls `CyclePreviousTarget()`
- Resets buffer after switching to prevent continuous rapid switching

### Input Setup Instructions

#### In Your Project's Input Settings:

**1. Add Gamepad Axis Mapping** (if not already present):
```
Project Settings > Input > Gamepad > Right Stick X-Axis
```

**2. Add Action Mappings**:
```
Project Settings > Input > Actions

TargetSwitchLeft:
  - Keyboard: Q
  - Gamepad: None (handled by axis)

TargetSwitchRight:
  - Keyboard: E
  - Gamepad: None (handled by axis)
```

**Optional - For Mouse Support**:
```
TargetSwitchLeft:
  - Mouse: Mouse X (negative axis)

TargetSwitchRight:
  - Mouse: Mouse X (positive axis)
```

## Usage in Blueprint

Once in lock-on:

**Gamepad**:
- Push right joystick left to cycle to previous target
- Push right joystick right to cycle to next target

**Keyboard**:
- Press `Q` to cycle to previous target
- Press `E` to cycle to next target

**Programmatic**:
```cpp
// In C++
LockOnComponent->CycleNextTarget();
LockOnComponent->CyclePreviousTarget();

// In Blueprint
Call "Cycle Next Target"
Call "Cycle Previous Target"
```

## Behavior Details

### Target Selection Algorithm
1. Find all nearby enemies within `TargetSwitchRadius`
2. For each candidate:
   - Calculate angle from owner to current target
   - Calculate angle from owner to candidate
   - Compute angular difference in the requested direction
3. Select the candidate with the smallest angular difference
4. Only consider if within `AngleThreshold` degrees

### Example Scenario
With a player looking at Enemy A (straight ahead), and Enemy B to the right:
- **CycleNextTarget()**: Switches to Enemy B (clockwise direction)
- **CyclePreviousTarget()**: Switches to an enemy to the left if one exists

### Edge Cases Handled
- ? No nearby targets: target switch fails silently
- ? Only current target nearby: no switch occurs
- ? Invalid actors: automatically filtered out
- ? Actors not implementing Enemy interface: excluded
- ? Rapid input: buffered and debounced by `JoystickSwitchThreshold`

## Delegates and Callbacks

The component broadcasts the `OnUpdatedTargetDelegate` when the target changes:

```cpp
// In your character class
void OnTargetUpdated(AActor* NewTarget)
{
    if (NewTarget)
    {
        UE_LOG(LogTemp, Warning, TEXT("Switched to target: %s"), *NewTarget->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Lock-on ended"));
    }
}

// In BeginPlay
if (ULockOnComponent* LockOn = FindComponentByClass<ULockOnComponent>())
{
    LockOn->OnUpdatedTargetDelegate.AddDynamic(this, &AYourCharacter::OnTargetUpdated);
}
```

## Tuning Recommendations

### For Responsive Switching
- Lower `JoystickSwitchThreshold` (e.g., 0.3f)
- Increase `AngleThreshold` (e.g., 60.0f)
- Increase `TargetSwitchRadius` (e.g., 2000.0f)

### For Deliberate Switching
- Raise `JoystickSwitchThreshold` (e.g., 0.7f)
- Lower `AngleThreshold` (e.g., 30.0f)
- Decrease `TargetSwitchRadius` (e.g., 1000.0f)

### For Side-Scroller Games
The system works in 2D when `bIsSideScroller` is true:
- Only uses X and Z axes for angle calculations
- Y axis is ignored for targeting
- Joystick input still functions normally

## Performance Notes

- **FindNearbyTargets()** uses sphere sweep - called every frame
- Nearby target list is cached and updated in `TickComponent()`
- Target switching is O(n) where n = number of nearby targets
- Suitable for typical combat scenarios with 5-20 nearby enemies

## Future Enhancements

Potential improvements:
- Visual indicators for available targets
- Filtering by distance within `TargetSwitchRadius`
- Priority-based targeting (shield enemies, healers, etc.)
- Animation feedback on target switch
- Audio cues for successful/failed switches
- Customizable target cycling patterns (spiral, closest, etc.)
