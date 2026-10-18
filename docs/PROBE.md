# Phase 0 probe: BP_AMProbe

> **Historical.** This is the original build guide for the discovery probe. The probe has since become `BP_AnalogMove`, and several steps changed after testing: the stub uses the `StateMachineConditions` struct, the hero mesh ticks after the actor, there's `PadActive`, and F8 now toggles the debug readout. `docs/TECHNICAL.md` describes the current design.

A throwaway actor that answers two questions in one play session:
1. Does writing `MaxWalkSpeed` every tick actually slow the hero, or does the game override it?
2. Does the native `RelativeMoveSpeed` drop along with the speed? If it does, the AnimBP blends to its own walk animation and we get a real walk cycle for free.

The probe already scales speed by stick tilt, so if both answers are yes, it is essentially Phase 1.

## 0. Prerequisites
- Close the Unreal editor.
- Run `.\tools\sync.ps1 -ToModKit` from this repo. It copies `CharacterAnimInstance.h` into the Mod Kit.
- Compile the editor module, as for ThirdPersonCam:
  `"C:\Program Files\Epic Games\UE_4.22\Engine\Build\BatchFiles\Build.bat" DungeonsEditor Win64 Development -Project="C:\Users\AlexWooz\Projects\Dungeons-Mod-Kit\UE4Project\Dungeons.uproject" -WaitMutex`
- Open the editor with `open_editor.bat`. **Cast To CharacterAnimInstance** should now be available in Blueprints.

## 1. Debug widget `W_AMDebug`
In `Content/Mods/AnalogMove/`, either duplicate `Mods/ThirdPersonCam/W_TPCDebug` and rename it, or create a Widget Blueprint:
- Canvas Panel → Text Block named **`DebugText`** with **Is Variable** checked
- Anchor top-left, position (40, 200), font size 14, shadow offset (1, 1)

## 2. Actor `BP_AMProbe` (parent: Actor)
Class settings: **Tick Group = Pre Physics** (the default) and **Start with Tick Enabled** on.

**Variables**
| Name | Type | Default |
|---|---|---|
| `PC` | Player Controller (ref) | |
| `Char` | Character (ref) | |
| `Move` | Character Movement Component (ref) | |
| `Anim` | Character Anim Instance (ref) | |
| `Dbg` | W_AMDebug (ref) | |
| `Enabled` | Boolean | true |
| `BaseSpeed` | Float | 0 |
| `LastWritten` | Float | -1 |
| `GameWrites` | Integer | 0 |
| `Deadzone` | Float | 0.15 |
| `MinScale` | Float | 0.2 |

**Event BeginPlay**
1. `Delay 0.5`
2. `PC = Get Player Controller (0)`, then `Enable Input (PC)`
3. `Dbg = Create Widget (W_AMDebug, Owning Player = PC)`, then `Add to Viewport`

**F8** (Keyboard F8 key event) → `Enabled = NOT Enabled`

**Function `Bind`** (call it from Tick when `Char` isn't valid or `Char != Get Player Pawn (0)`):
1. `Char = Cast To Character (Get Player Pawn (0))`. If the cast fails, return.
2. `Move = Char.Get Character Movement`
3. `Anim = Cast To CharacterAnimInstance (Char.Mesh → Get Anim Instance)`. A failed cast is fine; `Anim` just stays invalid.
4. `Add Tick Prerequisite Component` (Target = self, Prerequisite Component = `Move`), so this actor ticks before the movement component.
5. `LastWritten = -1`

**Event Tick** (return early if `PC` or `Move` isn't valid after `Bind`):
1. **Detect game writes:** `Raw = Move.Max Walk Speed`. If `NOT Nearly Equal (Raw, LastWritten, 0.01)`, set `BaseSpeed = Raw` and `GameWrites += 1`.
2. **Stick:**
   - `X = PC.Get Input Analog Key State (Gamepad Left Thumbstick X-Axis)`, and `Y` the same with the Y-Axis.
   - `RawMag = Clamp (Vector Length (Make Vector (X, Y, 0)), 0, 1)`
   - `Mag = Clamp ((RawMag − Deadzone) / (0.95 − Deadzone), 0, 1)`
   - Use **Get Input Analog Key State**, not axis events: it doesn't consume input, so the game's own `Move*Gamepad` axes still work.
3. **Target:** `Select Float`, where `Enabled AND RawMag > Deadzone` picks `BaseSpeed × Max (Mag, MinScale)` and otherwise `BaseSpeed`.
4. `Move.Max Walk Speed = Target`, then `LastWritten = Target`.
5. **Debug:** `Vel = Vector Length XY (Char.Get Velocity)`. Fill `Dbg.DebugText.Set Text` from one **Format Text** node:
   ```
   Enabled {E}   (F8)
   Stick {RM}  Mag {M}
   Base {B}  Wrote {T}  GameWrites {G}
   Velocity {V}  Vel/Base {VB}
   RelativeMoveSpeed {R}
   WalkVariedPlayRate {W}
   ```
   - `VB = V / BaseSpeed`. Guard against BaseSpeed = 0 with `Select Float`.
   - `R` and `W` come from `Anim` (if `Anim` isn't valid, show -1).

**Event End Play** → if `Move` is valid, `Move.Max Walk Speed = BaseSpeed`, so the next level starts clean.

## 3. Loader levels
Create `Content/BPLoader/InGame/AM_Ingame` and `Content/BPLoader/Lobby/AM_Lobby` (File → New Level → Empty Level). Each holds one `BP_AMProbe` at the origin, like TPC's levels.

## 4. Build & install
1. Close the editor and run the Mod Kit's `cook_assets.bat`.
2. Run `.\tools\package.ps1`, which writes `~mods\AnalogMove.pak` with only this mod's assets. Don't use `package.bat`: it packs ThirdPersonCam too.

## 5. What to record (camp first, then a mission)
At rest, then at ¼, ½ and full tilt, with F8 on and then off, note:
- **GameWrites:** does it stay small, or climb every frame? Climbing every frame means the game rewrites `MaxWalkSpeed` each tick. That's harmless as long as the speed still drops.
- **Velocity vs Wrote:** does the hero really slow down?
- **RelativeMoveSpeed:** compare full tilt with half tilt. Also watch the legs at about ⅓ tilt.

| Result | Meaning | Next step |
|---|---|---|
| Speed drops, RelativeMoveSpeed drops, legs walk | Best case | Phase 1 = this probe with settings and polish |
| Speed drops, RelativeMoveSpeed stays at ~1 | It's normalized by the current max | Write `Anim.RelativeMoveSpeed = Vel / BaseSpeed` after the native update (try the Post Update Work tick group), or fall back to Tier B |
| Speed doesn't drop | Native speed path ignores `MaxWalkSpeed` | Try `Max Acceleration`, or scaling `Custom Time Dilation` while moving |
| Cast to CharacterAnimInstance always fails, or the game crashes on load | Stub name or type mismatch | Remove the `Anim` reads and report the result |
