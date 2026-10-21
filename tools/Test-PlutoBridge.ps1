$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$mingw = if ($env:WSJTX_MINGW_PREFIX) { $env:WSJTX_MINGW_PREFIX } else { 'C:\msys64\mingw64' }
$oldPath = $env:PATH
try {
    $env:PATH = "$mingw\bin;$env:PATH"
    & "$mingw\bin\cmake.exe" --build "$root\build-custom" --target wsjtx wsjtx_pluto_bridge pluto_bridge_checks --parallel 6
    if ($LASTEXITCODE) { throw 'Pluto bridge test build failed.' }
    & "$root\build-custom\pluto_bridge_checks.exe"
    if ($LASTEXITCODE) { throw 'Pluto bridge checks failed.' }
    & python "$root\tools\Test-PlutoBridgeProtocol.py" "$root\build-custom\wsjtx_pluto_bridge.exe"
    if ($LASTEXITCODE) { throw 'Pluto bridge protocol lifecycle tests failed.' }
} finally { $env:PATH = $oldPath }
