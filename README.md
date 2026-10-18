# Analog Movement for Minecraft Dungeons

A Blueprint mod for **Minecraft Dungeons** (Steam, v1.17.0.0, UE 4.22) that makes the hero's speed follow how far you tilt the left stick: half tilt means half speed. The aim is for the game's own walk animation to play at low speed instead of a full run.

**Status:** Phase 0 (discovery probe). See [docs/PROBE.md](docs/PROBE.md) and [docs/TECHNICAL.md](docs/TECHNICAL.md).

It is independent of [ThirdPersonCam](../Dungeons-ThirdPersonCam), and the two can be installed together.

## Building
It uses the same setup as ThirdPersonCam: Dokucraft's Dungeons Mod Kit at `..\Dungeons-Mod-Kit`, UE 4.22, and VS 2019 Build Tools for the editor-only stub.
1. `.\tools\sync.ps1 -ToModKit` copies this mod's files into the Mod Kit. It touches only AnalogMove paths.
2. Compile `DungeonsEditor` (see PROBE.md step 0) and open the editor with `open_editor.bat`.
3. After editing, close the editor, run `cook_assets.bat`, then `.\tools\package.ps1`. That packs only this mod's assets. `package.bat` would also pack ThirdPersonCam.
4. Run `.\tools\sync.ps1 -FromModKit` before committing.

### Layout
| Path | Contents |
|---|---|
| `ModKit/UE4Project/Content/Mods/AnalogMove/` | `BP_AMProbe`, `W_AMDebug` |
| `ModKit/UE4Project/Content/BPLoader/{InGame,Lobby}/AM_*.umap` | Loader levels that spawn the actor |
| `ModKit/UE4Project/Source/Dungeons/CharacterAnimInstance.h` | Editor-only stub of the AnimBP's native parent, exposing `RelativeMoveSpeed` |
| `tools/sync.ps1`, `tools/package.ps1` | Copy files to and from the Mod Kit; pack this mod alone |
