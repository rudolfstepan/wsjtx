param([string]$Package = '')
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
if (!$Package) { $Package = Join-Path $root 'dist-custom\OE3SRO-hackrf-ft8-17m' }
$previousAppData = $env:LOCALAPPDATA
$work = Join-Path $root ('tmp\hackrf-ft8-launcher-' + [guid]::NewGuid().ToString('N'))
$global:hackrfFt8MockApps = @()
function Get-CimInstance { param($ClassName,$Filter) return $global:hackrfFt8MockApps }
function Assert-Check($Condition,[string]$Message) { if (!$Condition) { throw $Message } }
try {
    $env:LOCALAPPDATA = $work
    $sourceDir = Join-Path $work 'WSJT-X - Custom'
    New-Item -ItemType Directory -Force -Path $sourceDir | Out-Null
    $source = Join-Path $sourceDir 'WSJT-X - Custom.ini'
    [IO.File]::WriteAllText($source,"[Configuration]`r`nMyCall=OE3SRO`r`nMyGrid=JN88`r`nRig=Yaesu FT-991`r`nPassword=private`r`n")
    $sourceHash = (Get-FileHash -LiteralPath $source).Hash
    $launcher = Join-Path $root 'Start-HackRF-FT8-17m.ps1'
    $prepared = & $launcher -Package $Package -PrepareOnly
    Assert-Check (!$prepared.hardware_opened -and $prepared.port -eq 40004 -and $prepared.frequency -eq 18100000) 'Wrong initial frequency/port or hardware opened'
    Assert-Check ($prepared.tx_gain -eq 47 -and $prepared.amplitude -eq 1 -and $prepared.tx_amp) 'Maximum TX settings missing'
    $text = [IO.File]::ReadAllText($prepared.profile)
    foreach ($value in @('Mode=FT8','DialFreq=18100000','UDPServerPort=2243','TCIAudio=true','AutoTx=false','MyCall=OE3SRO','MyGrid=JN88')) {
        Assert-Check ($text.Contains($value)) "Missing profile setting: $value"
    }
    Assert-Check (!$text.Contains('private') -and !$text.Contains('Yaesu')) 'Unrelated source configuration imported'
    Assert-Check ((Get-FileHash -LiteralPath $source).Hash -eq $sourceHash) 'Source profile changed'
    $text = $text.Replace('DialFreq=18100000','DialFreq=14074000').Replace('Mode=FT8','Mode=FT4')
    [IO.File]::WriteAllText($prepared.profile,$text)
    $again = & $launcher -Package $Package -PrepareOnly -TxGain 0 -Amplitude 0.5 -TxAmp:$false
    Assert-Check ($again.frequency -eq 14074000 -and [IO.File]::ReadAllText($again.profile).Contains('Mode=FT4')) 'Existing frequency/mode lost'
    Assert-Check ($again.tx_gain -eq 0 -and $again.amplitude -eq 0.5 -and !$again.tx_amp) 'Power overrides ignored'
    foreach ($name in @('HackRF','HackRF-FT8-17m')) {
        $global:hackrfFt8MockApps = @([pscustomobject]@{CommandLine="wsjtx.exe --rig-name $name"})
        $blocked = $false
        try { & $launcher -Package $Package -PrepareOnly | Out-Null } catch { $blocked = $_.Exception.Message -match 'already running' }
        Assert-Check $blocked 'Concurrent HackRF instance not blocked'
    }
    Write-Output 'PASS HackRF FT8 launcher: isolated profile, 17m FT8, direct TCI audio, maximum TX defaults, power overrides, profile preservation and concurrent-device protection. Temporary profiles only; no GUI or hardware opened.'
} finally { $env:LOCALAPPDATA = $previousAppData }
