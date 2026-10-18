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
This is the same trick as ThirdPersonCam's `BasePlayerController` stub. `Source/Dungeons/CharacterAnimInstance.h` declares `UCharacterAnimInstance : UAnimInstance` with `RelativeMoveSpeed` and `LocomotionWalkVariedPlayRate` as `BlueprintReadWrite` floats. Cooked Blueprints reference `/Script/Dungeons.CharacterAnimInstance:<name>` and resolve it against the game's class at runtime. Both property types are inferred and need confirming in-game.

## Open questions (answered by the probe)
- Does writing `MaxWalkSpeed` every tick stick, or does the game override it?
- Is `RelativeMoveSpeed` velocity ÷ a fixed base speed (then walking is free), or ÷ the current max?
