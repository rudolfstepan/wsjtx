param([string]$RuntimeSource='D:\Development\sdr-suite\.tools\hackrf\Library\bin',[string]$Destination='')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
if (!$Destination) { $Destination=Join-Path $root '.tools\hackrf\runtime' }
foreach($name in @('hackrf-0.dll','libusb-1.0.dll','libwinpthread-1.dll')) { if (!(Test-Path -LiteralPath (Join-Path $RuntimeSource $name))) { throw "Missing HackRF runtime dependency: $name" } }
New-Item -ItemType Directory -Force -Path $Destination | Out-Null
foreach($name in @('hackrf-0.dll','libusb-1.0.dll','libwinpthread-1.dll','vcruntime140.dll','vcruntime140_1.dll','msvcp140.dll')) {
    $file=Join-Path $RuntimeSource $name
    if(Test-Path -LiteralPath $file){Copy-Item -LiteralPath $file -Destination $Destination -Force}
}
$licenses=Join-Path $Destination 'licenses'
New-Item -ItemType Directory -Force -Path $licenses | Out-Null
$packages='D:\Development\sdr-suite\.tools\mamba\pkgs\https\conda.anaconda.org\conda-forge\win-64'
foreach($package in @('libhackrf0-2026.01.3-hfd05255_0','libusb-1.0.30-h6a83c73_0','libwinpthread-12.0.0.r4.gg4f2fc60ca-h57928b3_10')) {
    $folder=Join-Path $packages "$package\info\licenses"
    if(Test-Path -LiteralPath $folder){$to=Join-Path $licenses $package;New-Item -ItemType Directory -Force -Path $to | Out-Null;Get-ChildItem -LiteralPath $folder | Copy-Item -Destination $to -Recurse -Force}
}
$entries=Get-ChildItem -LiteralPath $Destination -File | Where-Object Name -ne runtime-sources.json | ForEach-Object {[pscustomobject]@{file=$_.Name;sha256=(Get-FileHash -LiteralPath $_.FullName).Hash}}
[pscustomobject]@{libhackrf='2026.01.3';source='https://github.com/greatscottgadgets/hackrf/releases/tag/v2026.01.3';files=@($entries)} | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $Destination 'runtime-sources.json') -Encoding utf8
Write-Output "HackRF runtime prepared without USB/device access: $Destination"
