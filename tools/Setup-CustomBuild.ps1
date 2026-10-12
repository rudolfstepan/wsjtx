$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$deps = Join-Path $root '.tools\hamlib'
New-Item -ItemType Directory -Force -Path $deps | Out-Null
$archive = Join-Path $deps 'hamlib-w64-4.7.2.zip'
if (!(Test-Path -LiteralPath $archive)) {
    Invoke-WebRequest -Uri 'https://github.com/Hamlib/Hamlib/releases/download/4.7.2/hamlib-w64-4.7.2.zip' -OutFile $archive
}
if ((Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash -ne '8553bc6c5c6032e8debf99c017e98f58fed7e07e7c25d04815dc3e8bbe3304c7') {
    throw 'Hamlib archive checksum mismatch.'
}
Expand-Archive -LiteralPath $archive -DestinationPath $deps -Force
Write-Output 'Hamlib SDK extracted and SHA256 verified.'
$omnirig = Join-Path $root '.tools\OmniRig.tlb'
if (!(Test-Path -LiteralPath $omnirig)) {
    Invoke-WebRequest -Uri 'https://raw.githubusercontent.com/VE3NEA/OmniRig/e2f5d9ba7f0d7a3563e49fa2f3de417bfc8c749f/OmniRig.tlb' -OutFile $omnirig
}
if ((Get-FileHash -LiteralPath $omnirig -Algorithm SHA256).Hash -ne '9B050DD111EA9B181A99BE1732CBDB1A5572C752A8EDFE5A73F704BD4AF7E66E') {
    throw 'OmniRig type library checksum mismatch.'
}
Write-Output 'OmniRig type library available and SHA256 verified.'
