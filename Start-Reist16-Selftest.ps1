param(
    [string]$Package = '',
    [string]$PlaybackDevice = 'Realtek Digital Output (Realtek USB Audio)',
    [ValidateRange(18095000,18107000)][long]$FrequencyHz = 18104000,
    [switch]$PrepareOnly
)
$ErrorActionPreference = 'Stop'
if (!$Package) {
    $Package = if (Test-Path -LiteralPath "$PSScriptRoot\bin\wsjtx.exe") { $PSScriptRoot } else { "$PSScriptRoot\dist-custom" }
}
$exe = Join-Path $Package 'bin\wsjtx.exe'
$source = Join-Path $env:LOCALAPPDATA 'WSJT-X - Custom\WSJT-X - Custom.ini'
foreach ($file in @($exe,$source)) {
    if (!(Test-Path -LiteralPath $file)) { throw "Missing REIST16 self-test setup: $file" }
}
if (!$PlaybackDevice.Trim() -or $PlaybackDevice -match '[\r\n"]') { throw 'Invalid Windows playback device name.' }
$apps = @(Get-CimInstance Win32_Process -Filter "Name='wsjtx.exe'")
foreach ($name in @('REIST16-Selftest-TX','REIST16-Selftest-RX')) {
    $pattern = '--rig-name\s+"?' + [regex]::Escape($name) + '(?:"|\s|$)'
    if ($apps | Where-Object { $_.CommandLine -match $pattern }) { throw "Close the existing $name instance normally before preparing or starting this self-test." }
}
if (!$PrepareOnly -and ($apps | Where-Object { $_.CommandLine -match '--rig-name\s+"?Custom(?:"|\s|$)' })) {
    throw 'Close WSJT-X Custom normally first: the self-test sender needs exclusive access to the Yaesu CAT port.'
}

# Change plain scalar keys only. Preserve Qt's serialized values and every
# unrelated line in the copied Custom profile; never write the source profile.
function Set-IniScalar([string]$Text, [string]$Section, [string]$Key, [string]$Value) {
    $lines = [Collections.Generic.List[string]]::new()
    $lines.AddRange([string[]]($Text -split '\r?\n'))
    $start = -1
    $end = $lines.Count
    for ($i=0; $i -lt $lines.Count; ++$i) {
        if ($lines[$i] -eq "[$Section]") { $start = $i; continue }
        if ($start -ge 0 -and $lines[$i] -match '^\[') { $end = $i; break }
    }
    if ($start -lt 0) { $lines.Add("[$Section]"); $lines.Add("$Key=$Value") }
    else {
        $found = $false
        for ($i=$start+1; $i -lt $end; ++$i) {
            if ($lines[$i] -match ('^' + [regex]::Escape($Key) + '=')) {
                $lines[$i] = "$Key=$Value"; $found = $true; break
            }
        }
        if (!$found) { $lines.Insert($end,"$Key=$Value") }
    }
    return ($lines -join "`r`n")
}

$profiles = @{}
foreach ($side in @('TX','RX')) {
    $name = "REIST16-Selftest-$side"
    $dir = Join-Path $env:LOCALAPPDATA "WSJT-X - $name"
    $profile = Join-Path $dir "WSJT-X - $name.ini"
    New-Item -ItemType Directory -Force -Path $dir | Out-Null
    if (Test-Path -LiteralPath $profile) { $text = [IO.File]::ReadAllText($profile) }
    elseif ($side -eq 'TX') { $text = [IO.File]::ReadAllText($source) }
    else { $text = "[Configuration]`r`nRig=None`r`nTCIAudio=false`r`n[Common]`r`n" }
    $values = @{
        Common = @{Mode='REIST16'; DialFreq="$FrequencyHz"; TxFreq='1500'; RxFreq='1500'; AutoSeq='false'; AutoTx='false'; DXcall=''; DXgrid=''; TxFirst='true'}
        Configuration = @{PSKReporter='false'; UDPServer='127.0.0.1'; UDPServerPort=$(if($side -eq 'TX'){'2241'}else{'2242'}); AcceptUDPRequests='false'; MonitorOFF='false'; SaveDir=($dir.Replace('\','/')+'/save'); AzElDir=$dir.Replace('\','/')}
    }
    if ($side -eq 'RX') { $values.Configuration.Rig='None'; $values.Configuration.TCIAudio='false'; $values.Configuration.AudioInputChannel='Mono' }
    foreach ($section in $values.Keys) {
        foreach ($key in $values[$section].Keys) { $text = Set-IniScalar $text $section $key $values[$section][$key] }
    }
    [IO.File]::WriteAllText($profile,$text,[Text.UTF8Encoding]::new($false))
    $profiles[$side] = $profile
}
if ($PrepareOnly) {
    [pscustomobject]@{senderProfile=$profiles.TX;receiverProfile=$profiles.RX;frequency=$FrequencyHz;playbackDevice=$PlaybackDevice;executable=$exe}
    return
}
# The separate receiver remains monitoring while the Yaesu sender transmits.
$rx = Start-Process -FilePath $exe -ArgumentList '--rig-name','REIST16-Selftest-RX','--receive-only','--wasapi-loopback',"`"$PlaybackDevice`"" -WorkingDirectory (Split-Path $exe) -WindowStyle Normal -PassThru
Start-Sleep -Milliseconds 500
if ($rx.HasExited) { throw 'The self-test receiver exited before the sender could start.' }
Start-Process -FilePath $exe -ArgumentList '--rig-name','REIST16-Selftest-TX' -WorkingDirectory (Split-Path $exe) -WindowStyle Normal
Write-Output "REIST16 self-test: Yaesu TX and independent WebSDR RX at $FrequencyHz Hz USB, audio 1500 Hz."
Write-Output 'Set the browser to the same USB frequency. In the TX window select Tx6 (CQ), check the frequency is clear, then enable TX yourself.'
Write-Output 'Your CQ should appear in Band Activity in the RX window after the transmit slot. No transmission was enabled by this launcher.'
