param(
    [Parameter(Mandatory=$true)][string]$OutputDirectory,
    [ValidateRange(1,100)][int]$Trials=4,
    [uint32]$Seed=2026102204,
    [switch]$SkipBuild,
    [switch]$ChannelOnly,
    [switch]$SkipReference
)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$mingw=if($env:WSJTX_MINGW_PREFIX){$env:WSJTX_MINGW_PREFIX}else{'C:\msys64\mingw64'}
$oldPath=$env:PATH
try {
 $env:PATH="$mingw\bin;$env:PATH"
 $build=Join-Path $root 'build-custom'
 $output=[IO.Path]::GetFullPath((Join-Path $root $OutputDirectory))
 if(!$output.StartsWith("$build\",[StringComparison]::OrdinalIgnoreCase)){throw 'Output must be under build-custom.'}
 if(Test-Path -LiteralPath $output){throw 'Use a new output directory to preserve evidence.'}
 New-Item -ItemType Directory -Path $output | Out-Null
 if(!$SkipBuild){
  $testTargets=@(if($ChannelOnly){'reist16_qso2_channel_checks'}else{'reist16_qso2_checks';'jt9'})
  & "$mingw\bin\cmake.exe" --build $build --target @testTargets --parallel 6
  if($LASTEXITCODE){throw 'QSO/2 test build failed.'}
 }
 $sources=@('lib/reist16/qso2.cpp','lib/reist16/qso2.hpp','lib/reist16/reist16.cpp','lib/reist16/planner_lock.hpp','lib/ft8/native/ft8_tables.hpp','lib/ft8/native/ft8_ldpc.cpp','lib/ft8/native/ft8_ldpc_abi.cpp','lib/ft8/native/ft8_wave.cpp','lib/ft8/native/ft8_wave_abi.cpp','tests/reist16/qso2_checks.cpp')
 $sources | ForEach-Object { [pscustomobject]@{Path=$_;SHA256=(Get-FileHash -LiteralPath (Join-Path $root $_)).Hash} } | ConvertTo-Json | Set-Content -LiteralPath "$output\sources.json" -Encoding utf8
 Push-Location $output
 try {
  if($ChannelOnly){
   & "$build\reist16_qso2_channel_checks.exe" | Tee-Object -FilePath channel.txt
   if($LASTEXITCODE){throw 'QSO/2 channel controls failed.'}
   return
  }
  & "$build\reist16_qso2_checks.exe" --unit
  if($LASTEXITCODE){throw 'QSO/2 contract checks failed.'}
  & "$build\reist16_qso2_checks.exe" $Trials $Seed | Tee-Object -FilePath native.txt
  if($LASTEXITCODE){throw 'Native QSO/2 checks failed.'}
  if($SkipReference){return}
  $rows=Get-Content results.jsonl | ForEach-Object{$_|ConvertFrom-Json} | Where-Object{$_.PSObject.Properties.Name -contains 'message'}
  $measured=@()
  foreach($row in $rows) {
   $arguments=@('-8','-d','3','-L','100','-H','4000',$row.ft8_wave)
   $stem=[IO.Path]::GetFileNameWithoutExtension($row.ft8_wave)
   $watch=[Diagnostics.Stopwatch]::StartNew()
   $process=Start-Process -FilePath "$build\jt9.exe" -ArgumentList $arguments -WorkingDirectory $output -WindowStyle Hidden -PassThru -RedirectStandardOutput "$stem.txt" -RedirectStandardError "$stem.err"
   if(!$process.WaitForExit(30000)){$process.Kill();throw 'Reference decoder timeout.'}
   $process.WaitForExit()
   if($process.ExitCode){throw 'Reference decoder failed.'}
   $lines=Get-Content -LiteralPath "$stem.txt"
   $got=@($lines|Where-Object{$_ -match '^\d{6}\s' -and $_ -match '~'})
   $exact=@($got|Where-Object{$_.Contains($row.message)})
   $falseDecodes=@($got|Where-Object{!$_.Contains($row.message)})
   $row|Add-Member -NotePropertyName ft8 -NotePropertyValue ([int]($exact.Count -gt 0))
   $row|Add-Member -NotePropertyName ft8_false -NotePropertyValue $falseDecodes.Count
   $row|Add-Member -NotePropertyName ft8_wall_seconds -NotePropertyValue $watch.Elapsed.TotalSeconds
   $measured+=$row
   $row|ConvertTo-Json -Compress|Add-Content -LiteralPath reference.jsonl -Encoding utf8
  }
  $summary=@($measured|Group-Object snr2500|ForEach-Object{
   [pscustomobject]@{SNR2500=[int]$_.Name;Trials=$_.Count;QSO1=($_.Group|Measure-Object qso1 -Sum).Sum;QSO2=($_.Group|Measure-Object qso2 -Sum).Sum;FT8=($_.Group|Measure-Object ft8 -Sum).Sum;FT8False=($_.Group|Measure-Object ft8_false -Sum).Sum}
  })
  $summary|ConvertTo-Json|Set-Content -LiteralPath summary.json -Encoding utf8
  $summary|Format-Table
 } finally {Pop-Location}
} finally {$env:PATH=$oldPath}
