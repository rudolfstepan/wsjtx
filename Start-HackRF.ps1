param(
    [ValidatePattern('^(|[0-9a-fA-F]{1,32})$')][string]$Serial = '',
    [ValidateRange(1000000,6000000000)][long]$FrequencyHz = 432500000,
    [ValidateRange(1024,65535)][int]$Port = 40003,
    [ValidateRange(0,40)][int]$RxLna = 16,
    [ValidateRange(0,62)][int]$RxVga = 20,
    [ValidateRange(0,47)][int]$TxGain = 0,
    [ValidateRange(0.01,1)][double]$Amplitude = 0.5,
    [switch]$Simulate,
    [switch]$PrepareOnly,
    [string]$Package = ''
)
$ErrorActionPreference = 'Stop'
if ($RxLna % 8 -or $RxVga % 2) { throw 'RX LNA requires 8 dB steps; RX VGA requires 2 dB steps.' }
if (!$Package) { $Package = if (Test-Path -LiteralPath "$PSScriptRoot\bin\wsjtx.exe") { $PSScriptRoot } else { "$PSScriptRoot\dist-custom\OE3SRO-reist16-hackrf" } }
$bin = Join-Path $Package 'bin'
$exe = Join-Path $bin 'wsjtx.exe'
$bridge = Join-Path $bin 'wsjtx_hackrf_bridge.exe'
foreach ($file in @($exe,$bridge)) { if (!(Test-Path -LiteralPath $file)) { throw "Missing HackRF build: $file" } }
# An optional repository-local selection is never shipped in portable packages.
$selection = Join-Path $PSScriptRoot '.local\hackrf-settings.json'
if (!$Serial -and (Test-Path -LiteralPath $selection)) { $Serial = (Get-Content -LiteralPath $selection -Raw | ConvertFrom-Json).serial }
if ($Serial -and $Serial -notmatch '^[0-9a-fA-F]{1,32}$') { throw 'Invalid saved HackRF serial.' }
$profileDir = Join-Path $env:LOCALAPPDATA 'WSJT-X - HackRF'
$profile = Join-Path $profileDir 'WSJT-X - HackRF.ini'
$source = Join-Path $env:LOCALAPPDATA 'WSJT-X - Pluto\WSJT-X - Pluto.ini'
if (!(Test-Path -LiteralPath $source)) { $source = Join-Path $env:LOCALAPPDATA 'WSJT-X - Custom\WSJT-X - Custom.ini' }
$running = Get-CimInstance Win32_Process -Filter "Name='wsjtx.exe'" | Where-Object { $_.CommandLine -match '--rig-name\s+"?HackRF(?:"|\s|$)' }
if ($running) { throw 'WSJT-X HackRF is already running. Close that instance normally before starting it again.' }
$runtime = Join-Path $profileDir 'bridge'
New-Item -ItemType Directory -Force -Path $runtime | Out-Null
$session = [guid]::NewGuid().ToString('N')
$prepareOut = Join-Path $runtime "$session-profile.json"
$prepareError = Join-Path $runtime "$session-profile-error.log"
$prepare = @('--prepare-hackrf-profile',"`"$profile`"",'--hackrf-station-source',"`"$source`"",'--hackrf-frequency',"$FrequencyHz",'--hackrf-port',"$Port")
$prepared = Start-Process -FilePath $exe -ArgumentList $prepare -WorkingDirectory $bin -WindowStyle Hidden -Wait -PassThru -RedirectStandardOutput $prepareOut -RedirectStandardError $prepareError
if ($prepared.ExitCode) { throw "Cannot prepare HackRF profile. See $prepareError" }
$effective = Get-Content -LiteralPath $prepareOut -Raw | ConvertFrom-Json
# Existing profile choices win, so the bridge and WSJT-X always use the same endpoint/dial.
$Port = [int]$effective.port
$FrequencyHz = [long]$effective.frequency
if ($PrepareOnly) { [pscustomobject]@{profile=$profile;port=$Port;frequency=$FrequencyHz;serial=$Serial;package=$Package;hardware_opened=$false}; return }
$ready = Join-Path $runtime "$session-ready.json"
$stop = Join-Path $runtime "$session-stop"
$outLog = Join-Path $runtime "$session-stdout.log"
$errorLog = Join-Path $runtime "$session-stderr.log"
$culture = [Globalization.CultureInfo]::InvariantCulture
$bridgeArgs = @('--port',"$Port",'--frequency',"$FrequencyHz",'--rx-lna',"$RxLna",'--rx-vga',"$RxVga",'--tx-gain',"$TxGain",'--amplitude',$Amplitude.ToString($culture),'--ready-file',"`"$ready`"",'--stop-file',"`"$stop`"")
if ($Serial) { $bridgeArgs += @('--serial',$Serial) }
if ($Simulate) { $bridgeArgs += '--simulate' }
$service = Start-Process -FilePath $bridge -ArgumentList $bridgeArgs -WorkingDirectory $bin -WindowStyle Hidden -PassThru -RedirectStandardOutput $outLog -RedirectStandardError $errorLog
try {
    $deadline = [DateTime]::UtcNow.AddSeconds(15)
    while (!(Test-Path -LiteralPath $ready) -and !$service.HasExited -and [DateTime]::UtcNow -lt $deadline) { Start-Sleep -Milliseconds 100 }
    if (!(Test-Path -LiteralPath $ready)) { throw "HackRF bridge did not start. See $errorLog" }
    $status = Get-Content -LiteralPath $ready -Raw | ConvertFrom-Json
    if (!$status.ok) { throw "HackRF bridge: $($status.error). See $errorLog" }
    Write-Output "HackRF bridge ready at 127.0.0.1:$Port; starting the separate HackRF profile."
    $appOut = Join-Path $runtime "$session-wsjtx-stdout.log"
    $appError = Join-Path $runtime "$session-wsjtx-stderr.log"
    # This user-invoked launcher opens the interactive application.
    $app = Start-Process -FilePath $exe -ArgumentList '--rig-name','HackRF' -WorkingDirectory $bin -WindowStyle Normal -PassThru -RedirectStandardOutput $appOut -RedirectStandardError $appError
    while (!$app.HasExited) { if ($service.HasExited) { Write-Warning "HackRF bridge stopped. Check $errorLog"; break }; Start-Sleep -Milliseconds 500 }
    if ($app.HasExited) { $app.WaitForExit(); if ($app.ExitCode) { throw "WSJT-X HackRF exited with code $($app.ExitCode). See $appError" } }
} finally {
    if (!$service.HasExited) { [IO.File]::WriteAllText($stop,'stop'); if (!$service.WaitForExit(5000)) { Write-Warning "HackRF bridge is still cleaning up. See $errorLog" } }
}
