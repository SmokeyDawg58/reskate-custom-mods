param([Parameter(Mandatory=$true)][string]$GameDirectory,[string]$PackageDirectory=$PSScriptRoot,[string]$OutputPath=(Join-Path (Get-Location).Path 'playtest-preflight.json'))
$ErrorActionPreference='Stop'
$manifest=Get-Content -LiteralPath (Join-Path $PackageDirectory 'release-manifest.json') -Raw | ConvertFrom-Json
$game=(Resolve-Path -LiteralPath $GameDirectory).Path
$modData=Join-Path $game 'ModData'
$data=if(Test-Path -LiteralPath $modData -PathType Container){Join-Path $modData 'Default'}else{$game}
if(-not $manifest.files.PSObject.Properties['ReSkate.dll'] -or -not $manifest.files.PSObject.Properties['ReSkateLauncher.exe'] -or $manifest.native_assets.file_count -ne 17 -or @($manifest.native_assets.integrity_files).Count -ne 17 -or -not $manifest.supported_game.PSObject.Properties['Skate.exe'] -or -not $manifest.supported_game.PSObject.Properties['steam_api64.dll']) {throw 'Incomplete release manifest.'}
$issues=New-Object 'System.Collections.Generic.List[string]'
$checks=@()
foreach($group in @($manifest.files,$manifest.supported_game)) {
    foreach($entry in $group.PSObject.Properties) {
        $relative=$entry.Name
        # Mod managers may rewrite descriptive metadata. Native data and the
        # runtime must match exactly; harmless author/readme changes may differ.
        if($relative.StartsWith('Mods/') -and $relative -notin $manifest.native_assets.integrity_files){continue}
        if([IO.Path]::IsPathRooted($relative) -or $relative.Contains(':') -or @($relative -split '[\\/]' | Where-Object {$_ -eq '..' -or $_ -eq '.'}).Count) {throw 'Invalid release path.'}
        $base=if($relative.StartsWith('Mods/')){$data}else{$game}
        $path=Join-Path $base $relative.Replace('/',[IO.Path]::DirectorySeparatorChar)
        $actual=if(Test-Path -LiteralPath $path -PathType Leaf){(Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant()}else{''}
        $matches=$actual -eq $entry.Value.sha256
        if(-not $matches){$issues.Add('Missing or different file: '+$relative)}
        $checks+=@{file=$relative;matches=$matches;sha256=$actual}
    }
}
$settingsPath=Join-Path $game 'ReSkateLauncher.settings.json'
if(Test-Path -LiteralPath $settingsPath) {
    $settings=Get-Content -LiteralPath $settingsPath -Raw | ConvertFrom-Json
    if($settings.updates -eq $true){$issues.Add('Turn off launcher auto updates for this pinned playtest build.')}
    if($settings.offline -eq $true){$issues.Add('Turn off launcher offline mode before multiplayer testing.')}
}else{$issues.Add('Complete normal ReSkate launcher setup before testing.')}
$result=@{release=$manifest.release_tag;result=if($issues.Count -eq 0){'PASS'}else{'ACTION_NEEDED'};checked_at_utc=(Get-Date).ToUniversalTime().ToString('o');files=$checks;issues=@($issues);game_or_settings_changed=$false;active_data_directory=$data;diagnostic_logging_in_build=$false}
$result | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $OutputPath -Encoding utf8
Write-Output ('Preflight: '+$result.result)
foreach($issue in $issues){Write-Output $issue}
Write-Output ('Saved '+$OutputPath)
if($issues.Count -gt 0){exit 1}
