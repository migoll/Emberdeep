[CmdletBinding()]
param(
    [string]$GodotExecutable
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

if ([string]::IsNullOrWhiteSpace($GodotExecutable)) {
    foreach ($commandName in @('godot', 'godot4')) {
        $godotCommand = Get-Command $commandName -CommandType Application -ErrorAction SilentlyContinue
        if ($null -ne $godotCommand) {
            $GodotExecutable = $godotCommand.Source
            break
        }
    }
}

if ([string]::IsNullOrWhiteSpace($GodotExecutable)) {
    $portableSearchRoots = @((Join-Path ([Environment]::GetFolderPath('UserProfile')) 'Downloads') )
    $ancestorDirectory = [System.IO.DirectoryInfo]::new($PSScriptRoot)
    while ($null -ne $ancestorDirectory) {
        $siblingDownloads = Join-Path $ancestorDirectory.FullName 'Downloads'
        if (Test-Path -LiteralPath $siblingDownloads -PathType Container) {
            $portableSearchRoots += $siblingDownloads
        }
        $ancestorDirectory = $ancestorDirectory.Parent
    }
    $portableSearchRoots = $portableSearchRoots | Select-Object -Unique
    $portableCandidates = foreach ($searchRoot in $portableSearchRoots) {
        if (Test-Path -LiteralPath $searchRoot -PathType Container) {
            Get-ChildItem -LiteralPath $searchRoot -Filter 'Godot_v*-stable_win64_console.exe' -File -Recurse -Depth 4 -ErrorAction SilentlyContinue
        }
    }
    $portableGodot = $portableCandidates |
        Sort-Object LastWriteTimeUtc -Descending |
        Select-Object -First 1
    if ($null -ne $portableGodot) {
        $GodotExecutable = $portableGodot.FullName
    }
}

if ([string]::IsNullOrWhiteSpace($GodotExecutable) -or -not (Test-Path -LiteralPath $GodotExecutable -PathType Leaf)) {
    throw 'Godot was not found. Pass -GodotExecutable, add godot/godot4 to PATH, or extract a stable Windows console build under Downloads.'
}

$godotLogPath = Join-Path $PSScriptRoot 'godot-export.log'
& $GodotExecutable --headless --log-file $godotLogPath --path $PSScriptRoot --script 'res://export_thorgrim.gd'
if ($LASTEXITCODE -ne 0) {
    throw "Godot Thorgrim export failed with exit code $LASTEXITCODE"
}

& (Join-Path $PSScriptRoot 'validate_glb.ps1')
if ($LASTEXITCODE -ne 0) {
    throw "Thorgrim GLB validation failed with exit code $LASTEXITCODE"
}
