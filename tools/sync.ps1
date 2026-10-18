# Copies the mod's own files between this repo and a Dungeons Mod Kit checkout.
#   .\tools\sync.ps1 -FromModKit   # after editing in Unreal: pull changes into the repo
#   .\tools\sync.ps1 -ToModKit     # on a fresh Mod Kit clone: install the mod's files into it
# Only AnalogMove's own paths are listed, so this never touches ThirdPersonCam's files
# when both mods share one Mod Kit.
param(
    [switch]$FromModKit,
    [switch]$ToModKit,
    [string]$ModKit = (Join-Path $PSScriptRoot '..\..\Dungeons-Mod-Kit')
)

if ($FromModKit -eq $ToModKit) { throw 'Pass exactly one of -FromModKit or -ToModKit.' }

$repo = Join-Path $PSScriptRoot '..\ModKit'
$paths = @(
    'UE4Project\Content\Mods\AnalogMove',
    'UE4Project\Content\BPLoader\InGame\AM_Ingame.umap',
    'UE4Project\Content\BPLoader\Lobby\AM_Lobby.umap',
    'UE4Project\Source\Dungeons\CharacterAnimInstance.h'
)

foreach ($p in $paths) {
    $src = if ($FromModKit) { Join-Path $ModKit $p } else { Join-Path $repo $p }
    $dst = if ($FromModKit) { Join-Path $repo $p } else { Join-Path $ModKit $p }
    if (-not (Test-Path $src)) { Write-Warning "Missing: $src"; continue }
    if ((Get-Item $src).PSIsContainer) {
        New-Item -ItemType Directory -Force $dst | Out-Null
        Copy-Item (Join-Path $src '*') $dst -Recurse -Force
    } else {
        New-Item -ItemType Directory -Force (Split-Path $dst) | Out-Null
        Copy-Item $src $dst -Force
    }
    Write-Host "Copied $p"
}
