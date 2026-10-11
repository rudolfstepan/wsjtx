$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$ini = Join-Path $root '.local\installed-configuration\WSJT-X.ini'
$entry = Get-Content -LiteralPath $ini | Where-Object { $_ -match '^MyCall=' } | Select-Object -First 1
if (!$entry) { throw 'No callsign found in the imported configuration.' }
$call = [uri]::EscapeDataString($entry.Substring(7).Trim())
$url = "https://retrieve.pskreporter.info/query?senderCallsign=$call&mode=FT8&flowStartSeconds=-1800&rptlimit=100&rronly=1&noactive=1"
$response = Invoke-WebRequest -Uri $url -TimeoutSec 30
$xml = [xml]$response.Content
if ($xml.DocumentElement.Name -ne 'receptionReports') { throw 'Unexpected PSK Reporter XML root.' }
$out = Join-Path $root '.local\psk-reporter-live.xml'
[IO.File]::WriteAllText($out, $response.Content)
Write-Output "PSK Reporter HTTPS API reachable; XML root verified; $(@($xml.receptionReports.receptionReport).Where({ $null -ne $_ }).Count) reception reports."
Write-Output 'The private response was stored in .local/psk-reporter-live.xml.'
