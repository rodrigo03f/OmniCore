param(
    [string]$ProjectRoot = "",
    [switch]$SkipProjectFiles,
    [switch]$SkipBuild,
    [switch]$KillUnrealEditor,
    [switch]$SkipZenHygiene,
    [switch]$PurgeGlobalDDC,
    [switch]$AllPlugins
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Resolve-ProjectRoot {
    param([string]$InputRoot)
    if (-not [string]::IsNullOrWhiteSpace($InputRoot)) {
        return [System.IO.Path]::GetFullPath($InputRoot)
    }
    return [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
}

function Remove-DirIfExists {
    param([string]$PathToRemove)
    if (Test-Path $PathToRemove) {
        Write-Host "Removing: $PathToRemove" -ForegroundColor DarkYellow
        Remove-Item -Path $PathToRemove -Recurse -Force
    }
}

$projectRoot = Resolve-ProjectRoot -InputRoot $ProjectRoot
$normalScript = Join-Path $PSScriptRoot "clean_rebuild_normal.ps1"

if (-not (Test-Path $normalScript)) {
    throw "Required script not found: $normalScript"
}

Write-Host "Project root: $projectRoot" -ForegroundColor Green
Write-Host "Hard clean will remove additional cache folders." -ForegroundColor DarkYellow
Write-Host "Warning: hard clean rebuild is for exceptional troubleshooting only." -ForegroundColor Yellow

# Extra cleanup for hard mode
Remove-DirIfExists -PathToRemove (Join-Path $projectRoot "DerivedDataCache")
Remove-DirIfExists -PathToRemove (Join-Path $projectRoot "Saved\Cooked")
Remove-DirIfExists -PathToRemove (Join-Path $projectRoot "Saved\StagedBuilds")
Remove-DirIfExists -PathToRemove (Join-Path $projectRoot "Saved\ShaderDebugInfo")
Remove-DirIfExists -PathToRemove (Join-Path $projectRoot "Saved\webcache_4430")
Remove-DirIfExists -PathToRemove (Join-Path $projectRoot "Saved\webcache_6613")

if ($PurgeGlobalDDC) {
    Remove-DirIfExists -PathToRemove "$env:LOCALAPPDATA\UnrealEngine\Common\DerivedDataCache"
}

$normalArgs = @(
    "-ProjectRoot", $projectRoot
)

if ($SkipProjectFiles) { $normalArgs += "-SkipProjectFiles" }
if ($SkipBuild) { $normalArgs += "-SkipBuild" }
if ($KillUnrealEditor) { $normalArgs += "-KillUnrealEditor" }
if ($SkipZenHygiene) { $normalArgs += "-SkipZenHygiene" }
if ($AllPlugins) { $normalArgs += "-AllPlugins" }

Write-Host "> powershell -ExecutionPolicy Bypass -File `"$normalScript`" $($normalArgs -join ' ')" -ForegroundColor Cyan
powershell -ExecutionPolicy Bypass -File $normalScript @normalArgs
if ($LASTEXITCODE -ne 0) {
    throw "Hard clean failed during normal rebuild stage. Exit code: $LASTEXITCODE"
}

Write-Host "Hard clean rebuild completed." -ForegroundColor Green
