$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$mingw = if ($env:WSJTX_MINGW_PREFIX) { $env:WSJTX_MINGW_PREFIX } else { 'C:\msys64\mingw64' }
$env:PATH = "$mingw\bin;$env:PATH"
$build = Join-Path $root 'build-custom'
$package = Join-Path $root 'dist-custom\OE3SRO-psk-fix'
$bin = Join-Path $package 'bin'
$data = Join-Path $package 'share\wsjtx'
$sdkBin = Join-Path $root '.tools\hamlib\hamlib-w64-4.7.2\bin'
New-Item -ItemType Directory -Force -Path $bin, $data | Out-Null
foreach ($name in @('wsjtx', 'jt9', 'wsprd', 'fmtave', 'fcal', 'fmeasure', 'wsjtx_app_version')) {
    $source = Join-Path $build "$name.exe"
    if (!(Test-Path -LiteralPath $source)) { throw "Missing build output: $source" }
    Copy-Item -LiteralPath $source -Destination $bin -Force
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
& "$mingw\bin\windeployqt-qt5.exe" --release --no-translations --no-angle --no-opengl-sw "$bin\wsjtx.exe"
if ($LASTEXITCODE) { throw 'Qt deployment failed.' }
# WSJT-X uses SQLite; the other optional database plugins require unbundled servers.
foreach ($name in @('qsqlibase.dll', 'qsqlmysql.dll', 'qsqlpsql.dll')) {
    $plugin = Join-Path $bin "sqldrivers\$name"
    if (Test-Path -LiteralPath $plugin) { Remove-Item -LiteralPath $plugin }
}
# OpenSSL is dynamically loaded by Qt and does not appear in PE imports.
Get-ChildItem -LiteralPath "$mingw\bin" -Filter 'libssl-*.dll' | Copy-Item -Destination $bin -Force
Get-ChildItem -LiteralPath "$mingw\bin" -Filter 'libcrypto-*.dll' | Copy-Item -Destination $bin -Force
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
