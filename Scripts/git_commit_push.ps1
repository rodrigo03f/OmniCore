param(
    [Parameter(Mandatory = $true)]
    [string]$Message,
    [string]$ProjectRoot = "",
    [string]$Remote = "origin",
    [string]$Branch = "",
    [switch]$NoPush
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

function Resolve-GitExe {
    $gitCmd = Get-Command git -ErrorAction SilentlyContinue
    if ($gitCmd) {
        return $gitCmd.Source
    }

    $candidates = @(
        "C:\Program Files\Git\cmd\git.exe",
        "C:\Program Files\Git\bin\git.exe"
    )

    foreach ($candidate in $candidates) {
        if (Test-Path $candidate) {
            return $candidate
        }
    }

    throw "Git executable not found."
}

function Invoke-Git {
    param(
        [string]$GitExe,
        [string]$WorkingDirectory,
        [string[]]$ArgumentList
    )

    & $GitExe @ArgumentList
    if ($LASTEXITCODE -ne 0) {
        throw "Git command failed ($LASTEXITCODE): git $($ArgumentList -join ' ')"
    }
}

$projectRoot = Resolve-ProjectRoot -InputRoot $ProjectRoot
$gitExe = Resolve-GitExe

Push-Location $projectRoot
try {
    Invoke-Git -GitExe $gitExe -WorkingDirectory $projectRoot -ArgumentList @("rev-parse", "--is-inside-work-tree")

    Invoke-Git -GitExe $gitExe -WorkingDirectory $projectRoot -ArgumentList @("add", "-A")

    $statusOutput = & $gitExe status --short
    if (-not $statusOutput) {
        Write-Host "No changes to commit." -ForegroundColor DarkYellow
        return
    }

    Invoke-Git -GitExe $gitExe -WorkingDirectory $projectRoot -ArgumentList @("commit", "-m", $Message)

    if ($NoPush) {
        Write-Host "Commit created. Push skipped by -NoPush." -ForegroundColor Green
        return
    }

    if ([string]::IsNullOrWhiteSpace($Branch)) {
        $Branch = (& $gitExe rev-parse --abbrev-ref HEAD).Trim()
        if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($Branch)) {
            throw "Unable to resolve current branch."
        }
    }

    Invoke-Git -GitExe $gitExe -WorkingDirectory $projectRoot -ArgumentList @("push", "-u", $Remote, $Branch)
    Write-Host "Commit and push completed ($Remote/$Branch)." -ForegroundColor Green
}
finally {
    Pop-Location
}
