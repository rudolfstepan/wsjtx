param(
    [string]$Package = '',
    [string]$PlaybackDevice = 'Realtek Digital Output (Realtek USB Audio)',
    [switch]$PrepareOnly
)
# Independent receiver; the WebSDR profile retains its chosen mode/frequency.
& "$PSScriptRoot\Start-WebSDR.ps1" @PSBoundParameters
