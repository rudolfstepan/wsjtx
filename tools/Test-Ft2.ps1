param([string]$SamplesDirectory = '')
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$mingw = 'C:\msys64\mingw64'
$env:PATH = "$mingw\bin;$env:PATH"
$env:MSYSTEM_PREFIX = $mingw.Replace('\', '/')
$build = Join-Path $root 'build-custom'
$work = Join-Path $root '.local\ft2-checks'
if (!(Test-Path -LiteralPath "$build\CMakeCache.txt")) { throw 'Run Build-Custom.ps1 first.' }
& "$mingw\bin\cmake.exe" --build $build --target ft2_wave_checks ft2_ui_checks jt9 --parallel 6
if ($LASTEXITCODE) { throw 'FT2 test build failed.' }
New-Item -ItemType Directory -Path $work -Force | Out-Null
if ($SamplesDirectory) { $SamplesDirectory = (Resolve-Path -LiteralPath $SamplesDirectory).Path }
Push-Location $work
try {
    & "$build\ft2_ui_checks.exe"
    if ($LASTEXITCODE) { throw 'FT2 mode/timing/frequency tests failed.' }
    & "$build\ft2_wave_checks.exe"
    if ($LASTEXITCODE) { throw 'FT2 waveform/mode-switch tests failed.' }
    $cases = @(
        @{ File='generated1_000000.wav'; Mode='-2'; Message='CQ K1ABC FN42'; Frequency=1500; Dt=-0.35 },
        @{ File='generated2_000000.wav'; Mode='-2'; Message='K1ABC W9XYZ -10'; Frequency=500; Dt=-0.15 },
        @{ File='generated3_000000.wav'; Mode='-2'; Message='K1ABC W9XYZ RR73'; Frequency=2900; Dt=-0.45 },
        @{ File='generated4_000000.wav'; Mode='-5'; Message='CQ W9XYZ EN37'; Frequency=1500; Dt=-0.2 }
    )
    foreach ($case in $cases) {
        $output = (& "$build\jt9.exe" $case.Mode -d 3 $case.File | Out-String)
        if ($LASTEXITCODE) { throw "Decoder failed: $($case.File)" }
        $marker = if ($case.Mode -eq '-2') { '\*' } else { '\+' }
        $pattern = '(?m)^000000\s+[-\d]+\s+([-\d.]+)\s+(\d+)\s+' + $marker + '\s+' + [regex]::Escape($case.Message)
        $match = [regex]::Match($output, $pattern)
        if (!$match.Success) { throw "Missing round-trip decode: $($case.Message)`n$output" }
        if ([Math]::Abs([int]$match.Groups[2].Value - $case.Frequency) -gt 3) { throw 'Wrong decoded frequency.' }
        if ([Math]::Abs([double]$match.Groups[1].Value - $case.Dt) -gt 0.11) { throw 'Wrong FT2/FT4 DT conversion.' }
        Write-Output "Round trip passed: $($case.Message)"
    }
    if ($SamplesDirectory) {
        $samples = @(
            @{ File='260301_170123.wav'; Hash='5F2A87342D3D46AE6052BC54F39083B20588F46A89588D4C7557B8E9260F8E89'; Messages=@('DH3YAK F1DXP RR73','CQ IR0SAR JN40','CQ 4U1A JN88','3Z0L SV1TMQ KM18','CQ IW0RLC JN63','DL9NCY IT9LKX +01') },
            @{ File='260301_085945.wav'; Hash='E2501935A2E4223061C1A97735269B2DBD9BFD3441CD74A624D795DCE0D68104'; Messages=@('G4SRT F1BLQ +03','EW2RA OE1JUA 73') }
        )
        foreach ($sample in $samples) {
            $file = Join-Path $SamplesDirectory $sample.File
            if ((Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash -ne $sample.Hash) { throw 'Unexpected reference WAV hash.' }
            $output = (& "$build\jt9.exe" -2 -d 3 $file | Out-String)
            if ($LASTEXITCODE) { throw 'Reference WAV decode failed.' }
            foreach ($message in $sample.Messages) {
                if (!$output.Contains($message)) { throw "Missing reference decode: $message" }
            }
            Write-Output "Reference WAV passed: $($sample.File) ($($sample.Messages.Count) stations)"
        }
    }
    Write-Output 'FT2 checks passed. No radio, CAT, or audio device was opened.'
} finally {
    Pop-Location
}
