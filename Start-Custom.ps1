param([string]$Executable = "$PSScriptRoot\dist-custom\OE3SRO-reist16-pluto-timing\bin\wsjtx.exe")
$ErrorActionPreference = 'Stop'
if (!(Test-Path -LiteralPath $Executable)) { throw "Custom build not found: $Executable" }
if (!(Test-Path -LiteralPath "$env:LOCALAPPDATA\WSJT-X - Custom\WSJT-X - Custom.ini")) {
    throw 'Run tools\Copy-InstalledConfiguration.ps1 before first launch.'
}
# This user-invoked launcher opens the interactive radio application.
Start-Process -FilePath $Executable -ArgumentList '--rig-name', 'Custom' -WorkingDirectory (Split-Path $Executable) -WindowStyle Normal
