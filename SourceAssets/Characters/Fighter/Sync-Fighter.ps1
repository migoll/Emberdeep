param(
    [switch]$CheckOnly
)

$ErrorActionPreference = "Stop"
$generator = Join-Path $PSScriptRoot "generate_fighter.py"
$animationBridge = Join-Path $PSScriptRoot "fighter_animation_bridge.py"
$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..\..")).Path
$equipmentGenerator = Join-Path $projectRoot "SourceAssets\Equipment\generate_equipment.py"
$projectFile = Join-Path $projectRoot "Emberdeep.uproject"

$pythonCommand = $null
$pythonPrefix = @()
$pythonLauncher = Get-Command "py" -ErrorAction SilentlyContinue
if ($pythonLauncher) {
    $pythonCommand = $pythonLauncher.Source
    $pythonPrefix = @("-3")
}
if (-not $pythonCommand) {
    $pythonExecutable = Get-Command "python" -ErrorAction SilentlyContinue
    if ($pythonExecutable) {
        $pythonCommand = $pythonExecutable.Source
    }
}
if (-not $pythonCommand) {
    $bundledPython = Join-Path $env:USERPROFILE ".cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe"
    if (Test-Path -LiteralPath $bundledPython) {
        $pythonCommand = $bundledPython
    }
}
if (-not $pythonCommand) {
    throw "Python blev ikke fundet. Aabn Codex en gang eller installer Python 3."
}

$generatorArguments = @($generator)
if ($CheckOnly) {
    $generatorArguments += "--check"
}
& $pythonCommand @pythonPrefix @generatorArguments
if ($LASTEXITCODE -ne 0) {
    throw "Fighter-konverteringen fejlede. Ret den viste Blockbench-gridfejl og proev igen."
}

$animationArguments = @($animationBridge)
if ($CheckOnly) {
    $animationArguments += "--check"
}
& $pythonCommand @pythonPrefix @animationArguments
if ($LASTEXITCODE -ne 0) {
    throw "Fighter-animationerne kunne ikke laeses. Brug almindelige numeriske keyframes i Blockbench og proev igen."
}

$equipmentArguments = @($equipmentGenerator)
if ($CheckOnly) {
    $equipmentArguments += "--check"
}
& $pythonCommand @pythonPrefix @equipmentArguments
if ($LASTEXITCODE -ne 0) {
    throw "Equipment-konverteringen fejlede. Kontroller de separate Blockbench-assets og proev igen."
}

if ($CheckOnly) {
    Write-Host "Fighterens Blockbench-master og genererede filer er synkroniserede."
    exit 0
}

if (Get-Process -Name "UnrealEditor" -ErrorAction SilentlyContinue) {
    throw "Luk Emberdeep/Unreal-vinduet og koer filen igen, saa den nye model kan kompileres."
}

$buildScript = "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat"
if (-not (Test-Path -LiteralPath $buildScript)) {
    throw "Unreal Engine 5.8 blev ikke fundet paa den forventede placering: $buildScript"
}

& $buildScript "EmberdeepEditor" "Win64" "Development" $projectFile "-WaitMutex" "-NoHotReloadFromIDE"
if ($LASTEXITCODE -ne 0) {
    throw "Unreal-builden fejlede. Se build-outputtet ovenfor."
}

Write-Host "Fighteren er synkroniseret og bygget. Den er klar til at blive aabnet i Emberdeep."
