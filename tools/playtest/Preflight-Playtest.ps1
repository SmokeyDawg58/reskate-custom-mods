param([Parameter(Mandatory=$true)][string]$GameDirectory,[string]$PackageDirectory=$PSScriptRoot,[string]$OutputPath=(Join-Path (Get-Location).Path 'playtest-preflight.json'))
$ErrorActionPreference='Stop'
$manifest=Get-Content -LiteralPath (Join-Path $PackageDirectory 'release-manifest.json') -Raw | ConvertFrom-Json
$game=(Resolve-Path -LiteralPath $GameDirectory).Path
$modData=Join-Path $game 'ModData'
$data=if(Test-Path -LiteralPath $modData -PathType Container){Join-Path $modData 'Default'}else{$game}
if(-not $manifest.files.PSObject.Properties['ReSkate.dll'] -or -not $manifest.files.PSObject.Properties['ReSkateLauncher.exe'] -or $manifest.native_assets.file_count -ne 17 -or @($manifest.native_assets.integrity_files).Count -ne 19 -or -not $manifest.supported_game.PSObject.Properties['Skate.exe'] -or -not $manifest.supported_game.PSObject.Properties['steam_api64.dll']) {throw 'Incomplete release manifest.'}
$issues=New-Object 'System.Collections.Generic.List[string]'
$checks=@()
$assetPrefix='Mods/SmokeyDawg58-ReSkate_Throwdown_Assets/'
$assetChecks=if($manifest.native_asset_files){$manifest.native_asset_files}else{$manifest.files}
foreach($nativeFile in $manifest.native_assets.integrity_files){if(-not $assetChecks.PSObject.Properties[$nativeFile]){throw 'Missing native asset checksum.'}}
$mods=Join-Path $data 'Mods'
$assetDirectory=Join-Path $mods 'SmokeyDawg58-ReSkate_Throwdown_Assets'
# Match the runtime: prefer the canonical installation, otherwise accept a
# version suffix produced by Windows Extract All or a mod manager. Verify all
# native files before choosing a versioned candidate. Never search recursively.
if(-not (Test-Path -LiteralPath $assetDirectory) -and (Test-Path -LiteralPath $mods -PathType Container)) {
    $candidates=@(Get-ChildItem -LiteralPath $mods -Directory | Where-Object {$_.Name.Length -le 64 -and $_.Name -match '^SmokeyDawg58-ReSkate_Throwdown_Assets-[0-9]+\.[0-9]+\.[0-9]+$'} | Sort-Object Name)
    if($candidates.Count -gt 0) {$assetDirectory=$candidates[0].FullName}
    foreach($candidate in $candidates) {
        $valid=$true
        foreach($relative in $manifest.native_assets.integrity_files) {
            if(-not $relative.StartsWith($assetPrefix) -or $relative.Contains(':') -or @($relative -split '[\\/]' | Where-Object {$_ -eq '..' -or $_ -eq '.'}).Count) {throw 'Invalid native asset path.'}
            $entry=$assetChecks.PSObject.Properties[$relative]
            if(-not $entry){throw 'Missing native asset checksum.'}
            $path=Join-Path $candidate.FullName $relative.Substring($assetPrefix.Length).Replace('/',[IO.Path]::DirectorySeparatorChar)
            if(-not (Test-Path -LiteralPath $path -PathType Leaf) -or (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash -ne $entry.Value.sha256) {$valid=$false;break}
        }
        if($valid){$assetDirectory=$candidate.FullName;break}
    }
}
$groups=@($manifest.files,$manifest.supported_game)
if($manifest.native_asset_files){$groups+=@($manifest.native_asset_files)}
foreach($group in $groups) {
    foreach($entry in $group.PSObject.Properties) {
        $relative=$entry.Name
        # Mod managers may rewrite descriptive metadata. Native data and the
        # runtime must match exactly; harmless author/readme changes may differ.
        if($relative.StartsWith('Mods/') -and $relative -notin $manifest.native_assets.integrity_files){continue}
        if([IO.Path]::IsPathRooted($relative) -or $relative.Contains(':') -or @($relative -split '[\\/]' | Where-Object {$_ -eq '..' -or $_ -eq '.'}).Count) {throw 'Invalid release path.'}
        $base=if($relative.StartsWith('Mods/')){$data}else{$game}
        $path=Join-Path $base $relative.Replace('/',[IO.Path]::DirectorySeparatorChar)
        if($relative.StartsWith($assetPrefix)){$path=Join-Path $assetDirectory $relative.Substring($assetPrefix.Length).Replace('/',[IO.Path]::DirectorySeparatorChar)}
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
$result=@{release=$manifest.release_tag;result=if($issues.Count -eq 0){'PASS'}else{'ACTION_NEEDED'};checked_at_utc=(Get-Date).ToUniversalTime().ToString('o');files=$checks;issues=@($issues);game_or_settings_changed=$false;active_data_directory=$data;asset_directory=$assetDirectory;diagnostic_logging_in_build=$false}
$result | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $OutputPath -Encoding utf8
Write-Output ('Preflight: '+$result.result)
foreach($issue in $issues){Write-Output $issue}
Write-Output ('Saved '+$OutputPath)
if($issues.Count -gt 0){exit 1}
