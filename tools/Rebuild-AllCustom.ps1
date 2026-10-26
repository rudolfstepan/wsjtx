param([string]$BuildDirectory = '', [switch]$Incremental)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$build = if ($BuildDirectory) { $BuildDirectory } else { Join-Path $root 'build-custom-make' }
$dist = Join-Path $root 'dist-custom'
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$logs = Join-Path $root "tmp/rebuild-all-$stamp"
New-Item -ItemType Directory -Force -Path $logs | Out-Null
New-Item -ItemType Directory -Force -Path $dist | Out-Null
$packages = @($dist)
Write-Output "Build and update of $($packages.Count) existing WSJT-X packages (clean: $(!$Incremental)). Logs: $logs"
& (Join-Path $PSScriptRoot 'Build-Custom.ps1') -Generator 'MinGW Makefiles' -BuildDirectory $build -Clean:(!$Incremental) *> (Join-Path $logs 'build.log')
if ($LASTEXITCODE) { throw 'Full build failed.' }
& (Join-Path $build 'wsjtx.exe') --check-web-resources *> (Join-Path $logs 'web-resources.txt')
if ($LASTEXITCODE) { throw 'Production EXE is missing web resources.' }
$names = @('wsjtx', 'jt9', 'wsprd', 'fmtave', 'fcal', 'fmeasure', 'wsjtx_app_version', 'wsjtx_pluto_bridge', 'wsjtx_hackrf_bridge')
$hashes = @{}
foreach ($name in $names) {
    $hashes[$name] = (Get-FileHash -LiteralPath (Join-Path $build "$name.exe") -Algorithm SHA256).Hash
}
$commit = (& git -C $root rev-parse HEAD | Out-String).Trim()
if ($LASTEXITCODE) { throw 'Cannot read source commit.' }
$dirty = [bool](& git -C $root status --porcelain --untracked-files=normal)
$version = (Get-Item -LiteralPath (Join-Path $build 'wsjtx.exe')).VersionInfo.ProductVersion
$results = @()
foreach ($package in $packages) {
    $name = Split-Path $package -Leaf
    Write-Output "Updating $package"
    & (Join-Path $PSScriptRoot 'Package-Custom.ps1') -BuildDirectory $build -Package $package *> (Join-Path $logs "package-$name.log")
    if ($LASTEXITCODE) { throw "Packaging failed: $package" }
    foreach ($binary in $names) {
        $file = Join-Path $package "bin/$binary.exe"
        if ((Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash -ne $hashes[$binary]) { throw "Build hash mismatch: $file" }
    }
    $info = [ordered]@{version=$version;branding='build by OE3SRO';platform='windows-x64';commit=$commit;sourceDirty=$dirty;
        builtAtUtc=[DateTime]::UtcNow.ToString('o');generator='MinGW Makefiles';exe='bin/wsjtx.exe';exeSha256=$hashes['wsjtx'].ToLowerInvariant();
        plutoBridge=(Test-Path -LiteralPath (Join-Path $package 'bin/pluto/libiio.dll'));
        hackrfBridge=(Test-Path -LiteralPath (Join-Path $package 'bin/hackrf/hackrf-0.dll'))}
    [IO.File]::WriteAllText((Join-Path $package 'build-info.json'), ($info | ConvertTo-Json), [Text.UTF8Encoding]::new($false))
    $results += [pscustomobject]@{package=$package;exeSha256=$hashes['wsjtx'];binariesVerified=$names.Count}
}
$results | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $logs 'packages.json') -Encoding UTF8
Write-Output "All $($results.Count) packages updated; all $($names.Count) production binaries per package match the clean build."
Write-Output "EXE SHA256: $($hashes['wsjtx'])"
