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

## Open questions
- **Foot sliding:** tune `GlobalAnimRateScale` per speed with the speed lock (Backslash) and `RateMul` ([ / ]), then fit a curve (plan Phase 2b).
- Speed boosts (potions, Swiftfooted): does `BaseSpeed` follow them? Not tested yet.
- Behaviour during rolls, attacks, and in missions (only camp has been tested).
