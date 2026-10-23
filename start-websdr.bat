@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Start-WebSDR.ps1" %*
if errorlevel 1 (
    pause
    exit /b 1
)
