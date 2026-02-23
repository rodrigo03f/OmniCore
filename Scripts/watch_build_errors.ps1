param(
    [string]$LogPath = ""
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Resolve-DefaultLogPath {
    $candidates = @(
        "$env:LOCALAPPDATA\\UnrealBuildTool\\Log.txt",
        (Join-Path $PSScriptRoot "..\\Saved\\Logs\\OmniSandbox.log")
    )

    foreach ($candidate in $candidates) {
        $resolved = [System.IO.Path]::GetFullPath($candidate)
        if (Test-Path $resolved) {
            return $resolved
        }
    }

    return $null
}

if ([string]::IsNullOrWhiteSpace($LogPath)) {
    $LogPath = Resolve-DefaultLogPath
}

if ([string]::IsNullOrWhiteSpace($LogPath)) {
    Write-Host "No log file found. Pass -LogPath explicitly." -ForegroundColor Red
    exit 1
}

$LogPath = [System.IO.Path]::GetFullPath($LogPath)
if (-not (Test-Path $LogPath)) {
    Write-Host "Log file not found: $LogPath" -ForegroundColor Red
    exit 1
}

Write-Host "Watching log: $LogPath" -ForegroundColor Cyan
Write-Host "Press Ctrl+C to stop." -ForegroundColor DarkGray

$errorPattern = '(?i)\berror\b|fatal error|assertion failed|unhandled exception|access violation|MSB\d+|error C\d+|error LNK'
$warningPattern = '(?i)\bwarning\b|ensure condition failed'

Get-Content -Path $LogPath -Wait -Tail 0 | ForEach-Object {
    $line = $_
    if ($line -match $errorPattern) {
        Write-Host $line -ForegroundColor Red
    }
    elseif ($line -match $warningPattern) {
        Write-Host $line -ForegroundColor Yellow
    }
}

