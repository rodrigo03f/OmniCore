param(
    [string]$ProjectRoot = "",
    [switch]$SkipProjectFiles,
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

    throw "Could not resolve engine root for EngineAssociation='$assoc'. Checked: $($candidates -join ', ')"
}

function Invoke-External {
    param(
        [string]$Exe,
        [string[]]$ArgumentList
    )
    Write-Host "> $Exe $($ArgumentList -join ' ')" -ForegroundColor Cyan
    & $Exe @ArgumentList
    if ($LASTEXITCODE -ne 0) {
        throw "Command failed with exit code ${LASTEXITCODE}: $Exe"
    }
}

$projectRoot = Resolve-ProjectRoot -InputRoot $ProjectRoot
$uprojectPath = Resolve-UprojectPath -Root $projectRoot
$projectName = [System.IO.Path]::GetFileNameWithoutExtension($uprojectPath)
$engineRoot = Resolve-EngineRoot -UprojectPath $uprojectPath

$generateProjectFilesBat = Join-Path $engineRoot "Engine\Build\BatchFiles\GenerateProjectFiles.bat"
$buildBat = Join-Path $engineRoot "Engine\Build\BatchFiles\Build.bat"

if (-not (Test-Path $buildBat)) {
    throw "Build.bat not found at '$buildBat'."
}

Write-Host "Project root: $projectRoot" -ForegroundColor Green
Write-Host "UProject: $uprojectPath" -ForegroundColor Green
Write-Host "Engine root: $engineRoot" -ForegroundColor Green
Write-Host "Mode: incremental build (recommended daily path)" -ForegroundColor Green

if ($KillUnrealEditor) {
    $editorProcesses = Get-Process -Name "UnrealEditor" -ErrorAction SilentlyContinue
    if ($editorProcesses) {
        Write-Host "Stopping UnrealEditor process(es)..." -ForegroundColor DarkYellow
        $editorProcesses | Stop-Process -Force
    }
}

if (-not $SkipProjectFiles) {
    if (Test-Path $generateProjectFilesBat) {
        Invoke-External -Exe $generateProjectFilesBat -ArgumentList @(
            "-project=$uprojectPath",
            "-game",
            "-engine"
        )
    }
    else {
        Write-Host "GenerateProjectFiles.bat not found; using Build.bat -projectfiles fallback." -ForegroundColor DarkYellow
        Invoke-External -Exe $buildBat -ArgumentList @(
            "-projectfiles",
            "-project=$uprojectPath",
            "-game",
            "-engine"
        )
    }
}

$target = "$projectName" + "Editor"
Invoke-External -Exe $buildBat -ArgumentList @(
    $target,
    "Win64",
    "Development",
    "-Project=$uprojectPath",
    "-WaitMutex",
    "-FromMsBuild"
)

Write-Host "Incremental build completed." -ForegroundColor Green
