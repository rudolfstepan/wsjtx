param(
    [Parameter(Mandatory)][ValidatePattern('^[0-9A-Za-z][0-9A-Za-z._-]+$')][string]$Version
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$package = Join-Path $root 'dist-custom\OE3SRO-psk-fix'
$output = Join-Path $root '.local\windows-artifacts'
$exe = Join-Path $package 'bin\wsjtx.exe'
if (!(Test-Path -LiteralPath $exe)) { throw 'Run Package-Custom.ps1 first.' }
if ((Get-FileHash -LiteralPath $exe).Hash -ne (Get-FileHash -LiteralPath "$root\build-custom\wsjtx.exe").Hash) {
    throw 'Packaged EXE does not match the build output.'
}
$commit = (& git -C $root rev-parse HEAD | Out-String).Trim()
if ($LASTEXITCODE) { throw 'Cannot read source commit.' }
New-Item -ItemType Directory -Force -Path $output | Out-Null
$utf8 = [Text.UTF8Encoding]::new($false)
# This launcher also works on a clean computer: WSJT-X creates the Custom profile.
# No personal configuration, callsign, logbook, or repository paths are shipped.
[IO.File]::WriteAllText((Join-Path $package 'Start-WSJT-X.bat'), "@echo off`r`ncd /d `"%~dp0bin`"`r`nstart `"WSJT-X OE3SRO`" wsjtx.exe --rig-name Custom`r`n", $utf8)
$instructions = @"
WSJT-X OE3SRO Custom $Version (Windows x64)
build by OE3SRO

Extract the entire ZIP, then double-click Start-WSJT-X.bat.
Keep bin and share together; the EXE requires the bundled DLLs and data.
On first launch, configure your station and radio in Settings.
Existing WSJT-X - Custom profiles are reused without importing other profiles.

Source commit: $commit
Source and GPLv3 license: https://github.com/rudolfstepan/wsjtx
"@
[IO.File]::WriteAllText((Join-Path $package 'README-Windows.txt'), $instructions, $utf8)
$manifest = [ordered]@{
    version = $Version
    branding = 'build by OE3SRO'
    platform = 'windows-x64'
    commit = $commit
    run = $env:GITHUB_RUN_ID
    exe = 'bin/wsjtx.exe'
    exeSha256 = (Get-FileHash -LiteralPath $exe).Hash.ToLowerInvariant()
}
[IO.File]::WriteAllText((Join-Path $package 'build-info.json'), ($manifest | ConvertTo-Json), $utf8)
$name = "wsjtx-OE3SRO-$Version-windows-x64"
$zip = Join-Path $output "$name.zip"
Compress-Archive -Path (Join-Path $package '*') -DestinationPath $zip -CompressionLevel Optimal -Force
$hash = (Get-FileHash -LiteralPath $zip).Hash.ToLowerInvariant()
[IO.File]::WriteAllText((Join-Path $output "$name.sha256"), "$hash  $name.zip`n", $utf8)
Write-Output "Windows artifact ready: $zip"
