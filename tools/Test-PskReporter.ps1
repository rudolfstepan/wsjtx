$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$mingw = 'C:\msys64\mingw64'
$env:PATH = "$mingw\bin;$env:PATH"
$build = Join-Path $root 'build-custom\pskreporter-checks'
& "$mingw\bin\cmake.exe" -S "$root/tests/pskreporter" -B $build -G Ninja "-DCMAKE_PREFIX_PATH=$mingw"
if ($LASTEXITCODE) { throw 'Test configuration failed.' }
& "$mingw\bin\cmake.exe" --build $build
if ($LASTEXITCODE) { throw 'Test build failed.' }
$env:QT_QPA_PLATFORM = 'offscreen'
Push-Location $root
try {
    & "$build\pskreporter_checks.exe"
    if ($LASTEXITCODE) { throw 'PSK Reporter tests failed.' }
    if (Test-Path -LiteralPath "$build\about_preview.exe") {
        & "$build\about_preview.exe"
        if ($LASTEXITCODE) { throw 'About preview failed.' }
    }
} finally {
    Pop-Location
    Remove-Item Env:QT_QPA_PLATFORM
}
