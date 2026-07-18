@echo off
setlocal
set "BLOCKBENCH=%LOCALAPPDATA%\Programs\Blockbench\Blockbench.exe"
set "MODEL=%~dp0fighter_v0.bbmodel"

if exist "%BLOCKBENCH%" (
    start "" "%BLOCKBENCH%" "%MODEL%"
) else (
    start "" "%MODEL%"
)
