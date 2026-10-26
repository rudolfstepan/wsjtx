$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$mingw = if ($env:WSJTX_MINGW_PREFIX) { $env:WSJTX_MINGW_PREFIX } else { 'C:\msys64\mingw64' }
$oldPath = $env:PATH
try {
    # The CAT test links Hamlib; a clean runner has no installed Hamlib DLLs.
    $hamlibBin = Join-Path $root '.tools\hamlib\hamlib-w64-4.7.2\bin'
    $env:PATH = "$mingw\bin;$hamlibBin;$env:PATH"
    & "$mingw\bin\cmake.exe" --build "$root\build-custom-make" --target wsjtx wsjtx_pluto_bridge pluto_bridge_checks --parallel 6
    if ($LASTEXITCODE) { throw 'Pluto bridge test build failed.' }
    & "$root\build-custom-make\pluto_bridge_checks.exe"
    if ($LASTEXITCODE) { throw "Pluto bridge checks failed (exit $LASTEXITCODE)." }
    & python "$root\tools\Test-PlutoBridgeProtocol.py" "$root\build-custom-make\wsjtx_pluto_bridge.exe"
    if ($LASTEXITCODE) { throw 'Pluto bridge protocol lifecycle tests failed.' }
} finally { $env:PATH = $oldPath }
