param(
    [string]$OutputRoot = ""
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$projectRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))

if ([string]::IsNullOrWhiteSpace($OutputRoot)) {
    $OutputRoot = Join-Path $projectRoot "Saved\\Logs\\Automation"
}

$timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
$outDir = Join-Path $OutputRoot "diag_$timestamp"
New-Item -ItemType Directory -Path $outDir -Force | Out-Null

function Copy-IfExists {
    param(
        [string]$SourcePath,
        [string]$TargetName
    )

    if (Test-Path $SourcePath) {
        Copy-Item -Path $SourcePath -Destination (Join-Path $outDir $TargetName) -Force
        return $true
    }

    return $false
}

$copied = @()

$mainLog = Join-Path $projectRoot "Saved\\Logs\\OmniSandbox.log"
if (Copy-IfExists -SourcePath $mainLog -TargetName "OmniSandbox.log") { $copied += $mainLog }

$ubtLog = "$env:LOCALAPPDATA\\UnrealBuildTool\\Log.txt"
if (Copy-IfExists -SourcePath $ubtLog -TargetName "UnrealBuildTool_Log.txt") { $copied += $ubtLog }

$crashRoot = Join-Path $projectRoot "Saved\\Crashes"
if (Test-Path $crashRoot) {
    $latestCrash = Get-ChildItem -Path $crashRoot -Directory | Sort-Object LastWriteTime -Descending | Select-Object -First 1
    if ($latestCrash) {
        $crashLog = Join-Path $latestCrash.FullName "OmniSandbox.log"
        $crashXml = Join-Path $latestCrash.FullName "CrashContext.runtime-xml"
        if (Copy-IfExists -SourcePath $crashLog -TargetName "Crash_OmniSandbox.log") { $copied += $crashLog }
        if (Copy-IfExists -SourcePath $crashXml -TargetName "CrashContext.runtime-xml") { $copied += $crashXml }
    }
}

$summaryPath = Join-Path $outDir "summary.txt"
$summary = New-Object System.Collections.Generic.List[string]
$summary.Add("Omni diagnostics summary")
$summary.Add("Generated: $(Get-Date -Format "yyyy-MM-dd HH:mm:ss")")
$summary.Add("Project: $projectRoot")
$summary.Add("")
$summary.Add("Copied files:")

if ($copied.Count -eq 0) {
    $summary.Add("- none")
}
else {
    foreach ($file in $copied) {
        $summary.Add("- $file")
    }
}

$summary.Add("")
$summary.Add("Detected error lines:")
$errorPattern = '(?i)\berror\b|fatal error|assertion failed|unhandled exception|access violation|MSB\d+|error C\d+|error LNK|BUILD FAILED|Failed \('

foreach ($copiedFile in Get-ChildItem -Path $outDir -File) {
    $matches = Select-String -Path $copiedFile.FullName -Pattern $errorPattern -ErrorAction SilentlyContinue
    if ($matches) {
        $summary.Add("")
        $summary.Add("[$($copiedFile.Name)]")
        foreach ($match in ($matches | Select-Object -First 40)) {
            $summary.Add("$($match.LineNumber): $($match.Line.Trim())")
        }
    }
}

$summary | Set-Content -Path $summaryPath -Encoding UTF8

Write-Host "Diagnostics collected in: $outDir" -ForegroundColor Green
Write-Host "Summary: $summaryPath" -ForegroundColor Green

