$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$executable = [IO.Path]::GetFullPath((Join-Path $root 'dist-custom\bin\wsjtx.exe'))
$versionTool = [IO.Path]::GetFullPath((Join-Path $root 'dist-custom\bin\wsjtx_app_version.exe'))
$checks = Get-CimInstance Win32_Process -Filter "Name = 'wsjtx.exe' OR Name = 'wsjtx_app_version.exe'" | Where-Object {
    $_.ExecutablePath -in $executable, $versionTool -and $_.CommandLine -match '--version'
}
foreach ($check in $checks) {
    Stop-Process -Id $check.ProcessId
}
Write-Output "Stopped $(@($checks).Count) custom-build version-dialog checks."
