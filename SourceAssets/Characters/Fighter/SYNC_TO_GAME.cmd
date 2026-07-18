@echo off
setlocal
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Sync-Fighter.ps1"
if errorlevel 1 (
    echo.
    echo Synkronisering fejlede. Laes beskeden ovenfor.
    pause
    exit /b 1
)
echo.
echo Faerdig. Du kan nu starte Emberdeep.
pause
