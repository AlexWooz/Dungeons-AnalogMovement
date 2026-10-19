# Technical notes

The general modding constraints (Blueprint only, Blueprint Loader levels, no UE4SS, no `PrintString`) are the same as ThirdPersonCam's; see its `docs/TECHNICAL.md`.

## Hero animation (from FModel, `Actors/Characters/Player/Master`)
- **AnimBP:** `A_Player_Animation`, parent native `/Script/Dungeons.CharacterAnimInstance`. It casts its owner to `BP_PlayerCharacter`.
- **`Main State Machine`** states: Idle, Walk/Run, Falling, Downed/Dead (+ Start), Glide, GlideLand, Dive, InitiateDive, DiveLand, DiveRecover, and a `Center` conduit.
- **Walk/Run** plays the BlendSpace1D `Animations/Player_Walk_Run`, axis `WalkRun` (0–1.2):

  | Sample | WalkRun | RateScale |
  |---|---|---|
  | `Player_Master_Walk` | 0.3 | 2.0 |
  | `Player_Master_Run` | 1.2 | 1.0 |

  Each weapon type has its own `Walk`/`Run`/`Walk_Run` (Claymore, Glaive, Hammer, Katana, Scythe).
- **Native inputs** the graph reads (from the package name table; cooked JSON has no bytecode):
  - `RelativeMoveSpeed`, which most likely goes through an `FClamp` into the blendspace X;
  - `LocomotionWalkVariedPlayRate`;
  - `FallBackLocomotionBlend`.
- **Montage slots:** `FullBody`, `UpperBody`, `NoArms`. Legs are layered separately (`LayeredBoneBlend` on `J_L_Leg`/`J_R_Leg`), and arms likewise (`J_L_Arm`/`J_R_Arm`).
- **Hero movement component:** `CharMoveComp` (`PlayerCharacterMovementComponent`) on `BP_AlexCharacter` / `BP_PlayerCharacter`.

## The CharacterAnimInstance stub
This is the same trick as ThirdPersonCam's `BasePlayerController` stub. The import table shows the AnimBP's native inputs aren't direct members of the anim instance. They live in a struct:
- `/Script/Dungeons.CharacterAnimInstance.StateMachineConditions` (StructProperty)
- of type `/Script/Dungeons.AnimGraphStateConditions`, with `RelativeMoveSpeed` (float), `LocomotionWalkVariedPlayRate` (float) and `FallBackLocomotionBlend` (enum, type unknown, not stubbed).

`Source/Dungeons/CharacterAnimInstance.h` mirrors that layout. The first version declared the floats directly on the class; those reads silently returned 0 in-game because the names didn't resolve.

To read the values, use `Anim → StateMachineConditions → Break`. To write them, use `Set Members in AnimGraphStateConditions` on that getter, then `Set StateMachineConditions` with the Struct Out. The struct is resolved to the real type at runtime, so fields we don't stub are preserved.

## Probe findings (2026-10-18, camp)
- **Speed:** writing `CharMoveComp.MaxWalkSpeed` every tick works. Velocity equals the value written. The game resets it to the hero's base speed (700) every frame, so the probe re-detects the base each tick and re-writes it.
- **RelativeMoveSpeed** is the hero's *nominal* speed / 600 (1.167 at 700) whenever they're moving, and 0 at rest. It ignores the actual velocity, so the blendspace stays on the run. `LocomotionWalkVariedPlayRate` is about 2.7–2.8 while moving (it differs per session) and 0 at rest.
- **The game recomputes the struct every frame**, before our tick. Writing `RelativeMoveSpeed = Velocity / 600` works only if the hero's **mesh ticks after our actor**: `Char.Mesh → Add Tick Prerequisite Actor(self)`. With that, the AnimBP blends to the game's own walk animation. Our actor already ticks after `CharMoveComp` (`Add Tick Prerequisite Component(Move)`), so the movement uses last frame's write.
- **Stick reading:** `Get Input Analog Key State (Gamepad Left Thumbstick X/Y)` has a small deadzone the game doesn't have. The game reads the stick natively, as it does for the stick-dodge. At the slightest tilt Unreal reports 0 while the hero still moves. The probe keeps a `PadActive` flag (set by any stick input, cleared by Left Mouse Button with Consume Input off) and treats "pad active, stick 0" as the minimum speed. Both the speed scaling and the anim override are gated on `PadActive`.
- The game has no keyboard movement, only gamepad axes and mouse click-to-move.

## What the AnimBP does with the struct (decoded from ExecuteUbergraph bytecode)
I scanned the cooked `A_Player_Animation` event graph for package-index references to the imported properties:
```
BlendSpacePlayer.X        = StateMachineConditions.RelativeMoveSpeed
BlendSpacePlayer.PlayRate = FMax(StateMachineConditions.RelativeMoveSpeed, 1.0)
LayeredBoneBlend(legs).BlendWeights[] = FClamp(StateMachineConditions.LocomotionWalkVariedPlayRate, 0?, 1.0)
FallBackLocomotionBlend   → many state-machine transition conditions (byte/enum compares)
```
- **`LocomotionWalkVariedPlayRate` is not a play rate.** It drives the leg-layer blend weight: how much the legs follow the locomotion while the upper body plays an action. **Never override it.** Lowering it half-blends the legs out.
- The blendspace's play rate is never below 1, so the AnimBP can't slow the walk below its 2× RateScale. To slow it, we use `Mesh.GlobalAnimRateScale`, applied only while `PadActive`, moving (velocity > 10) and **no montage is playing**, so attacks, rolls and artifacts keep normal speed. It's reset to 1 on EndPlay.

## Stride matching
**v1.0** was tuned from the isometric camera at 20/35/50/75/100% of base speed. The matching rates were 0.4, 0.65, 0.75, 0.95 and 1.0, fitted as `GroundSpeed = MapRangeClamped(Vel, 180..720 -> 350..700)`. Close-ups with the third-person camera showed the slow end still stepping too fast.

**v1.1** was re-tuned in third-person close-up at the slow end. The measured AnimRate was 0.36 at speed 140, 0.594 at 245 and 0.80 at 350; 525 and 700 were unchanged. The implied ground speed (Velocity / AnimRate) bends at about 350: the walk dominates and barely rises (389 → 437), then the run blends in quickly (437 → 700). Two clamped ranges added together follow that bend without branching:
```
GroundSpeed = MapRangeClamped(Vel, 140..350 -> 389..437)
            + MapRangeClamped(Vel, 350..700 ->   0..263)
AnimRate    = Clamp(Vel / GroundSpeed, 0.3, 1.0) * RateMul   // RateMul = 1; predicts 0.36, 0.595, 0.80, 0.92, 1.0
```
It works from the actual velocity, so it stays correct with speed boosts. Above base speed it stays at 1, and the AnimBP's own `Max(RelativeMoveSpeed, 1)` play rate takes over.

### Tuning keys (only while the F8 readout is visible)
- **`]` / `[`**: `RateMul` ±0.05 (min 0.05), a trim applied after the clamp.
- **`\`**: cycles a speed lock through off / 0.2 / 0.35 / 0.5 / 0.75 / 1.0 of base speed. Push the stick fully and the mod holds that exact speed. The lock only applies while the readout is shown.
- Tuning procedure: at each lock, trim until a planted foot stays still on the ground, then note **AnimRate**. Measure each lock starting from RateMul 1.0, and refit the formula from the AnimRate values (ground speed = speed / AnimRate). Don't ship a non-1 RateMul.

## Weapon walk sets (v1.2)
Equipping a weapon from one of five families **swaps the hero's anim instance** to a child AnimBP class with its own blendspace. Other weapons, and unarmed, use the base class. These are the class names, read from `Anim → GetClass → GetClassDisplayName` (the readout's `AnimBP` line):

| Weapons | AnimBP class | Blendspace walk sample |
|---|---|---|
| Unarmed, swords, daggers, axes, maces, etc. | `A_Player_Animation_C` | `Player_Master_Walk` @ RateScale **2.0** |
| Claymore family | `A_Player_Animation_Claymore_C` | `…_Walk_Claymore` @ **1.0** |
| Spear / glaive family | `A_Player_Animation_Glaive_C` | `…_Walk_Glaive` @ **1.0** |
| Great hammer family | `A_Player_Animation_Hammer_C` | `…_Walk_Hammer` @ **1.0** |
| Katana family | `A_Player_Animation_Katana_C` | `…_Walk_Katana` @ **1.0** |
| Scythe family | `A_Player_Animation_Scythe_C` | `…_Walk_Scythe` @ **1.0** |

All walk sequences are 4.333 s and all runs 2.167 s, and every run sample is at RateScale 1.0.

- **The anim instance is re-fetched every tick** (`Cast To CharacterAnimInstance(Char.Mesh.GetAnimInstance)` → `Anim`, or None on failure). Caching it in `Bind` broke after a weapon change: we kept writing `RelativeMoveSpeed` into the discarded instance, so the new one ran its native 1.167, which showed up as a slowed run.
- **Weapon factor:** Unreal advances a blendspace by the weighted average of its samples' lengths. With a 1× walk sample, the weapon sets step up to twice as slowly the more walk is in the blend. Starting from that model (`1 + walk weight`), the factor was tuned in-game: at 75% speed it needed 1.5 instead of the model's 1.36, and at base speed it's exactly 1.
  ```
  WalkWeight   = MapRangeClamped(Vel, 180..350 -> 1..0.685)
               + MapRangeClamped(Vel, 350..525 -> 0..-0.185)
               + MapRangeClamped(Vel, 525..700 -> 0..-0.5)
  WeaponFactor = (ClassDisplayName != "A_Player_Animation_C" AND IsValid(Anim)) ? 1 + WalkWeight : 1
  AnimRate     = Clamp(Vel / GroundSpeed, 0.3, 1.0) * RateMul * WeaponFactor
  ```
  Weapon-set AnimRate at 20/35/50/75/100% speed: about 0.72, 1.12, 1.35, 1.38 and 1.0. Values above 1 are expected, since the native weapon walk plays slowly. Speed boosts above base speed get factor 1, which is the game's own timing.
- **Unarmed right arm:** the right arm stays fairly stiff while walking. The AnimBP layers both arms from a separate pose with fixed weights (`LayeredBoneBlend` on `J_L_Arm`/`J_R_Arm`, which the graph never sets at runtime), so this is most likely the game's own behaviour, just easier to see at walking speed. It hasn't been compared against the vanilla game yet. Changing it would mean replacing the AnimBP.

## Lifecycle
- **F8** toggles the debug readout (`W_AMDebug`), which is hidden by default. `SetText` only runs while it's shown.
- **EndPlay**: on a level change, the actor removes its widget (otherwise the camp's readout stays frozen on screen in the mission) and resets `MaxWalkSpeed` and `GlobalAnimRateScale`. On **Quit** (`EndPlayReason == Quit`), it does nothing: calling `RemoveFromParent` during shutdown crashed the game on exit.

## Verified (2026-10-18)
Camp and missions: speed boosts, rolls, attacks while moving, mouse click-to-move, F8 across level changes, and clean quits from camp and a mission.
