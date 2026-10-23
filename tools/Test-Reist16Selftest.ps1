$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$work = Join-Path $root ('tmp\selftest-launcher-check-' + [guid]::NewGuid().ToString('N'))
$previousAppData = $env:LOCALAPPDATA
$global:reist16MockApps = @()
$global:reist16TestLaunches = [Collections.Generic.List[object]]::new()
function Get-CimInstance { param($ClassName,$Filter) return $global:reist16MockApps }
function Start-Process {
    param($FilePath,$ArgumentList,$WorkingDirectory,$WindowStyle,[switch]$PassThru)
    $global:reist16TestLaunches.Add([pscustomobject]@{exe=$FilePath;arguments=@($ArgumentList)})
    if ($PassThru) { [pscustomobject]@{HasExited=$false} }
}
function Assert-Check($Condition,[string]$Message) { if (!$Condition) { throw $Message } }
try {
    $env:LOCALAPPDATA = $work
    $sourceDir = Join-Path $work 'WSJT-X - Custom'
    New-Item -ItemType Directory -Force -Path $sourceDir | Out-Null
    $source = Join-Path $sourceDir 'WSJT-X - Custom.ini'
    $fixture = @'
[Configuration]
Rig=Yaesu FT-991
MyCall=OE3SRO
MyGrid=JN87
CATSerialPort=COM987
SoundOutName=Speakers (USB Audio CODEC )
PTTMethod=@Variant(\0\0\0\x7f\0opaque)
[Common]
Mode=FT8
DialFreq=18100000
AutoSeq=true
DXcall=DL1ABC
'@
    [IO.File]::WriteAllText($source,$fixture)
    $hash = (Get-FileHash -LiteralPath $source).Hash
    $launcher = Join-Path $root 'Start-Reist16-Selftest.ps1'
    $prepared = & $launcher -PrepareOnly
    $tx = [IO.File]::ReadAllText($prepared.senderProfile)
    $rx = [IO.File]::ReadAllText($prepared.receiverProfile)
    Assert-Check ($tx.Contains('Rig=Yaesu FT-991') -and $tx.Contains('CATSerialPort=COM987')) 'Yaesu/CAT selection not retained'
    Assert-Check ($tx.Contains('PTTMethod=@Variant(\0\0\0\x7f\0opaque)')) 'Qt serialized value changed'
    Assert-Check ($tx.Contains('SoundOutName=Speakers (USB Audio CODEC )')) 'TX audio output changed'
    foreach ($text in @($tx,$rx)) {
        Assert-Check ($text.Contains('Mode=REIST16') -and $text.Contains('DialFreq=18104000')) 'REIST16/frequency setup missing'
        Assert-Check ($text.Contains('AutoTx=false') -and $text.Contains('AutoSeq=false')) 'Self-test must not enable automatic TX/answers'
    }
    Assert-Check ($rx.Contains('Rig=None') -and !$rx.Contains('CATSerialPort=')) 'Receiver inherited radio control'
    Assert-Check ((Get-FileHash -LiteralPath $source).Hash -eq $hash) 'Original Custom profile modified'
    [IO.File]::WriteAllText($prepared.senderProfile,$tx.Replace('COM987','COM986'))
    $again = & $launcher -PrepareOnly
    Assert-Check ([IO.File]::ReadAllText($again.senderProfile).Contains('CATSerialPort=COM986')) 'Existing test-profile choices overwritten'
    & $launcher | Out-Null
    Assert-Check ($global:reist16TestLaunches.Count -eq 2) 'Self-test must open exactly two instances'
    Assert-Check ($global:reist16TestLaunches[0].arguments -contains '--receive-only') 'Receiver must be receive-only'
    Assert-Check ($global:reist16TestLaunches[0].arguments -contains '--wasapi-loopback') 'Receiver loopback missing'
    Assert-Check (!($global:reist16TestLaunches[1].arguments -contains '--wasapi-loopback')) 'Sender must retain Yaesu audio path'
    $global:reist16TestLaunches.Clear()
    $global:reist16MockApps = @([pscustomobject]@{CommandLine='wsjtx.exe --rig-name Custom'})
    $blocked = $false
    try { & $launcher | Out-Null } catch { $blocked = $_.Exception.Message -match 'exclusive access' }
    Assert-Check ($blocked -and $global:reist16TestLaunches.Count -eq 0) 'Concurrent Yaesu CAT use not blocked'
    $global:reist16MockApps = @([pscustomobject]@{CommandLine='wsjtx.exe --rig-name REIST16-Selftest-RX'})
    $blocked = $false
    try { & $launcher -PrepareOnly | Out-Null } catch { $blocked = $_.Exception.Message -match 'existing REIST16-Selftest-RX' }
    Assert-Check $blocked 'Running test profile can be rewritten'
    Write-Output 'PASS self-test launcher: profile isolation, Qt serialization, preserved CAT/TX audio, REIST16 setup, RX-only loopback, two launches and concurrent-radio protection. No GUI, CAT or HF used.'
} finally { $env:LOCALAPPDATA = $previousAppData }
