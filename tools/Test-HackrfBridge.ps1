param([switch]$SkipBuild)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$mingw=if($env:WSJTX_MINGW_PREFIX){$env:WSJTX_MINGW_PREFIX}else{'C:\msys64\mingw64'}
$previousPath=$env:PATH
try{
    $hamlibBin=Join-Path $root '.tools\hamlib\hamlib-w64-4.7.2\bin'
    $env:PATH="$mingw\bin;$hamlibBin;$env:PATH"
    if(!$SkipBuild){& "$mingw\bin\cmake.exe" --build "$root\build-custom" --target wsjtx wsjtx_hackrf_bridge hackrf_bridge_checks pluto_bridge_checks sdr_configuration_checks --parallel 6;if($LASTEXITCODE){throw 'HackRF test build failed'}}
    & "$root\build-custom\sdr_configuration_checks.exe"
    if($LASTEXITCODE){throw 'Production SDR configuration checks failed'}
    & "$root\build-custom\hackrf_bridge_checks.exe"
    if($LASTEXITCODE){throw 'HackRF mock / profile checks failed'}
    & python "$root\tools\Test-PlutoBridgeProtocol.py" "$root\build-custom\wsjtx_hackrf_bridge.exe"
    if($LASTEXITCODE){throw 'HackRF protocol checks failed'}
    & "$root\build-custom\pluto_bridge_checks.exe" --hackrf-simulation
    if($LASTEXITCODE){throw 'Production TCI / QSO2 audio checks against HackRF simulation failed'}
}finally{$env:PATH=$previousPath}
