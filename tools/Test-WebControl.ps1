param([string]$BuildDirectory = '')
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$mingw = if ($env:WSJTX_MINGW_PREFIX) { $env:WSJTX_MINGW_PREFIX } else { 'C:\msys64\mingw64' }
$env:PATH = "$mingw\bin;$env:PATH"
$build = if ($BuildDirectory) { $BuildDirectory } else { Join-Path $root 'build-custom-make/webcontrol-checks' }
& cmake -S "$root/tests/webcontrol" -B $build -G "MinGW Makefiles" "-DCMAKE_PREFIX_PATH=$mingw" '-DCMAKE_BUILD_TYPE=Release'
if ($LASTEXITCODE) { throw 'Web control test configuration failed.' }
& cmake --build $build --parallel 4
if ($LASTEXITCODE) { throw 'Web control test build failed.' }
& ctest --test-dir $build --output-on-failure
if ($LASTEXITCODE) { throw 'Web control checks failed.' }
