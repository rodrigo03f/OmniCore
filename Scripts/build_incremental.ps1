param(
    [string]$ProjectRoot = "",
    [switch]$KillUnrealEditor,
    [switch]$SkipProjectFiles
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$devScript = Join-Path $PSScriptRoot "omni_build_dev.ps1"
if (-not (Test-Path $devScript)) {
    throw "Required script not found: $devScript"
}

if ($SkipProjectFiles) {
    Write-Host "Info: -SkipProjectFiles is now implicit in DEV mode." -ForegroundColor DarkYellow
}

$args = @()
if (-not [string]::IsNullOrWhiteSpace($ProjectRoot)) {
    $args += @("-ProjectRoot", $ProjectRoot)
}
if ($KillUnrealEditor) { $args += "-KillUnrealEditor" }

Write-Host "Compatibility alias: build_incremental -> omni_build_dev" -ForegroundColor DarkYellow
powershell -ExecutionPolicy Bypass -File $devScript @args
exit $LASTEXITCODE
