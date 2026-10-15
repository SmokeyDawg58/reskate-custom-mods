param(
    [Parameter(Mandatory=$true)][string]$GameDirectory,
    [string]$PackageDirectory=$PSScriptRoot,
    [string]$OutputPath=(Join-Path (Get-Location).Path 'playtest-preflight.json')
)
$ErrorActionPreference='Stop'
$manifest=Get-Content -LiteralPath (Join-Path $PackageDirectory 'release-manifest.json') -Raw | ConvertFrom-Json
$game=(Resolve-Path -LiteralPath $GameDirectory).Path
$issues=New-Object 'System.Collections.Generic.List[string]'
$checks=@()
foreach ($required in @('ReSkate.dll','ReSkateLauncher.exe')) {
    if (-not $manifest.files -or -not $manifest.files.PSObject.Properties[$required]) { throw ('Incomplete release manifest: '+$required) }
}
foreach ($required in @('Skate.exe','steam_api64.dll')) {
    if (-not $manifest.supported_game -or -not $manifest.supported_game.PSObject.Properties[$required]) { throw ('Incomplete release manifest: '+$required) }
}
foreach ($group in @($manifest.files,$manifest.supported_game)) {
    foreach ($entry in $group.PSObject.Properties) {
        if ($entry.Name -notin @('ReSkate.dll','ReSkateLauncher.exe','Skate.exe','steam_api64.dll')) { continue }
        $path=Join-Path $game $entry.Name
        $actual=if (Test-Path -LiteralPath $path -PathType Leaf) { (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant() } else { '' }
        $matches=$actual -eq $entry.Value.sha256
        if (-not $matches) { $issues.Add('Missing or different build: '+$entry.Name) }
        $checks+=@{file=$entry.Name;matches=$matches;sha256=$actual}
    }
}
$settingsPath=Join-Path $game 'ReSkateLauncher.settings.json'
$updates=$null
$offline=$null
if (Test-Path -LiteralPath $settingsPath) {
    $settings=Get-Content -LiteralPath $settingsPath -Raw | ConvertFrom-Json
    if ($settings.PSObject.Properties['updates']) { $updates=[bool]$settings.updates }
    if ($settings.PSObject.Properties['offline']) { $offline=[bool]$settings.offline }
    if ($updates -eq $true) { $issues.Add('Turn off launcher auto updates for this pinned playtest build.') }
    if ($offline -eq $true) { $issues.Add('Turn off launcher offline mode before the multiplayer test.') }
} else { $issues.Add('Complete the normal ReSkate launcher setup before this test.') }
$mapCatalogs=@()
$modsPath=Join-Path $game 'Mods'
if (Test-Path -LiteralPath $modsPath -PathType Container) {
    foreach ($folder in Get-ChildItem -LiteralPath $modsPath -Directory) {
        $catalog=Join-Path $folder.FullName 'reskate-levels.json'
        if (Test-Path -LiteralPath $catalog -PathType Leaf) {
            $mapCatalogs+=@{package=$folder.Name;catalog_sha256=(Get-FileHash -LiteralPath $catalog -Algorithm SHA256).Hash.ToLowerInvariant()}
        }
    }
}
$result=@{release=$manifest.release_tag;result=if($issues.Count -eq 0){'PASS'}else{'ACTION_NEEDED'};checked_at_utc=(Get-Date).ToUniversalTime().ToString('o');files=$checks;launcher_auto_updates=$updates;launcher_offline=$offline;map_catalogs=$mapCatalogs;issues=@($issues);game_or_settings_changed=$false}
$result | ConvertTo-Json -Depth 7 | Set-Content -LiteralPath $OutputPath -Encoding utf8
Write-Output ('Preflight: '+$result.result)
foreach ($issue in $issues) { Write-Output $issue }
Write-Output ('Saved '+$OutputPath)
if ($issues.Count -gt 0) { exit 1 }
