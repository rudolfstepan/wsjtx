param([switch]$SkipBuild, [string]$BuildDirectory = '')
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$mingw = if ($env:WSJTX_MINGW_PREFIX) { $env:WSJTX_MINGW_PREFIX } else { 'C:\msys64\mingw64' }
$oldPath = $env:PATH
try {
    $env:PATH = "$mingw\bin;$env:PATH"
    if (!$BuildDirectory) {
        $BuildDirectory = 'build-custom-make'
    }
    $build = if ([IO.Path]::IsPathRooted($BuildDirectory)) { $BuildDirectory } else { Join-Path $root $BuildDirectory }
    if (!$SkipBuild) {
        & "$mingw\bin\cmake.exe" --build $build --target reist16_tx_message_checks reist16_qso2_checks reist16_multisignal_checks reist16_weak_checks reist16_checks reist16_wave_checks reist16_audio_checks jt9 --parallel 6
        if ($LASTEXITCODE) { throw 'REIST16 test build failed.' }
    }
    $work = Join-Path $root '.local\reist16-checks'
    New-Item -ItemType Directory -Force -Path $work | Out-Null
    Push-Location $work
    try {
        & "$build\reist16_tx_message_checks.exe"
        if ($LASTEXITCODE) { throw 'REIST16 transmit preflight / input limits failed.' }
        & "$build\reist16_qso2_checks.exe" --unit
        if ($LASTEXITCODE) { throw 'QSO/2 contract tests failed.' }
        & "$build\reist16_multisignal_checks.exe"
        if ($LASTEXITCODE) { throw 'REIST16 multisignal / cancellation tests failed.' }
        & "$build\reist16_weak_checks.exe" --unit
        if ($LASTEXITCODE) { throw 'REIST16 exact sync reference tests failed.' }
        & "$build\reist16_checks.exe"
        if ($LASTEXITCODE) { throw 'REIST16 native tests failed.' }
        & "$build\reist16_wave_checks.exe"
        if ($LASTEXITCODE) { throw 'REIST16 QSO waveform tests failed.' }
        $waves = 1..6 | ForEach-Object { "reist16_${_}_000000.wav" }
        $decoded = & "$build\jt9.exe" --reist16 @waves
        if ($LASTEXITCODE) { throw 'REIST16 jt9 process failed.' }
        $decoded | Set-Content -LiteralPath 'decoded-output.txt' -Encoding utf8
        $messages = @('CQ OE3SRO JN87','OE3SRO DL1ABC JO31','DL1ABC OE3SRO -12',
                      'OE3SRO DL1ABC R-10','DL1ABC OE3SRO RR73','OE3SRO DL1ABC 73')
        foreach ($message in $messages) {
            if (!($decoded | Where-Object { $_.Contains("%  $message") })) { throw "Missing QSO decode: $message" }
        }
        if (@($decoded | Where-Object { $_ -match '^<DecodeFinished>\s+0\s+1\s+0$' }).Count -ne 6) {
            throw 'Unexpected jt9 decode counts.'
        }
        & "$build\reist16_audio_checks.exe" 'decoded-output.txt'
        if ($LASTEXITCODE) { throw 'REIST16 production audio / QSO parser tests failed.' }
        $decoded
    } finally { Pop-Location }
} finally { $env:PATH = $oldPath }
