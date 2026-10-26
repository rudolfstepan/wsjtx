param([switch]$AudioOnly)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$mingw = if ($env:WSJTX_MINGW_PREFIX) { $env:WSJTX_MINGW_PREFIX } else { 'C:\msys64\mingw64' }
$previousPath = $env:PATH
$previousPlatform = $env:QT_QPA_PLATFORM
$previousLimit = $env:OMP_THREAD_LIMIT
$previousThreads = $env:OMP_NUM_THREADS
$previousWait = $env:OMP_WAIT_POLICY
$previousLevels = $env:OMP_MAX_ACTIVE_LEVELS
$previousPrefix = $env:MSYSTEM_PREFIX
try {
    $env:PATH = "$mingw\bin;$env:PATH"
    $env:MSYSTEM_PREFIX = $mingw.Replace('\', '/')
    $audioBuild = Join-Path $root 'build-custom-make\txaudio-checks'
    & "$mingw\bin\cmake.exe" -S "$root/tests/txaudio" -B $audioBuild -G "MinGW Makefiles" "-DCMAKE_PREFIX_PATH=$mingw"
    if ($LASTEXITCODE) { throw 'TX audio test configuration failed.' }
    & "$mingw\bin\cmake.exe" --build $audioBuild --parallel 4
    if ($LASTEXITCODE) { throw 'TX audio test build failed.' }
    $env:QT_QPA_PLATFORM = 'offscreen'
    Push-Location $root
    try {
        & "$mingw\bin\ctest.exe" --test-dir $audioBuild --output-on-failure
        $testExit = $LASTEXITCODE
        Get-Content -LiteralPath "$audioBuild\test-results.txt"
        if ($testExit) { throw 'TX audio regression tests failed.' }
    } finally { Pop-Location }
    if ($AudioOnly) { return }
    $build = Join-Path $root 'build-custom-make'
    & "$mingw\bin\cmake.exe" --build $build --target ft8_wave_checks ft8_native_checks ft8_reference_decoder jt9 --parallel 6
    if ($LASTEXITCODE) { throw 'FT8 waveform test build failed.' }
    $work = Join-Path $root '.local\ft8-tx-checks'
    New-Item -ItemType Directory -Force -Path $work | Out-Null
    Push-Location $work
    try {
        $env:OMP_WAIT_POLICY = 'PASSIVE'
        $env:OMP_NUM_THREADS = '8'
        $env:OMP_THREAD_LIMIT = '16'
        $env:OMP_MAX_ACTIVE_LEVELS = '1'
        & "$build\ft8_native_checks.exe"
        if ($LASTEXITCODE) { throw "Native FT8 reference comparison failed (exit $LASTEXITCODE)." }
        & "$build\ft8_wave_checks.exe"
        if ($LASTEXITCODE) { throw 'FT8 waveform / thread-budget checks failed.' }
        # Deterministic local waveforms, no CAT or audio output. Include a
        # restricted OpenMP team to detect section/barrier deadlocks.
        $cases = @(
            @{ Name='standard-low'; File='ft8_1_000000.wav'; Args=@('-8'); Limit='8'; Threads='8' },
            @{ Name='standard-split'; File='ft8_2_000000.wav'; Args=@('-8'); Limit='8'; Threads='8' },
            @{ Name='standard-high'; File='ft8_3_000000.wav'; Args=@('-8'); Limit='8'; Threads='8' },
            @{ Name='smp-one'; File='ft8_2_000000.wav'; Args=@('-8','-M','-N','1'); Limit='8'; Threads='8' },
            @{ Name='smp-two'; File='ft8_2_000000.wav'; Args=@('-8','-M','-N','2'); Limit='8'; Threads='8' },
            @{ Name='smp-four'; File='ft8_2_000000.wav'; Args=@('-8','-M','-N','4'); Limit='8'; Threads='8' },
            @{ Name='smp-auto'; File='ft8_2_000000.wav'; Args=@('-8','-M','-N','0'); Limit='16'; Threads='16' },
            @{ Name='smp-eight'; File='ft8_2_000000.wav'; Args=@('-8','-M','-N','8'); Limit='16'; Threads='16' },
            @{ Name='smp-twelve'; File='ft8_2_000000.wav'; Args=@('-8','-M','-N','12'); Limit='16'; Threads='16' },
            @{ Name='smp-limited'; File='ft8_2_000000.wav'; Args=@('-8','-M','-N','12'); Limit='2'; Threads='2' }
            @{ Name='smp-disabled-teams'; File='ft8_2_000000.wav'; Args=@('-8','-M','-N','12'); Limit='8'; Threads='8'; Levels='0' }
        )
        $env:OMP_WAIT_POLICY = 'PASSIVE'
        foreach ($case in $cases) {
            $env:OMP_THREAD_LIMIT = $case.Limit
            $env:OMP_NUM_THREADS = $case.Threads
            $env:OMP_MAX_ACTIVE_LEVELS = if ($case.Levels) { $case.Levels } else { '1' }
            $stdout = Join-Path $work "$($case.Name).txt"
            $stderr = Join-Path $work "$($case.Name)-stderr.txt"
            $arguments = $case.Args + @('-d','3','-L','300','-H','3200',$case.File)
            $watch = [Diagnostics.Stopwatch]::StartNew()
            $process = Start-Process -FilePath "$build\jt9.exe" -ArgumentList $arguments -WorkingDirectory $work -WindowStyle Hidden -PassThru -RedirectStandardOutput $stdout -RedirectStandardError $stderr
            if (!$process.WaitForExit(45000)) {
                $process.Kill()
                $process.WaitForExit()
                throw "Decoder timed out: $($case.Name)"
            }
            $process.WaitForExit()
            $output = Get-Content -Raw -LiteralPath $stdout
            if ($process.ExitCode -ne 0 -or !$output.Contains('CQ K1ABC FN42')) {
                throw "FT8 decode failed: $($case.Name)`n$output`n$(Get-Content -Raw -LiteralPath $stderr)"
            }
            Write-Output "FT8 passed: $($case.Name) ($([Math]::Round($watch.Elapsed.TotalSeconds, 2)) s)"
        }
        # Recorded multi-station FT8 sample shipped in this repository. Compare
        # decoded messages with the original kernels; SNR/DT may round differently.
        $env:OMP_THREAD_LIMIT = '16'
        $env:OMP_NUM_THREADS = '16'
        $env:OMP_MAX_ACTIVE_LEVELS = '1'
        $recording = Join-Path $root 'samples\FT8\210703_133430.wav'
        foreach ($parallel in @($false, $true)) {
            $messages = @()
            foreach ($exe in @('ft8_reference_decoder', 'jt9')) {
                $arguments = @('-8','-d','3','-L','300','-H','3200')
                if ($parallel) { $arguments += @('-M','-N','0') }
                $arguments += '"' + $recording + '"'
                $stdout = Join-Path $work "$exe-recording-$parallel.txt"
                $stderr = Join-Path $work "$exe-recording-$parallel-stderr.txt"
                $process = Start-Process -FilePath "$build\$exe.exe" -ArgumentList $arguments -WorkingDirectory $work -WindowStyle Hidden -PassThru -RedirectStandardOutput $stdout -RedirectStandardError $stderr
                if (!$process.WaitForExit(45000)) {
                    $process.Kill()
                    $process.WaitForExit()
                    throw "Recorded FT8 decode timed out: $exe / parallel=$parallel"
                }
                $process.WaitForExit()
                if ($process.ExitCode -ne 0) { throw "Recorded FT8 decode failed: $exe" }
                $decoded = @(Get-Content -LiteralPath $stdout | ForEach-Object {
                    if ($_ -match '^\d{6}\s+[-\d]+\s+[-\d.]+\s+\d+\s+~\s+(.+?)\s*$') { $Matches[1] }
                } | Sort-Object -Unique)
                if (!$decoded.Count) { throw "No recorded FT8 messages parsed: $exe" }
                $messages += ,$decoded
            }
            $missing = @($messages[0] | Where-Object { $_ -notin $messages[1] })
            if ($missing.Count) { throw "Recorded FT8 regression (parallel=$parallel): $($missing -join ', ')" }
            Write-Output "Recorded FT8 passed: parallel=$parallel, original=$($messages[0].Count), native=$($messages[1].Count) messages"
        }
    } finally { Pop-Location }
} finally {
    $env:PATH = $previousPath
    $env:QT_QPA_PLATFORM = $previousPlatform
    $env:OMP_THREAD_LIMIT = $previousLimit
    $env:OMP_NUM_THREADS = $previousThreads
    $env:OMP_WAIT_POLICY = $previousWait
    $env:OMP_MAX_ACTIVE_LEVELS = $previousLevels
    $env:MSYSTEM_PREFIX = $previousPrefix
}
