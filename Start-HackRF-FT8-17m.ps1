param(
    [ValidatePattern('^(|[0-9a-fA-F]{1,32})$')][string]$Serial = '',
    [ValidateRange(0,40)][int]$RxLna = 16,
    [ValidateRange(0,62)][int]$RxVga = 20,
    [ValidateRange(0,47)][int]$TxGain = 47,
    [ValidateRange(0.01,1)][double]$Amplitude = 1.0,
    [switch]$TxAmp = $true,
    [switch]$Simulate,
    [switch]$PrepareOnly,
    [string]$Package = ''
)
$ErrorActionPreference = 'Stop'
if (!$Package) {
    $Package = if (Test-Path -LiteralPath "$PSScriptRoot\bin\wsjtx.exe") { $PSScriptRoot } else { "$PSScriptRoot\dist-custom\OE3SRO-hackrf-ft8-17m" }
}
# Separate profile: defaults apply once; later frequency/mode choices persist.
$launch = @{} + $PSBoundParameters
$launch.Package = $Package
$launch.TxGain = $TxGain
$launch.Amplitude = $Amplitude
$launch.TxAmp = $TxAmp
& "$PSScriptRoot\Start-HackRF.ps1" @launch -ProfileName HackRF-FT8-17m -InitialMode FT8 -FrequencyHz 18100000 -Port 40004 -UdpPort 2243
