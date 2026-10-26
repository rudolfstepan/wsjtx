param([string]$Package = '',[string]$BuildDirectory = '')
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$mingw = if ($env:WSJTX_MINGW_PREFIX) { $env:WSJTX_MINGW_PREFIX } else { 'C:\msys64\mingw64' }
$env:PATH = "$mingw\bin;$env:PATH"
$build = if ($BuildDirectory) { $BuildDirectory } else { Join-Path $root 'build-custom-make' }
$package = if ($Package) { $Package } else { Join-Path $root 'dist-custom\OE3SRO-reist16-pluto' }
$bin = Join-Path $package 'bin'
$data = Join-Path $package 'share\wsjtx'
$sdkBin = Join-Path $root '.tools\hamlib\hamlib-w64-4.7.2\bin'
New-Item -ItemType Directory -Force -Path $bin, $data | Out-Null
Copy-Item -LiteralPath (Join-Path $root 'docs/WEB-CONTROL.txt') -Destination $package -Force
New-Item -ItemType Directory -Force -Path (Join-Path $package 'images') | Out-Null
Copy-Item -LiteralPath (Join-Path $root 'docs/images/web-control-ft8-2026-10-25.png') -Destination (Join-Path $package 'images') -Force
Copy-Item -LiteralPath (Join-Path $root 'docs/images/web-control-ft8-en-2026-10-25.png') -Destination (Join-Path $package 'images') -Force
foreach ($name in @('wsjtx', 'jt9', 'wsprd', 'fmtave', 'fcal', 'fmeasure', 'wsjtx_app_version', 'wsjtx_pluto_bridge', 'wsjtx_hackrf_bridge')) {
    $source = Join-Path $build "$name.exe"
    if (!(Test-Path -LiteralPath $source)) { throw "Missing build output: $source" }
    $destination = Join-Path $bin "$name.exe"
    try { [IO.File]::Copy([IO.Path]::GetFullPath($source), [IO.Path]::GetFullPath($destination), $true) }
    catch [IO.IOException] {
        if (($_.Exception.HResult -band 0xffff) -notin @(32,33)) { throw }
        # Keep running radio/decoder processes alive; Windows permits renaming
        # their loaded EXEs. The replacement takes effect at the next launch.
        $backup = Join-Path $bin ("$name-before-update-" + (Get-Date -Format 'yyyyMMdd-HHmmssfff') + '.exe')
        Move-Item -LiteralPath $destination -Destination $backup
        [IO.File]::Copy([IO.Path]::GetFullPath($source), [IO.Path]::GetFullPath($destination), $true)
        Write-Output "Running executable retained as $backup; restart the application to use the update."
    }
}
foreach ($name in @('rigctl', 'rigctld', 'rigctlcom')) {
    Copy-Item -LiteralPath (Join-Path $sdkBin "$name.exe") -Destination (Join-Path $bin "$name-wsjtx.exe") -Force
}
foreach ($name in @('cty.dat', 'cty.dat_copyright.txt', 'grid.dat', 'sat.dat', 'eclipse.txt', 'ALLCALL7.TXT')) {
    Copy-Item -LiteralPath (Join-Path $root $name) -Destination $data -Force
}
Copy-Item -LiteralPath (Join-Path $root 'contrib\Ephemeris\JPLEPH') -Destination $data -Force
Copy-Item -LiteralPath (Join-Path $root 'sounds') -Destination $bin -Recurse -Force
Copy-Item -LiteralPath (Join-Path $root 'COPYING') -Destination $package -Force
Copy-Item -LiteralPath (Join-Path $root 'docs\REIST16.txt') -Destination $package -Force
Copy-Item -LiteralPath (Join-Path $root 'docs\REIST16-QSO2.md') -Destination $package -Force
$multisignalDocs = Join-Path $package 'validation/reist16-multisignal'
New-Item -ItemType Directory -Force -Path $multisignalDocs | Out-Null
Get-ChildItem -LiteralPath (Join-Path $root 'docs/validation/reist16-multisignal') -File |
    Where-Object { $_.Extension -in @('.md','.csv','.tsv','.json','.jsonl','.txt') } |
    Copy-Item -Destination $multisignalDocs -Force
Copy-Item -LiteralPath (Join-Path $root 'docs\PLUTO-WSJTX.txt') -Destination $package -Force
Copy-Item -LiteralPath (Join-Path $root 'Start-Pluto.ps1'),(Join-Path $root 'start-pluto.bat') -Destination $package -Force
Copy-Item -LiteralPath (Join-Path $root 'Start-WebSDR.ps1'),(Join-Path $root 'start-websdr.bat'),(Join-Path $root 'docs/WEBSDR-WSJTX.txt') -Destination $package -Force
Copy-Item -LiteralPath (Join-Path $root 'Start-Wasapi.ps1'),(Join-Path $root 'start-wasapi.bat') -Destination $package -Force
Copy-Item -LiteralPath (Join-Path $root 'Start-Yaesu-WebSDR.ps1'),(Join-Path $root 'start-yaesu-websdr.bat') -Destination $package -Force
Copy-Item -LiteralPath (Join-Path $root 'Start-Reist16-Selftest.ps1'),(Join-Path $root 'start-reist16-selftest.bat') -Destination $package -Force
Copy-Item -LiteralPath (Join-Path $root 'Start-HackRF.ps1'),(Join-Path $root 'start-hackrf.bat'),(Join-Path $root 'docs/HACKRF-WSJTX.txt') -Destination $package -Force
$hackrfRuntime=Join-Path $root '.tools/hackrf/runtime'
Copy-Item -LiteralPath (Join-Path $root 'Start-HackRF-FT8-17m.ps1'),(Join-Path $root 'start-hackrf-ft8-17m.bat') -Destination $package -Force
if(Test-Path -LiteralPath (Join-Path $hackrfRuntime 'hackrf-0.dll')){New-Item -ItemType Directory -Force -Path (Join-Path $bin 'hackrf') | Out-Null;Get-ChildItem -LiteralPath $hackrfRuntime | Copy-Item -Destination (Join-Path $bin 'hackrf') -Recurse -Force}
$plutoRuntime = Join-Path $root '.tools\pluto\runtime'
if (Test-Path -LiteralPath (Join-Path $plutoRuntime 'libiio.dll')) {
    New-Item -ItemType Directory -Force -Path (Join-Path $bin 'pluto') | Out-Null
    Get-ChildItem -LiteralPath $plutoRuntime | Copy-Item -Destination (Join-Path $bin 'pluto') -Recurse -Force
}
& "$mingw\bin\windeployqt-qt5.exe" --release --no-translations --no-angle --no-opengl-sw "$bin\wsjtx.exe"
if ($LASTEXITCODE) { throw 'Qt deployment failed.' }
# WSJT-X uses SQLite; the other optional database plugins require unbundled servers.
foreach ($name in @('qsqlibase.dll', 'qsqlmysql.dll', 'qsqlpsql.dll')) {
    $plugin = Join-Path $bin "sqldrivers\$name"
    if (Test-Path -LiteralPath $plugin) { Remove-Item -LiteralPath $plugin }
}
# OpenSSL is dynamically loaded by Qt and does not appear in PE imports.
foreach ($source in @(Get-ChildItem -LiteralPath "$mingw\bin" -Filter 'libssl-*.dll') + @(Get-ChildItem -LiteralPath "$mingw\bin" -Filter 'libcrypto-*.dll')) {
    $destination = Join-Path $bin $source.Name
    # An identical DLL can be in use by the running station; no replacement
    # is necessary. A genuinely different runtime still has to copy safely.
    if ((Test-Path -LiteralPath $destination) -and
        (Get-FileHash -LiteralPath $source.FullName).Hash -eq (Get-FileHash -LiteralPath $destination).Hash) { continue }
    Copy-Item -LiteralPath $source.FullName -Destination $destination -Force
}
# Resolve the full PE import closure, including Qt plugins and decoder tools.
$queue = [Collections.Generic.Queue[string]]::new()
Get-ChildItem -LiteralPath $bin -Recurse -File | Where-Object { $_.Extension -in '.exe', '.dll' } | ForEach-Object { $queue.Enqueue($_.FullName) }
$seen = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
while ($queue.Count) {
    $file = $queue.Dequeue()
    if (!$seen.Add($file)) { continue }
    $imports = & "$mingw\bin\objdump.exe" -p $file
    if ($LASTEXITCODE) { throw "Cannot inspect imports: $file" }
    foreach ($line in $imports) {
        if ($line -notmatch 'DLL Name:\s*(\S+)') { continue }
        $name = $Matches[1]
        # Pluto's vendor DLLs keep their own dependency directory, matching
        # LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR in the isolated bridge process.
        $localDependency = Join-Path (Split-Path $file) $name
        if (Test-Path -LiteralPath $localDependency) { $queue.Enqueue($localDependency); continue }
        $destination = Join-Path $bin $name
        if (Test-Path -LiteralPath $destination) { $queue.Enqueue($destination); continue }
        $source = @((Join-Path "$mingw\bin" $name), (Join-Path $sdkBin $name)) | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
        if ($source) {
            Copy-Item -LiteralPath $source -Destination $destination
            $queue.Enqueue($destination)
        } elseif (!(Test-Path -LiteralPath (Join-Path "$env:WINDIR\System32" $name)) -and $name -notmatch '^(api-ms-|ext-ms-)') {
            throw "Unresolved runtime dependency $name required by $file"
        }
    }
}
Write-Output "Portable custom package ready: $bin\wsjtx.exe"
Write-Output 'Launch with Start-Custom.ps1 to use the copied configuration.'
