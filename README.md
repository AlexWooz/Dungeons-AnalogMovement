# Analog Movement for Minecraft Dungeons

A Blueprint mod for **Minecraft Dungeons** (Steam, v1.17.0.0, UE 4.22) that makes the hero's speed follow how far you tilt the left stick: half tilt means half speed. At low speed the game's own walk animation plays and blends into the run as you tilt further.

**Status:** feature-complete and tested in camp and missions: analog speed, the game's walk animation at low speed, and stride-matched footsteps. See [docs/TECHNICAL.md](docs/TECHNICAL.md).

## Install
1. Install Dungeons Blueprint Loader 1.0 (`Blueprint-Loader.pak`) in your `~mods` folder.
2. Put `AnalogMove.pak` in `<Steam>\steamapps\common\MinecraftDungeons\Dungeons\Content\Paks\~mods\`.

To turn the mod off, remove the .pak. There is no in-game toggle. **F8** shows or hides a debug readout.

It is independent of [ThirdPersonCam](../Dungeons-ThirdPersonCam), and the two can be installed together.

## Building
It uses the same setup as ThirdPersonCam: Dokucraft's Dungeons Mod Kit at `..\Dungeons-Mod-Kit`, UE 4.22, and VS 2019 Build Tools for the editor-only stub.
1. `.\tools\sync.ps1 -ToModKit` copies this mod's files into the Mod Kit. It touches only AnalogMove paths.
2. Compile `DungeonsEditor` (see docs/PROBE.md step 0) and open the editor with `open_editor.bat`.
3. After editing, close the editor, run `cook_assets.bat`, then `.\tools\package.ps1`. That packs only this mod's assets. `package.bat` would also pack ThirdPersonCam.
4. Run `.\tools\sync.ps1 -FromModKit` before committing.

### Layout
| Path | Contents |
|---|---|
| `ModKit/UE4Project/Content/Mods/AnalogMove/` | `BP_AnalogMove` (all logic), `W_AMDebug` (F8 readout) |
| `ModKit/UE4Project/Content/BPLoader/{InGame,Lobby}/AM_*.umap` | Loader levels that spawn the actor |
| `ModKit/UE4Project/Source/Dungeons/CharacterAnimInstance.h` | Editor-only stub of the AnimBP's native parent, exposing `StateMachineConditions` (`RelativeMoveSpeed`, `LocomotionWalkVariedPlayRate`) |
| `tools/sync.ps1`, `tools/package.ps1` | Copy files to and from the Mod Kit; pack this mod alone |
