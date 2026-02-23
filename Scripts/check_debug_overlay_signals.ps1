param(
    [int]$TailLines = 3000
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$projectRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
$logPath = Join-Path $projectRoot "Saved\\Logs\\OmniSandbox.log"

if (-not (Test-Path $logPath)) {
    Write-Host "Log not found: $logPath" -ForegroundColor Red
    exit 1
}

$lines = Get-Content -Path $logPath
if ($lines.Count -gt $TailLines) {
    $lines = $lines[($lines.Count - $TailLines)..($lines.Count - 1)]
}

$checks = @(
    @{ Name = "Overlay shown"; Patterns = @("Overlay de debug exibido", "Debug overlay shown") },
    @{ Name = "Overlay hidden"; Patterns = @("Overlay de debug ocultado", "Debug overlay hidden") },
    @{ Name = "Entries cleared"; Patterns = @("Entradas de debug limpas", "Debug entries cleared", "via F9") },
    @{ Name = "Mode sync"; Patterns = @("omni.debug", "Modo de debug do Omni", "Omni debug mode") }
)

$allGood = $true

Write-Host "Checking debug overlay signals in: $logPath" -ForegroundColor Cyan
Write-Host "Scanned lines: $($lines.Count)" -ForegroundColor DarkGray
Write-Host ""

foreach ($check in $checks) {
    $count = 0
    foreach ($line in $lines) {
        foreach ($pattern in $check.Patterns) {
            if ($line -like "*$pattern*") {
                $count++
                break
            }
        }
    }

    if ($count -gt 0) {
        Write-Host ("[OK] {0}: {1} hit(s)" -f $check.Name, $count) -ForegroundColor Green
    }
    else {
        Write-Host ("[WARN] {0}: no hits" -f $check.Name) -ForegroundColor Yellow
        $allGood = $false
    }
}

Write-Host ""
if ($allGood) {
    Write-Host "Debug overlay signals look healthy." -ForegroundColor Green
    exit 0
}
else {
    Write-Host "Some signals were not found. Run PIE actions (F10/F9/commands) and check again." -ForegroundColor Yellow
    exit 2
}

