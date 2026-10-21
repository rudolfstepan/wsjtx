param(
    [string]$Uri = 'ip:192.168.2.1',
    [ValidateRange(325000000,3800000000)][long]$FrequencyHz = 432500000,
    [ValidateRange(1024,65535)][int]$Port = 40002,
    [ValidateRange(0,70)][double]$RxGain = 30,
    [ValidateRange(-89.75,0)][double]$TxGain = -40,
    [switch]$Simulate,
    [string]$Package = ''
)
$ErrorActionPreference = 'Stop'
if (!$Package) {
    $Package = if (Test-Path -LiteralPath "$PSScriptRoot\bin\wsjtx.exe") { $PSScriptRoot } else { "$PSScriptRoot\dist-custom\OE3SRO-reist16-pluto-timing" }
}
$bin = Join-Path $Package 'bin'
$exe = Join-Path $bin 'wsjtx.exe'
$bridge = Join-Path $bin 'wsjtx_pluto_bridge.exe'
foreach ($file in @($exe,$bridge)) { if (!(Test-Path -LiteralPath $file)) { throw "Missing Pluto build: $file" } }
$profileDir = Join-Path $env:LOCALAPPDATA 'WSJT-X - Pluto'
$profile = Join-Path $profileDir 'WSJT-X - Pluto.ini'
$source = Join-Path $env:LOCALAPPDATA 'WSJT-X - Custom\WSJT-X - Custom.ini'
$running = Get-CimInstance Win32_Process -Filter "Name='wsjtx.exe'" | Where-Object { $_.CommandLine -match '--rig-name\s+"?Pluto(?:"|\s|$)' }
if ($running) { throw 'The WSJT-X Pluto instance is already running. Close it normally before starting again.' }
$prepare = @('--prepare-pluto-profile',"`"$profile`"",'--pluto-station-source',"`"$source`"",'--pluto-frequency',"$FrequencyHz",'--pluto-port',"$Port")
$prepared = Start-Process -FilePath $exe -ArgumentList $prepare -WorkingDirectory $bin -WindowStyle Hidden -Wait -PassThru
if ($prepared.ExitCode) { throw "Cannot prepare Pluto profile ($($prepared.ExitCode))." }
$runtime = Join-Path $profileDir 'bridge'
New-Item -ItemType Directory -Force -Path $runtime | Out-Null
$session = [guid]::NewGuid().ToString('N')
$ready = Join-Path $runtime "$session-ready.json"
$stop = Join-Path $runtime "$session-stop"
$outLog = Join-Path $runtime "$session-stdout.log"
$errorLog = Join-Path $runtime "$session-stderr.log"
$culture = [Globalization.CultureInfo]::InvariantCulture
$bridgeArgs = @('--uri',"`"$Uri`"",'--port',"$Port",'--frequency',"$FrequencyHz",'--rx-gain',$RxGain.ToString($culture),'--tx-gain',$TxGain.ToString($culture),'--ready-file',"`"$ready`"",'--stop-file',"`"$stop`"")
if ($Simulate) { $bridgeArgs += '--simulate' }
$service = Start-Process -FilePath $bridge -ArgumentList $bridgeArgs -WorkingDirectory $bin -WindowStyle Hidden -PassThru -RedirectStandardOutput $outLog -RedirectStandardError $errorLog
try {
    $deadline = [DateTime]::UtcNow.AddSeconds(15)
    while (!(Test-Path -LiteralPath $ready) -and !$service.HasExited -and [DateTime]::UtcNow -lt $deadline) { Start-Sleep -Milliseconds 100 }
    if (!(Test-Path -LiteralPath $ready)) { throw "Pluto bridge did not start. See $errorLog" }
    $status = Get-Content -LiteralPath $ready -Raw | ConvertFrom-Json
    if (!$status.ok) { throw "Pluto bridge: $($status.error). See $errorLog" }
    Write-Output "Pluto bridge ready at 127.0.0.1:$Port; starting the separate Pluto profile."
    $appOut = Join-Path $runtime "$session-wsjtx-stdout.log"
    $appError = Join-Path $runtime "$session-wsjtx-stderr.log"
    $app = Start-Process -FilePath $exe -ArgumentList '--rig-name','Pluto' -WorkingDirectory $bin -WindowStyle Normal -PassThru -RedirectStandardOutput $appOut -RedirectStandardError $appError
    while (!$app.HasExited) {
        if ($service.HasExited) { Write-Warning "Pluto bridge stopped. Check $errorLog"; break }
        Start-Sleep -Milliseconds 500
    }
    if ($app.HasExited) {
        $app.WaitForExit()
        if ($app.ExitCode) { throw "WSJT-X Pluto exited with code $($app.ExitCode). See $appError and Windows Application event log." }
    }
} finally {
    # Graceful shutdown runs IIO cleanup and mutes TX before destroying buffers.
    if (!$service.HasExited) {
        [IO.File]::WriteAllText($stop,'stop')
        if (!$service.WaitForExit(5000)) { Write-Warning "Pluto bridge is still cleaning up. See $errorLog" }
    }
}
