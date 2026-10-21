param(
    [string]$RuntimeSource = 'D:\Development\sdr-suite\.tools\pluto\runtime',
    [switch]$Download,
    [string]$Destination = ''
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$destination = if ($Destination) { [IO.Path]::GetFullPath($Destination) } else { Join-Path $root '.tools\pluto\runtime' }
if ($Download) {
    $cache = Join-Path $root '.tools\pluto\downloads'
    New-Item -ItemType Directory -Force -Path $cache | Out-Null
    $archive = Join-Path $cache 'libiio-0.26-Windows.zip'
    if (!(Test-Path -LiteralPath $archive)) {
        Invoke-WebRequest 'https://github.com/analogdevicesinc/libiio/releases/download/v0.26/Windows.zip' -OutFile $archive
    }
    $expected = '4AD4A8C6B3F7145922C122DCFC51D693F0D3F9A0054FB2228814CE58C538A6CA'
    if ((Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash -ne $expected) { throw 'Unexpected official libiio 0.26 archive SHA256.' }
    $unpack = Join-Path $cache 'libiio-0.26'
    Expand-Archive -LiteralPath $archive -DestinationPath $unpack -Force
    New-Item -ItemType Directory -Force -Path $destination | Out-Null
    foreach ($name in @('libiio.dll','libusb-1.0.dll','libxml2.dll','libserialport-0.dll','msvcp140.dll','vcruntime140.dll','iio_info.exe','iio_attr.exe')) {
        Copy-Item -LiteralPath (Join-Path $unpack "Windows-VS-2019-x64/$name") -Destination $destination -Force
    }
    $licenses = Join-Path $destination 'licenses'
    New-Item -ItemType Directory -Force -Path $licenses | Out-Null
    $sources = @{
        'COPYING.txt' = 'https://raw.githubusercontent.com/analogdevicesinc/libiio/v0.26/COPYING.txt'
        'COPYING_GPL.txt' = 'https://raw.githubusercontent.com/analogdevicesinc/libiio/v0.26/COPYING_GPL.txt'
        'libusb-COPYING.txt' = 'https://raw.githubusercontent.com/libusb/libusb/v1.0.24/COPYING'
        'libusb-AUTHORS.txt' = 'https://raw.githubusercontent.com/libusb/libusb/v1.0.24/AUTHORS'
        'libserialport-COPYING.txt' = 'https://raw.githubusercontent.com/sigrokproject/libserialport/master/COPYING'
        'libserialport-AUTHORS.txt' = 'https://raw.githubusercontent.com/sigrokproject/libserialport/master/AUTHORS'
        'libxml2-Copyright.txt' = 'https://raw.githubusercontent.com/GNOME/libxml2/v2.9.12/Copyright'
    }
    foreach ($entry in $sources.GetEnumerator()) { Invoke-WebRequest $entry.Value -OutFile (Join-Path $licenses $entry.Key) }
    $sources | ConvertTo-Json | Set-Content (Join-Path $licenses 'sources.json')
    Copy-Item -LiteralPath (Join-Path $unpack 'README.txt') -Destination $licenses -Force
    @{version='0.26';source='https://github.com/analogdevicesinc/libiio/releases/tag/v0.26';archive_sha256=$expected} | ConvertTo-Json | Set-Content (Join-Path $destination 'source.json')
    Write-Output "Pinned official libiio 0.26 runtime prepared: $destination"
    return
}
if (!(Test-Path -LiteralPath (Join-Path $RuntimeSource 'libiio.dll'))) { throw "libiio 0.26 runtime missing: $RuntimeSource. Use -Download or supply -RuntimeSource." }
New-Item -ItemType Directory -Force -Path (Split-Path $destination) | Out-Null
if (([IO.Path]::GetFullPath($RuntimeSource)).TrimEnd('\') -ne ([IO.Path]::GetFullPath($destination)).TrimEnd('\')) {
    New-Item -ItemType Directory -Force -Path $destination | Out-Null
    Get-ChildItem -LiteralPath $RuntimeSource | Copy-Item -Destination $destination -Recurse -Force
}
Write-Output "Pluto runtime and licenses copied to $destination"
