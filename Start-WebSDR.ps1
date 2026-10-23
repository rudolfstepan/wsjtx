param(
    [string]$Package = '',
    [string]$PlaybackDevice = 'Realtek Digital Output (Realtek USB Audio)',
    [switch]$PrepareOnly
)
$ErrorActionPreference = 'Stop'
if (!$Package) {
    $Package = if (Test-Path -LiteralPath "$PSScriptRoot\bin\wsjtx.exe") { $PSScriptRoot } else { "$PSScriptRoot\dist-custom\OE3SRO-reist16-websdr" }
}
$exe = Join-Path $Package 'bin\wsjtx.exe'
if (!(Test-Path -LiteralPath $exe)) { throw "Custom build not found: $exe" }
if (!$PlaybackDevice.Trim() -or $PlaybackDevice -match '[\r\n"]') { throw 'Invalid Windows playback device name.' }
$running = Get-CimInstance Win32_Process -Filter "Name='wsjtx.exe'" | Where-Object { $_.CommandLine -match '--rig-name\s+"?WebSDR(?:"|\s|$)' }
if ($running) { throw 'WSJT-X WebSDR is already running.' }
$profileDir = Join-Path $env:LOCALAPPDATA 'WSJT-X - WebSDR'
$profile = Join-Path $profileDir 'WSJT-X - WebSDR.ini'
if (!(Test-Path -LiteralPath $profile)) {
    New-Item -ItemType Directory -Force -Path $profileDir | Out-Null
    # Deliberately start with an independent profile, without radio/CAT settings.
    $initial = @"
[Configuration]
Rig=None
TCIAudio=false
AudioInputChannel=Mono
MonitorOFF=false
PSKReporter=false
UDPServer=127.0.0.1
UDPServerPort=2240
AcceptUDPRequests=false

[Common]
Mode=FT8
DialFreq=18100000
RxFreq=1500
TxFreq=1500
AutoSeq=false
AutoTx=false
"@
    [IO.File]::WriteAllText($profile, $initial, [Text.UTF8Encoding]::new($false))
}
if ($PrepareOnly) { [pscustomobject]@{profile=$profile;executable=$exe;playbackDevice=$PlaybackDevice}; return }
Write-Output "WebSDR: Receiving Windows output via WASAPI loopback: $PlaybackDevice"
Write-Output 'Initial mode: FT8. Match the displayed dial frequency to the browser. Enable TX must remain off.'
# Explicit user-invoked interactive receiver; uses its own settings and log directory.
Start-Process -FilePath $exe -ArgumentList '--rig-name','WebSDR','--receive-only','--wasapi-loopback',"`"$PlaybackDevice`"" -WorkingDirectory (Split-Path $exe) -WindowStyle Normal
