param([Parameter(Mandatory=$true)][string]$GameDirectory,[string]$OutputDirectory=(Join-Path (Get-Location).Path ('PlaytestReport-'+(Get-Date -Format 'yyyyMMdd-HHmmss-fff'))))
$ErrorActionPreference='Stop'
$game=(Resolve-Path -LiteralPath $GameDirectory).Path
New-Item -ItemType Directory -Path $OutputDirectory -ErrorAction Stop | Out-Null
$output=(Resolve-Path -LiteralPath $OutputDirectory).Path
$files=@()
foreach ($file in @('ReSkate.dll','ReSkateLauncher.exe','Skate.exe','steam_api64.dll')) {
    $path=Join-Path $game $file
    if (Test-Path -LiteralPath $path -PathType Leaf) { $files+=@{name=$file;sha256=(Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant();bytes=(Get-Item -LiteralPath $path).Length} }
}
$log=Join-Path $game 'logs\ReSkate.log'
if (Test-Path -LiteralPath $log -PathType Leaf) { Copy-Item -LiteralPath $log -Destination (Join-Path $output 'ReSkate.log') -ErrorAction Stop }
$mods=@()
$modSettings=Join-Path $game 'Mods\mods.json'
if (Test-Path -LiteralPath $modSettings) {
    $catalog=Get-Content -LiteralPath $modSettings -Raw | ConvertFrom-Json
    $mods=@($catalog.mods | Where-Object {$_.enabled} | ForEach-Object {$_.name})
}
@{collected_at_utc=(Get-Date).ToUniversalTime().ToString('o');files=$files;enabled_mods=$mods;programs_controlled=$false;uploaded=$false} | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $output 'system-and-build.json') -Encoding utf8
'Describe the host/guest role, map, challenge, player count, exact steps, expected result and actual result. Add screenshots or a short recording. Review logs before sharing: they can contain player names and Steam IDs. No credentials/settings files or crash memory dumps were collected.' | Set-Content -LiteralPath (Join-Path $output 'ADD-YOUR-NOTES.txt') -Encoding utf8
Write-Output ('Saved local report: '+$output)
Write-Output 'Nothing was uploaded and no programs were controlled.'
