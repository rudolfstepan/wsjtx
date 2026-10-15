$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$mingw = if ($env:WSJTX_MINGW_PREFIX) { $env:WSJTX_MINGW_PREFIX } else { 'C:\msys64\mingw64' }
$previousPath = $env:PATH
try {
    $env:PATH = "$mingw\bin;$env:PATH"
    $build = Join-Path $root 'build-custom\core-checks'
    & "$mingw\bin\cmake.exe" -S "$root/tests/core" -B $build -G Ninja "-DCMAKE_PREFIX_PATH=$mingw"
    if ($LASTEXITCODE) { throw 'Core test configuration failed.' }
    & "$mingw\bin\cmake.exe" --build $build
    if ($LASTEXITCODE) { throw 'Core test build failed.' }
    $results = Join-Path $build 'test-results.txt'
    & "$mingw\bin\ctest.exe" --test-dir $build --output-on-failure
    $testExit = $LASTEXITCODE
    if (Test-Path -LiteralPath $results) { Get-Content -LiteralPath $results }
    if ($testExit) { throw 'Core regression tests failed.' }
} finally {
    $env:PATH = $previousPath
}
