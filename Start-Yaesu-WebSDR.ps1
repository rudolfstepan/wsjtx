param(
    [string]$Package = '',
    [string]$PlaybackDevice = 'Realtek Digital Output (Realtek USB Audio)',
    [switch]$CheckOnly
)
$ErrorActionPreference = 'Stop'
if (!$Package) {
    $Package = if (Test-Path -LiteralPath "$PSScriptRoot\bin\wsjtx.exe") { $PSScriptRoot } else { "$PSScriptRoot\dist-custom\OE3SRO-reist16-websdr" }
}
$exe = Join-Path $Package 'bin\wsjtx.exe'
$profile = Join-Path $env:LOCALAPPDATA 'WSJT-X - Custom\WSJT-X - Custom.ini'
foreach ($file in @($exe,$profile)) {
    if (!(Test-Path -LiteralPath $file)) { throw "Missing Yaesu/WebSDR setup: $file" }
}
if (!$PlaybackDevice.Trim() -or $PlaybackDevice -match '[\r\n"]') { throw 'Invalid Windows playback device name.' }
$apps = @(Get-CimInstance Win32_Process -Filter "Name='wsjtx.exe'")
$custom = @($apps | Where-Object { $_.CommandLine -match '--rig-name\s+"?Custom(?:"|\s|$)' })
$hybridRunning = @($custom | Where-Object { $_.CommandLine -match '--wasapi-loopback(?:\s|$)' }).Count -gt 0
if ($CheckOnly) {
    [pscustomobject]@{singleInstance=$true;hybridRunning=$hybridRunning;customRunning=$custom.Count -gt 0;executable=$exe;profile=$profile;playbackDevice=$PlaybackDevice}
    return
}
if ($hybridRunning) { Write-Output 'Yaesu/WebSDR is already running in the Custom instance.'; return }
if ($custom.Count) { throw 'Close the existing Custom instance normally first. This start option uses the same Yaesu profile in ONE instance.' }
Write-Output "ONE instance: receive browser audio from $PlaybackDevice; transmit using the existing Yaesu CAT/PTT and audio output."
Write-Output 'Match browser USB frequency and WSJT-X dial frequency. Enable TX remains under your control.'
# Reuse the user's existing Yaesu profile; no import or configuration rewrite.
Start-Process -FilePath $exe -ArgumentList '--rig-name','Custom','--wasapi-loopback',"`"$PlaybackDevice`"" -WorkingDirectory (Split-Path $exe) -WindowStyle Normal
