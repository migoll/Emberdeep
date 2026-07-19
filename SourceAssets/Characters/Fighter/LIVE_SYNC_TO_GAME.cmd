@echo off
setlocal
set "PYTHON=%USERPROFILE%\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe"
if not exist "%PYTHON%" (
    where python >nul 2>nul || (
        echo Python was not found. Run SYNC_TO_GAME.cmd once through Codex first.
        pause
        exit /b 1
    )
    set "PYTHON=python"
)

start "Emberdeep Live Sync" powershell.exe -NoExit -Command "& '%PYTHON%' '%~dp0live_sync.py'"

tasklist /FI "IMAGENAME eq UnrealEditor.exe" 2>nul | find /I "UnrealEditor.exe" >nul
if errorlevel 1 (
    start "Emberdeep" "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "%~dp0..\..\..\Emberdeep.uproject" -game -windowed -ResX=1280 -ResY=720 -log
)

echo Live Sync started. Keep its PowerShell window open and save in Blockbench with Ctrl+Alt+S.
exit /b 0
