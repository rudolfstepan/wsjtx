param(
    [string]$SourceDirectory = "$env:LOCALAPPDATA\WSJT-X",
    [string]$RigName = 'Custom'
)
$ErrorActionPreference = 'Stop'
if ($RigName -match '[\\/,]' -or [string]::IsNullOrWhiteSpace($RigName)) {
    throw 'Invalid rig name.'
}
$source = Join-Path $SourceDirectory 'WSJT-X.ini'
if (!(Test-Path -LiteralPath $source)) { throw "Configuration not found: $source" }
$root = Split-Path $PSScriptRoot -Parent
$snapshot = Join-Path $root '.local\installed-configuration'
$destination = Join-Path $env:LOCALAPPDATA "WSJT-X - $RigName"
$target = Join-Path $destination "WSJT-X - $RigName.ini"
if (Test-Path -LiteralPath $target) {
    throw "A custom configuration already exists: $target. It was not overwritten."
}
New-Item -ItemType Directory -Force -Path $snapshot, $destination | Out-Null
Copy-Item -LiteralPath $source -Destination (Join-Path $snapshot 'WSJT-X.ini')
Copy-Item -LiteralPath $source -Destination $target
# Preserve local station data and logs used by the copied settings.
foreach ($name in @('CALL3.TXT', 'cty.dat', 'ignore.list', 'wsjtx_log.adi', 'wsjtx.log', 'db.sqlite', 'refspec.dat', 'lotw-user-activity.csv')) {
    $file = Join-Path $SourceDirectory $name
    if (Test-Path -LiteralPath $file) {
        $copy = Join-Path $destination $name
        if (!(Test-Path -LiteralPath $copy)) { Copy-Item -LiteralPath $file -Destination $copy }
    }
}
$sourceHash = (Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash
$targetHash = (Get-FileHash -LiteralPath $target -Algorithm SHA256).Hash
if ($sourceHash -ne $targetHash) { throw 'Configuration copy verification failed.' }
Write-Output "Configuration copied byte-for-byte and verified: $target"
Write-Output 'All settings and existing configured file paths were preserved.'
Write-Output "Start the custom executable with: --rig-name $RigName"
