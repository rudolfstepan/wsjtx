param(
    [string]$BuildDirectory = 'build-custom-make',
    [string]$OutputDirectory = ('tmp/reist16-multisignal-' + (Get-Date -Format 'yyyyMMdd-HHmmss')),
    [uint32]$Seed = 2026102601
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
$build = (Resolve-Path (Join-Path $root $BuildDirectory)).Path
$output = [IO.Path]::GetFullPath((Join-Path $root $OutputDirectory))
if (!$output.StartsWith($root + '\', [StringComparison]::OrdinalIgnoreCase)) { throw 'Output must stay in the repository.' }
if (Test-Path -LiteralPath $output) { throw 'Use a new output directory to preserve evidence.' }
New-Item -ItemType Directory -Path $output | Out-Null
$mingw = if ($env:WSJTX_MINGW_PREFIX) { $env:WSJTX_MINGW_PREFIX } else { 'C:\msys64\mingw64' }
$oldPath = $env:PATH
try {
    $env:PATH = "$mingw\bin;$oldPath"
    Push-Location $build
    try {
        $libs = Get-Content CMakeFiles/reist16_qso2_checks.dir/linkLibs.rsp -Raw
        if (!(Test-Path libwsjt_fort.a)) { $libs = $libs.Replace('libwsjt_fort.a', 'libwsjt_fort_omp.a') }
        $libs | Set-Content -LiteralPath "$output/libs.rsp"
        & "$mingw/bin/c++.exe" -std=gnu++11 -O3 -fopenmp "-I$root" "$PSScriptRoot/probe.cpp" -o "$output/probe.exe" "@$output/libs.rsp"
        if ($LASTEXITCODE) { throw 'Probe link failed; configure/build the production libraries first.' }
    } finally { Pop-Location }
    Push-Location $output
    try {
        & ./probe.exe $Seed | Tee-Object native.txt
        if ($LASTEXITCODE) { throw 'Probe failed.' }
        $expected = Get-Content expected.tsv | ConvertFrom-Csv -Delimiter "`t" -Header Case,Message
        $rows = @()
        foreach ($group in ($expected | Group-Object Case)) {
            foreach ($mode in @('reist','ft8')) {
                $modeArg = if ($mode -eq 'reist') { '--reist16' } else { '-8' }
                $cliArgs = @($modeArg,'-d','3','-L','100','-H','4000',"${mode}_$($group.Name)_000000.wav")
                $watch = [Diagnostics.Stopwatch]::StartNew()
                $lines = & "$build/jt9.exe" @cliArgs
                if ($LASTEXITCODE) { throw 'Production decoder failed.' }
                $seconds = $watch.Elapsed.TotalSeconds
                $lines | Set-Content "${mode}_$($group.Name).txt"
                $decodes = @($lines | Where-Object { $_ -match '^\d{6}\s' -and $_ -match '[%~]' })
                $exact = @($group.Group | Where-Object {
                    $message = $_.Message
                    @($decodes | Where-Object { $_.Contains($message) }).Count -gt 0
                }).Count
                $badCount = @($decodes | Where-Object {
                    $line = $_
                    @($group.Group | Where-Object { $line.Contains($_.Message) }).Count -eq 0
                }).Count
                $row = [pscustomobject]@{Case=$group.Name;Mode=$mode;Stations=$group.Count;Exact=$exact;False=$badCount;Seconds=$seconds}
                $rows += $row
                $row | ConvertTo-Json -Compress | Write-Output
                $rows | Export-Csv production.csv -NoTypeInformation
                if ($badCount) { throw "Unexpected decoded payload: $mode $($group.Name)" }
            }
        }
        $paths = @('lib/reist16/qso2.cpp','lib/reist16/qso2.hpp','lib/reist16/reist16.cpp',
                   'lib/reist16/reist16.f90','lib/reist16/reist16.hpp','lib/reist16/subtract.cpp',
                   'lib/reist16/subtract.hpp','lib/ft8_decode.f90','docs/validation/reist16-multisignal/probe.cpp',
                   'docs/validation/reist16-multisignal/Run-Probe.ps1','tests/reist16/multisignal_checks.cpp',
                   'tests/reist16/qso2_checks.cpp','tests/reist16/qso2_channel_checks.cpp','CMakeLists.txt',
                   "$BuildDirectory/jt9.exe", "$BuildDirectory/libwsjt_cxx.a", "$BuildDirectory/libwsjt_fort_omp.a")
        $paths | ForEach-Object { [pscustomobject]@{Path=$_;SHA256=(Get-FileHash (Join-Path $root $_)).Hash} } |
            ConvertTo-Json | Set-Content sources.json
        [pscustomobject]@{Seed=$Seed;BuildDirectory=$BuildDirectory;Cases=($expected | Group-Object Case).Count} |
            ConvertTo-Json | Set-Content conditions.json
    } finally { Pop-Location }
} finally { $env:PATH = $oldPath }
