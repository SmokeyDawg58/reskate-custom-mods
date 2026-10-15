param([Parameter(Mandatory=$true)][string]$GameDirectory,[string]$PackageDirectory=$PSScriptRoot)
$ErrorActionPreference='Stop'
$game=(Resolve-Path -LiteralPath $GameDirectory).Path
$package=(Resolve-Path -LiteralPath $PackageDirectory).Path
$manifest=Get-Content -LiteralPath (Join-Path $package 'release-manifest.json') -Raw | ConvertFrom-Json
$modData=Join-Path $game 'ModData'
$data=if(Test-Path -LiteralPath $modData -PathType Container){Join-Path $modData 'Default'}else{$game}
function Assert-Closed {
    if (@(Get-CimInstance Win32_Process | Where-Object {$_.Name -match '^(Skate|ReSkateLauncher)\.exe$'}).Count) {
        throw 'Please fully close ReSkate and its launcher yourself, then run this installer again.'
    }
}
function Assert-Hash([string]$Path,[string]$Expected) {
    if ((Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant() -ne $Expected.ToLowerInvariant()) {throw ('Checksum mismatch: '+$Path)}
}
function Relative-Path([string]$Name) {
    if ([IO.Path]::IsPathRooted($Name) -or $Name.Contains(':') -or @($Name -split '[\\/]' | Where-Object {$_ -eq '..' -or $_ -eq '.' -or $_ -eq ''}).Count) {throw 'Invalid release path.'}
    if ($Name -notin @('ReSkate.dll','ReSkateLauncher.exe') -and -not $Name.StartsWith('Mods/SmokeyDawg58-ReSkate_Throwdown_Assets/')) {throw 'Unexpected release payload.'}
    return $Name.Replace('/',[IO.Path]::DirectorySeparatorChar)
}
function Target-Path([string]$Name) {
    $relative=Relative-Path $Name
    $base=if($Name.StartsWith('Mods/')){$data}else{$game}
    $target=[IO.Path]::GetFullPath((Join-Path $base $relative))
    if (-not $target.StartsWith($base.TrimEnd('\')+'\',[StringComparison]::OrdinalIgnoreCase)) {throw 'Destination leaves the game directory.'}
    return $target
}
if (-not $manifest.files.PSObject.Properties['ReSkate.dll'] -or -not $manifest.files.PSObject.Properties['ReSkateLauncher.exe'] -or $manifest.native_assets.file_count -ne 17) {throw 'Incomplete release manifest.'}
Assert-Closed
foreach($entry in $manifest.supported_game.PSObject.Properties) {Assert-Hash (Join-Path $game $entry.Name) $entry.Value.sha256}
$entries=@($manifest.files.PSObject.Properties)
foreach($entry in $entries) {
    $relative=Relative-Path $entry.Name
    Assert-Hash (Join-Path $package $relative) $entry.Value.sha256
    $null=Target-Path $entry.Name
}
$backup=Join-Path $game ('PlaytestBackups\'+(Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
New-Item -ItemType Directory -Path $backup -ErrorAction Stop | Out-Null
$previous=@{}
foreach($entry in $entries) {
    $target=Target-Path $entry.Name
    if(Test-Path -LiteralPath $target -PathType Leaf) {
        $previous[$entry.Name]=(Get-FileHash -LiteralPath $target -Algorithm SHA256).Hash.ToLowerInvariant()
        $saved=Join-Path $backup (Relative-Path $entry.Name)
        New-Item -ItemType Directory -Path ([IO.Path]::GetDirectoryName($saved)) -Force | Out-Null
        Copy-Item -LiteralPath $target -Destination $saved -ErrorAction Stop
        Assert-Hash $saved $previous[$entry.Name]
    }
}
$configuration=@{}
foreach($path in @((Join-Path $game 'launcher.json'),(Join-Path $game 'ReSkateLauncher.settings.json'),(Join-Path $game 'ReSkate.favorites.json'),(Join-Path $game 'Mods\mods.json'),(Join-Path $data 'Mods\mods.json'))) {
    if(Test-Path -LiteralPath $path -PathType Leaf) {$configuration[$path]=(Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash}
}
$attempted=New-Object 'System.Collections.Generic.List[string]'
try {
    foreach($entry in $entries) {
        Assert-Closed
        $target=Target-Path $entry.Name
        New-Item -ItemType Directory -Path ([IO.Path]::GetDirectoryName($target)) -Force | Out-Null
        $attempted.Add($entry.Name)
        Copy-Item -LiteralPath (Join-Path $package (Relative-Path $entry.Name)) -Destination $target -Force -ErrorAction Stop
        Assert-Hash $target $entry.Value.sha256
    }
} catch {
    $problem=$_
    Assert-Closed
    foreach($name in $attempted) {
        $target=Target-Path $name
        if($previous.ContainsKey($name)) {
            Copy-Item -LiteralPath (Join-Path $backup (Relative-Path $name)) -Destination $target -Force -ErrorAction Stop
            Assert-Hash $target $previous[$name]
        } elseif(Test-Path -LiteralPath $target -PathType Leaf) {
            $saved=Join-Path $backup ('failed-new\'+(Relative-Path $name))
            New-Item -ItemType Directory -Path ([IO.Path]::GetDirectoryName($saved)) -Force | Out-Null
            Move-Item -LiteralPath $target -Destination $saved -ErrorAction Stop
        }
    }
    throw $problem
}
foreach($path in $configuration.Keys) {Assert-Hash $path $configuration[$path]}
@{release=$manifest.release_tag;installed_at_utc=(Get-Date).ToUniversalTime().ToString('o');files=$manifest.files;previous_binary_and_asset_hashes=$previous;configuration_unchanged=$true;programs_controlled=$false;active_data_directory=$data} | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $backup 'installation-receipt.json') -Encoding utf8
Write-Output ('Installed '+$manifest.release_tag+'. Backup: '+$backup)
Write-Output ('Required asset mod: '+(Join-Path $data 'Mods\SmokeyDawg58-ReSkate_Throwdown_Assets'))
Write-Output 'Mods and settings preserved. Launch ReSkate yourself when ready.'
