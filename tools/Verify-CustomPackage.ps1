param([switch]$PortableOnly,[string]$Package = '')
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$packageRoot = if ($Package) { $Package } else { Join-Path $root 'dist-custom\OE3SRO-reist16-pluto' }
$bin = Join-Path $packageRoot 'bin'
$checker = Join-Path $bin 'pskreporter-runtime-check.exe'
$previousPath = $env:PATH
Copy-Item -LiteralPath "$root\build-custom-make\pskreporter-checks\pskreporter_checks.exe" -Destination $checker -Force
try {
    # Verify that the package works without the development tools on PATH.
    $env:PATH = "$env:WINDIR\System32;$env:WINDIR"
    & $checker --package-check
    if ($LASTEXITCODE) { throw 'Packaged runtime / HTTPS check failed.' }
    & "$bin\wsjtx_app_version.exe"
    if ($LASTEXITCODE) { throw 'Packaged version tool failed.' }
    if ($PortableOnly) {
        Write-Output 'Portable runtime verified without an installed WSJT-X or personal configuration.'
        return
    }
    $snapshot = "$root\.local\installed-configuration\WSJT-X.ini"
    $profile = "$env:LOCALAPPDATA\WSJT-X - Custom\WSJT-X - Custom.ini"
    if ((Get-FileHash -LiteralPath $snapshot).Hash -eq (Get-FileHash -LiteralPath $profile).Hash) {
        Write-Output 'Copied configuration remains byte-for-byte identical.'
    } else {
        Write-Output 'Custom profile has been updated since import; its current settings were preserved.'
    }
    $installed = (Get-Item -LiteralPath 'C:\WSJT\wsjtx\bin\wsjtx.exe').VersionInfo
    Write-Output "Installed version: $($installed.ProductVersion)"
} finally {
    $env:PATH = $previousPath
    if (Test-Path -LiteralPath $checker) { Remove-Item -LiteralPath $checker }
}
