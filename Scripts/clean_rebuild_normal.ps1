param(
    [string]$ProjectRoot = "",
    [switch]$KillUnrealEditor,
    [switch]$SkipProjectFiles,
    [switch]$SkipBuild,
    [switch]$SkipZenHygiene,
    [switch]$AllPlugins
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$carimboScript = Join-Path $PSScriptRoot "omni_build_carimbo.ps1"
if (-not (Test-Path $carimboScript)) {
    throw "Required script not found: $carimboScript"
}

if ($SkipProjectFiles -or $SkipZenHygiene -or $AllPlugins) {
    Write-Host "Info: legacy flags ignored in CARIMBO mode." -ForegroundColor DarkYellow
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

Write-Host "Compatibility alias: clean_rebuild_normal -> omni_build_carimbo" -ForegroundColor DarkYellow
powershell -ExecutionPolicy Bypass -File $carimboScript @args
exit $LASTEXITCODE
