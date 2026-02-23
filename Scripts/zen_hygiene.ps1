param(
    [string]$ProjectRoot = "",
    [switch]$KillUnrealEditor
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

function Resolve-UprojectPath {
    param([string]$Root)
    $uproject = Get-ChildItem -Path $Root -File -Filter *.uproject | Select-Object -First 1
    if (-not $uproject) {
        throw "No .uproject file found in '$Root'."
    }
    return $uproject.FullName
}

function Resolve-EngineRoot {
    param([string]$UprojectPath)
    $uprojectJson = Get-Content -Raw $UprojectPath | ConvertFrom-Json
    $assoc = [string]$uprojectJson.EngineAssociation
    if ([string]::IsNullOrWhiteSpace($assoc)) {
        throw "EngineAssociation is empty in '$UprojectPath'."
    }

    $candidates = @(
        "D:\Epic Games\UE_$assoc",
        "D:\Epic Games\UE-$assoc",
        "C:\Program Files\Epic Games\UE_$assoc",
        "C:\Program Files\Epic Games\UE-$assoc"
    )

    foreach ($candidate in $candidates) {
        if (Test-Path $candidate) {
            return $candidate
        }
    }

    throw "Could not resolve engine root for EngineAssociation='$assoc'."
}

function Invoke-ZenCommand {
    param(
        [string]$ZenExe,
        [string]$Arguments,
        [int]$TimeoutSeconds = 8
    )

    if (-not (Test-Path $ZenExe)) {
        return @{
            Ran = $false
            TimedOut = $false
            ExitCode = $null
            Output = ""
        }
    }

    $tmpOut = [System.IO.Path]::GetTempFileName()
    $tmpErr = [System.IO.Path]::GetTempFileName()

    try {
        $proc = Start-Process -FilePath $ZenExe -ArgumentList $Arguments -NoNewWindow -PassThru `
            -RedirectStandardOutput $tmpOut -RedirectStandardError $tmpErr

        if (-not $proc.WaitForExit($TimeoutSeconds * 1000)) {
            try { $proc.Kill($true) } catch {}
            return @{
                Ran = $true
                TimedOut = $true
                ExitCode = $null
                Output = "Timed out after $TimeoutSeconds seconds."
            }
        }

        $proc.Refresh()
        $exitCode = $proc.ExitCode
        $stdout = Get-Content $tmpOut -Raw -ErrorAction SilentlyContinue
        $stderr = Get-Content $tmpErr -Raw -ErrorAction SilentlyContinue
        $combined = @($stdout, $stderr) -join "`n"

        return @{
            Ran = $true
            TimedOut = $false
            ExitCode = $exitCode
            Output = ($combined.Trim())
        }
    }
    finally {
        Remove-Item $tmpOut -Force -ErrorAction SilentlyContinue
        Remove-Item $tmpErr -Force -ErrorAction SilentlyContinue
    }
}

function Write-ZenResult {
    param(
        [string]$Prefix,
        [hashtable]$Result
    )

    if (-not $Result.Ran) {
        Write-Host "$Prefix skipped (zen.exe not found)." -ForegroundColor DarkYellow
        return
    }

    if ($Result.TimedOut) {
        Write-Host "$Prefix timed out." -ForegroundColor DarkYellow
        return
    }

    Write-Host "$Prefix done." -ForegroundColor Gray
    if (-not [string]::IsNullOrWhiteSpace($Result.Output)) {
        $oneLine = ($Result.Output -replace "\r?\n", " | ")
        if ($oneLine.Length -gt 220) {
            $oneLine = $oneLine.Substring(0, 220) + "..."
        }
        Write-Host "  $oneLine" -ForegroundColor DarkGray
    }
}

$projectRoot = Resolve-ProjectRoot -InputRoot $ProjectRoot
$uprojectPath = Resolve-UprojectPath -Root $projectRoot
$engineRoot = Resolve-EngineRoot -UprojectPath $uprojectPath

Write-Host "Project root: $projectRoot" -ForegroundColor Green
Write-Host "Engine root: $engineRoot" -ForegroundColor Green

if ($KillUnrealEditor) {
    $editorProcesses = Get-Process -Name "UnrealEditor" -ErrorAction SilentlyContinue
    if ($editorProcesses) {
        Write-Host "Stopping UnrealEditor process(es)..." -ForegroundColor DarkYellow
        $editorProcesses | Stop-Process -Force
    }
}

$zenExeCandidates = @(
    (Join-Path $engineRoot "Engine\Binaries\Win64\zen.exe"),
    (Join-Path $env:LOCALAPPDATA "UnrealEngine\Common\Zen\Install\zen.exe")
) | Select-Object -Unique

Write-Host "Running Zen hygiene..." -ForegroundColor Cyan
foreach ($zenExe in $zenExeCandidates) {
    if (Test-Path $zenExe) {
        Write-Host "Using zen utility: $zenExe" -ForegroundColor Gray
        $down = Invoke-ZenCommand -ZenExe $zenExe -Arguments "down"
        Write-ZenResult -Prefix "  zen down" -Result $down

        $svc = Invoke-ZenCommand -ZenExe $zenExe -Arguments "service status"
        Write-ZenResult -Prefix "  zen service status" -Result $svc
    }
}

$zenServerProcs = Get-Process -Name "zenserver" -ErrorAction SilentlyContinue
if ($zenServerProcs) {
    Write-Host "Stopping lingering zenserver process(es): $($zenServerProcs.Count)" -ForegroundColor DarkYellow
    $zenServerProcs | Stop-Process -Force
}
else {
    Write-Host "No lingering zenserver processes found." -ForegroundColor Green
}

Write-Host "Zen hygiene completed." -ForegroundColor Green
