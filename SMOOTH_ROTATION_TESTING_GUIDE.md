# Testing Guide - Smooth Rotation Restrictions

## Quick Test Scenarios

### Scenario 1: Lock-On Combat
**Setup:**
- Get within lock-on range of an enemy
- Lock on to target (Y button / Right Stick Click)
- Start attacking

**Expected Behavior:**
- ? Character faces locked target
- ? **NO** smooth rotation from input direction
- ? Character stays aimed at target throughout combo
- ? Movement input doesn't change facing direction

**If Wrong:**
- ? Character rotates away from target = BUG
- ? Input direction overrides lock-on = BUG

---

### Scenario 2: Unlocked Combo (First Attacks)
**Setup:**
- Stand away from enemies (no lock-on)
- Start combo attack (LMB / Square)

**Expected Behavior:**
- ? First attack: Smooth rotation works (can aim with stick)
- ? Second attack: Smooth rotation works (can aim with stick)
- ? Character smoothly rotates to follow stick input

**If Wrong:**
- ? Character doesn't rotate = BUG
- ? Rotation snaps instead of smooth = BUG

---

### Scenario 3: Finisher Attack (Last Attack)
**Setup:**
- Stand away from enemies
- Chain through entire combo to finisher (3rd attack)
- Try to move stick during finisher

**Expected Behavior:**
- ? Character is **locked in place** (no rotation)
- ? **NO** smooth rotation occurs
- ? Facing direction is fixed (where attack started)
- ? Animation plays in committed direction

**If Wrong:**
- ? Character still rotates during finisher = BUG
- ? Smooth rotation applies = BUG

---

### Scenario 4: Finisher During Lock-On
**Setup:**
- Lock on to target
- Chain entire combo to finisher
- Try to move stick/camera

**Expected Behavior:**
- ? Character faces target (lock-on priority)
- ? **NO** smooth rotation from input
- ? Character stays aimed at target

**If Wrong:**
- ? Character rotates away from target = BUG

---

### Scenario 5: Switching Lock-On Mid-Combo
**Setup:**
- Start unlocked combo (not locked on)
- Smooth rotation is working
- Lock on to a nearby enemy mid-combo

**Expected Behavior:**
- ? Immediately stop smooth rotation
- ? Character snaps to face locked target
- ? Remaining attacks face the locked target
- ? Input no longer affects rotation

**If Wrong:**
- ? Rotation doesn't stop = BUG
- ? Still rotating to input after lock-on = BUG

---

### Scenario 6: Breaking Lock-On Mid-Combo
**Setup:**
- Start locked on to target
- Break lock-on (Y button again)
- Continue attacking

**Expected Behavior:**
- ? Smooth rotation resumes
- ? Character can be aimed with stick again
- ? Rotation is smooth (not instant)

**If Wrong:**
- ? Rotation doesn't resume = BUG
- ? Rotation snaps instead of smooth = BUG

---

## Combo Sequence Test

**Full Combo Expectations:**

```
Combo Hit 1 (Unlock-On):
  Input: Forward
  ?? Smooth Rotation: ? YES
  ?? Character Faces: Forward
  ?? Expected: Can aim with input

Combo Hit 2 (Unlock-On):
  Input: Right
  ?? Smooth Rotation: ? YES
  ?? Character Faces: Smoothly rotates right
  ?? Expected: Can aim with input

Combo Hit 3 (Finisher):
  Input: Left
  ?? Smooth Rotation: ? NO
  ?? Character Faces: Right (locked)
  ?? Expected: Locked in place, no rotation
              Finisher animation plays facing right
              even though stick is left
```

---

## Debugging Notes

### If Finisher Detection Fails

The finisher detection is based on combo array size:
```
ComboCounter ranges from 0 to (ArraySize - 1)
Finisher is when: ComboCounter - 1 >= (ArraySize - 1)
```

**Check:**
1. Open CombatComponent details in editor
2. Look at "AttackAnimations" array
3. Count the animations
4. Last one should be the finisher

**Example:**
- 3 animations = indices 0, 1, 2
- Finisher = when ComboCounter is 2 (3rd hit)

### If Lock-On Detection Fails

**Check:**
1. MainCharacter has LockOnComponent
2. LockOnComponent->CurrentTargetActor is set when locked on
3. GetCurrentTargetActor() returns valid actor

**Test:**
- Add debug line: `UE_LOG(LogTemp, Warning, TEXT("Locked On: %d"), IsLockedOn());`
- Watch output log during lock-on attacks

---

## Performance Check

**Should see no performance impact:**
- Frame rate stays consistent
- No stuttering during rotation
- No lag spikes

**If performance drops:**
- Check if IsLockedOn() is being called too frequently
- Profile with Unreal Profiler

---

## Before/After Comparison

### Before (Without Restrictions)
```
Lock-On Attack + Smooth Rotation:
  ?? Looks: Weird (rotating away from target)
  ?? Feel: Inconsistent

Finisher Attack:
  ?? Looks: Looks like regular attack (can rotate)
  ?? Feel: No commitment required (inconsistent)
```

### After (With Restrictions)
```
Lock-On Attack:
  ?? Looks: Clean (faces target only)
  ?? Feel: Consistent with lock-on mechanic

Finisher Attack:
  ?? Looks: Committed animation (no rotation)
  ?? Feel: Player must aim carefully (rewarding)
```

---

## Common Issues & Fixes

| Issue | Likely Cause | Fix |
|-------|-------------|-----|
| Finisher still rotates | IsFinisherAttack() broken | Check ComboCounter logic |
| Lock-on doesn't stop rotation | IsLockedOn() returns false | Check LockOnComponent exists |
| Rotation stops too early | IsAttacking() check wrong | Verify IsAttacking() logic |
| Performance drop | Function called too much | Already optimized, should be fine |

---

## Sign-Off Checklist

- [ ] Lock-on prevents smooth rotation
- [ ] Finisher prevents smooth rotation
- [ ] Regular attacks have smooth rotation
- [ ] No performance impact
- [ ] All animations play correctly
- [ ] Hit detection not affected
- [ ] Damage not affected
- [ ] No console errors

**Ready to Ship:** ? Once all checks pass
