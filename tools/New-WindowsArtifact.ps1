param(
    [Parameter(Mandatory)][ValidatePattern('^[0-9A-Za-z][0-9A-Za-z._-]+$')][string]$Version,
    [string]$Package = ''
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$package = if ($Package) { $Package } else { Join-Path $root 'dist-custom' }
$output = Join-Path $root '.local\windows-artifacts'
$exe = Join-Path $package 'bin\wsjtx.exe'
if (!(Test-Path -LiteralPath $exe)) { throw 'Run Package-Custom.ps1 first.' }
if ((Get-FileHash -LiteralPath $exe).Hash -ne (Get-FileHash -LiteralPath "$root\build-custom-make\wsjtx.exe").Hash) {
    throw 'Packaged EXE does not match the build output.'
}
$commit = (& git -C $root rev-parse HEAD | Out-String).Trim()
if ($LASTEXITCODE) { throw 'Cannot read source commit.' }
New-Item -ItemType Directory -Force -Path $output | Out-Null
$utf8 = [Text.UTF8Encoding]::new($false)
# A running station may still hold a renamed EXE from an earlier update.
# Stream the ZIP without a second package tree and omit those stale binaries.
Add-Type -AssemblyName System.IO.Compression.FileSystem
function Write-PortableZip([string]$Destination, [string]$RootDirectory, [string[]]$Inputs) {
    $base = [IO.Path]::GetFullPath($RootDirectory).TrimEnd('\','/')
    if (Test-Path -LiteralPath $Destination) { Remove-Item -LiteralPath $Destination -Force }
    $archive = [IO.Compression.ZipFile]::Open($Destination, [IO.Compression.ZipArchiveMode]::Create)
    try {
        foreach ($inputPath in $Inputs) {
            $item = Get-Item -LiteralPath $inputPath
            $files = if ($item.PSIsContainer) { Get-ChildItem -LiteralPath $item.FullName -Recurse -File } else { @($item) }
            foreach ($file in $files) {
                if ($file.Name -like '*-before-update-*.exe') { continue }
                if (!$file.FullName.StartsWith($base + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) { throw 'ZIP input outside package.' }
                $name = $file.FullName.Substring($base.Length + 1).Replace('\','/')
                [IO.Compression.ZipFileExtensions]::CreateEntryFromFile($archive, $file.FullName, $name, [IO.Compression.CompressionLevel]::Optimal) | Out-Null
            }
        }
    } finally { $archive.Dispose() }
}
# This launcher also works on a clean computer: WSJT-X creates the Custom profile.
# No personal configuration, callsign, logbook, or repository paths are shipped.
[IO.File]::WriteAllText((Join-Path $package 'Start-WSJT-X.bat'), "@echo off`r`ncd /d `"%~dp0bin`"`r`nstart `"WSJT-X OE3SRO`" wsjtx.exe --rig-name Custom`r`n", $utf8)
$instructions = @"
WSJT-X OE3SRO Custom $Version (Windows x64)
build by OE3SRO

Extract the entire ZIP, then double-click Start-WSJT-X.bat.
Keep bin and share together; the EXE requires the bundled DLLs and data.
For the separate HackRF One profile, use start-hackrf.bat; see HACKRF-WSJTX.txt.
HackRF runtime DLLs are not bundled by CI; see HACKRF-WSJTX.txt for setup.
REIST16-QSO/2: experimental 15-second CQ/QSO mode; select Mode > REIST16.
Update both endpoints. REIST16 is a separate protocol, incompatible with FT8.
For browser reception, see WEBSDR-WSJTX.txt and docs/index.html.
On first launch, configure your station and radio in Settings.
Existing WSJT-X - Custom profiles are reused without importing other profiles.

Source commit: $commit
Source and GPLv3 license: https://github.com/rudolfstepan/wsjtx

Deutsche und englische Anleitung / German and English guide: docs/index.html
"@
[IO.File]::WriteAllText((Join-Path $package 'README-Windows.txt'), $instructions, $utf8)
$manifest = [ordered]@{
    version = $Version
    branding = 'build by OE3SRO'
    platform = 'windows-x64'
    commit = $commit
    sourceDirty = [bool](& git -C $root status --porcelain --untracked-files=normal)
    buildOrigin = if ($env:GITHUB_ACTIONS -eq 'true') { 'github-actions' } else { 'local' }
    builtAtUtc = [DateTime]::UtcNow.ToString('o')
    experimentalMode = 'REIST16-QSO/2'
    plutoBridge = (Test-Path -LiteralPath (Join-Path $package 'bin\pluto\libiio.dll'))
    hackrfBridge = (Test-Path -LiteralPath (Join-Path $package 'bin\hackrf\hackrf-0.dll'))
    run = $env:GITHUB_RUN_ID
    exe = 'bin/wsjtx.exe'
    exeSha256 = (Get-FileHash -LiteralPath $exe).Hash.ToLowerInvariant()
}
[IO.File]::WriteAllText((Join-Path $package 'build-info.json'), ($manifest | ConvertTo-Json), $utf8)
$docs = Join-Path $package 'docs'
New-Item -ItemType Directory -Force -Path $docs | Out-Null
Copy-Item -LiteralPath "$root\docs\REIST16.txt" -Destination $docs -Force
Copy-Item -LiteralPath "$root\docs\PLUTO-WSJTX.txt" -Destination $docs -Force
Copy-Item -LiteralPath "$root\docs\HACKRF-WSJTX.txt" -Destination $docs -Force
Copy-Item -LiteralPath "$root\docs\WEBSDR-WSJTX.txt" -Destination $docs -Force
Copy-Item -LiteralPath "$root\docs\WEB-CONTROL.txt" -Destination $docs -Force
Copy-Item -LiteralPath "$root\docs\releases" -Destination $docs -Recurse -Force
New-Item -ItemType Directory -Force -Path "$docs\validation" | Out-Null
Copy-Item -LiteralPath "$root\docs\validation\reist16-multisignal" -Destination "$docs\validation" -Recurse -Force
Copy-Item -LiteralPath "$root\docs\WINDOWS-QUICKSTART.html" -Destination "$docs\index.html" -Force
New-Item -ItemType Directory -Force -Path "$docs\images" | Out-Null
Copy-Item -LiteralPath "$root\docs\images\ft2-qso-oe3sro-og5o-2026-10-16.png" -Destination "$docs\images" -Force
Copy-Item -LiteralPath "$root\docs\images\REIST16_QSO.png" -Destination "$docs\images" -Force
Copy-Item -LiteralPath "$root\docs\images\web-control-ft8-2026-10-25.png" -Destination "$docs\images" -Force
Copy-Item -LiteralPath "$root\docs\images\web-control-ft8-en-2026-10-25.png" -Destination "$docs\images" -Force
foreach ($document in @('FT2.md', 'FT8.md', 'REIST16.md', 'REIST16-QSO2.md', 'CHANGES.md')) {
    $content = [IO.File]::ReadAllText("$root\docs\$document")
    # Repository source links should also work from an extracted binary package.
    $content = [regex]::Replace($content, '\]\(\.\./([^)]+)\)', [Text.RegularExpressions.MatchEvaluator]{ param($match)
        $target = $match.Groups[1].Value
        $kind = if (Test-Path -LiteralPath (Join-Path $root $target.Split('#')[0]) -PathType Container) { 'tree' } else { 'blob' }
        return "](https://github.com/rudolfstepan/wsjtx/$kind/$commit/$target)"
    })
    [IO.File]::WriteAllText((Join-Path $docs $document), $content, $utf8)
}
$name = "wsjtx-OE3SRO-$Version-windows-x64"
$zip = Join-Path $output "$name.zip"
Write-PortableZip $zip $package @($package)
$hash = (Get-FileHash -LiteralPath $zip).Hash.ToLowerInvariant()
[IO.File]::WriteAllText((Join-Path $output "$name.sha256"), "$hash  $name.zip`n", $utf8)
$docsName = "wsjtx-OE3SRO-$Version-documentation"
$docsZip = Join-Path $output "$docsName.zip"
Write-PortableZip $docsZip $package @($docs, (Join-Path $package 'COPYING'), (Join-Path $package 'README-Windows.txt'), (Join-Path $package 'build-info.json'))
$docsHash = (Get-FileHash -LiteralPath $docsZip).Hash.ToLowerInvariant()
[IO.File]::WriteAllText((Join-Path $output "$docsName.sha256"), "$docsHash  $docsName.zip`n", $utf8)
Write-Output "Windows artifact ready: $zip"
