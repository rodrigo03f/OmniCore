param(
    [string]$ProjectRoot = "",
    [switch]$KillUnrealEditor,
    [switch]$PurgeGlobalDDC,
    [switch]$SkipProjectFiles,
    [switch]$SkipBuild,
    [switch]$SkipZenHygiene,
    [switch]$AllPlugins
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$nuclearScript = Join-Path $PSScriptRoot "omni_build_nuclear.ps1"
if (-not (Test-Path $nuclearScript)) {
    throw "Required script not found: $nuclearScript"
}

if ($PurgeGlobalDDC -or $SkipProjectFiles -or $SkipZenHygiene -or $AllPlugins) {
    Write-Host "Info: legacy flags ignored in NUCLEAR mode." -ForegroundColor DarkYellow
}

if ($SkipBuild) {
    Write-Host "Info: -SkipBuild set; nothing to do." -ForegroundColor DarkYellow
    exit 0
}

$args = @()
if (-not [string]::IsNullOrWhiteSpace($ProjectRoot)) {
    $args += @("-ProjectRoot", $ProjectRoot)
}
if ($KillUnrealEditor) { $args += "-KillUnrealEditor" }

Write-Host "Compatibility alias: clean_rebuild_hard -> omni_build_nuclear" -ForegroundColor DarkYellow
powershell -ExecutionPolicy Bypass -File $nuclearScript @args
exit $LASTEXITCODE
