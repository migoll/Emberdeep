@echo off
setlocal
set "BLOCKBENCH=%LOCALAPPDATA%\Programs\Blockbench\Blockbench.exe"

if not exist "%BLOCKBENCH%" (
    echo Blockbench blev ikke fundet: %BLOCKBENCH%
    pause
    exit /b 1
)

start "" "%BLOCKBENCH%" "%~dp0Weapons\notched_iron_sword.bbmodel"
start "" "%BLOCKBENCH%" "%~dp0Weapons\notched_iron_axe.bbmodel"
start "" "%BLOCKBENCH%" "%~dp0Weapons\bonecarver_axe.bbmodel"
start "" "%BLOCKBENCH%" "%~dp0Weapons\warden_cleaver.bbmodel"
start "" "%BLOCKBENCH%" "%~dp0Weapons\emberdeep_oath.bbmodel"
start "" "%BLOCKBENCH%" "%~dp0OffHands\fighter_shield_v0.bbmodel"
