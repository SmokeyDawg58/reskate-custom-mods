param([Parameter(Mandatory=$true)][string]$GameDirectory,[string]$PackageDirectory=$PSScriptRoot)
$ErrorActionPreference='Stop'
$game=(Resolve-Path -LiteralPath $GameDirectory).Path
$package=(Resolve-Path -LiteralPath $PackageDirectory).Path
$manifest=Get-Content -LiteralPath (Join-Path $package 'release-manifest.json') -Raw | ConvertFrom-Json
$files=@('ReSkate.dll','ReSkateLauncher.exe')
function Assert-Closed {
    $running=@(Get-CimInstance Win32_Process | Where-Object {$_.Name -match '^(Skate|ReSkateLauncher)\.exe$'})
    if ($running.Count -gt 0) { throw 'Please fully close ReSkate and its launcher yourself, then run this installer again.' }
}
function Assert-Hash([string]$Path,[string]$Expected) {
    if ((Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant() -ne $Expected.ToLowerInvariant()) { throw ('Checksum mismatch: '+[IO.Path]::GetFileName($Path)) }
}
Assert-Closed
foreach ($entry in $manifest.supported_game.PSObject.Properties) { Assert-Hash (Join-Path $game $entry.Name) $entry.Value.sha256 }
foreach ($file in $files) { Assert-Hash (Join-Path $package $file) $manifest.files.$file.sha256 }
$backup=Join-Path $game ('PlaytestBackups\'+(Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
New-Item -ItemType Directory -Path $backup -ErrorAction Stop | Out-Null
$previous=@{}
foreach ($file in $files) {
    $path=Join-Path $game $file
    if (Test-Path -LiteralPath $path -PathType Leaf) {
        $previous[$file]=(Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant()
        Copy-Item -LiteralPath $path -Destination (Join-Path $backup $file) -ErrorAction Stop
        Assert-Hash (Join-Path $backup $file) $previous[$file]
    }
}
$configuration=@{}
foreach ($relative in @('launcher.json','ReSkateLauncher.settings.json','ReSkate.favorites.json','Mods\mods.json')) {
    $path=Join-Path $game $relative
    if (Test-Path -LiteralPath $path -PathType Leaf) { $configuration[$relative]=(Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash }
}
$copied=@()
try {
    foreach ($file in $files) {
        Assert-Closed
        # Include a partially written destination in rollback if copying fails.
        $copied+=$file
        Copy-Item -LiteralPath (Join-Path $package $file) -Destination (Join-Path $game $file) -Force -ErrorAction Stop
        Assert-Hash (Join-Path $game $file) $manifest.files.$file.sha256
    }
} catch {
    $problem=$_
    Assert-Closed
    foreach ($file in $copied) {
        if ($previous.ContainsKey($file)) {
            Copy-Item -LiteralPath (Join-Path $backup $file) -Destination (Join-Path $game $file) -Force -ErrorAction Stop
            Assert-Hash (Join-Path $game $file) $previous[$file]
        } else {
            # Preserve a newly added file on a failed install; never delete it.
            Move-Item -LiteralPath (Join-Path $game $file) -Destination (Join-Path $backup ('failed-new-'+$file)) -ErrorAction Stop
        }
    }
    throw $problem
}
foreach ($relative in $configuration.Keys) {
    if ((Get-FileHash -LiteralPath (Join-Path $game $relative) -Algorithm SHA256).Hash -ne $configuration[$relative]) { throw ('Configuration changed during installation: '+$relative) }
}
@{release=$manifest.release_tag;installed_at_utc=(Get-Date).ToUniversalTime().ToString('o');files=$manifest.files;previous_binary_hashes=$previous;configuration_unchanged=$true;programs_controlled=$false} | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $backup 'installation-receipt.json') -Encoding utf8
Write-Output ('Installed '+$manifest.release_tag+'. Backup: '+$backup)
Write-Output 'Mods and settings preserved. Launch ReSkate yourself when ready.'
