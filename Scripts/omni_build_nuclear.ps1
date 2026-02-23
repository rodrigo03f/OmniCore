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

    throw "Could not resolve engine root for EngineAssociation='$assoc'. Checked: $($candidates -join ', ')"
}

function Remove-DirIfExists {
    param([string]$PathToRemove)
    if (Test-Path $PathToRemove) {
        Write-Host "Removing: $PathToRemove" -ForegroundColor DarkYellow
        Remove-Item -Path $PathToRemove -Recurse -Force
    }
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

$sw = [System.Diagnostics.Stopwatch]::StartNew()

$projectRoot = Resolve-ProjectRoot -InputRoot $ProjectRoot
$uprojectPath = Resolve-UprojectPath -Root $projectRoot
$projectName = [System.IO.Path]::GetFileNameWithoutExtension($uprojectPath)
$engineRoot = Resolve-EngineRoot -UprojectPath $uprojectPath
$buildBat = Join-Path $engineRoot "Engine\Build\BatchFiles\Build.bat"
$generateProjectFilesBat = Join-Path $engineRoot "Engine\Build\BatchFiles\GenerateProjectFiles.bat"
$zenHygieneScript = Join-Path $PSScriptRoot "zen_hygiene.ps1"

if (-not (Test-Path $buildBat)) {
    throw "Build.bat not found at '$buildBat'."
}

Write-Host "Build Mode: NUCLEAR (higiene completa)" -ForegroundColor Green
Write-Host "Project root: $projectRoot" -ForegroundColor Green
Write-Host "UProject: $uprojectPath" -ForegroundColor Green
Write-Host "Engine root: $engineRoot" -ForegroundColor Green

if ($KillUnrealEditor) {
    $editorProcesses = Get-Process -Name "UnrealEditor" -ErrorAction SilentlyContinue
    if ($editorProcesses) {
        Write-Host "Stopping UnrealEditor process(es)..." -ForegroundColor DarkYellow
        $editorProcesses | Stop-Process -Force
    }
}

if (Test-Path $zenHygieneScript) {
    Invoke-External -Exe "powershell" -ArgumentList @(
        "-ExecutionPolicy",
        "Bypass",
        "-File",
        $zenHygieneScript,
        "-ProjectRoot",
        $projectRoot
    )
}
else {
    Write-Host "Zen hygiene script not found; continuing." -ForegroundColor DarkYellow
}

Remove-DirIfExists -PathToRemove (Join-Path $projectRoot "Intermediate")
Remove-DirIfExists -PathToRemove (Join-Path $projectRoot "Binaries")
Remove-DirIfExists -PathToRemove (Join-Path $projectRoot "Plugins\Omni\Intermediate")
Remove-DirIfExists -PathToRemove (Join-Path $projectRoot "Plugins\Omni\Binaries")

if (Test-Path $generateProjectFilesBat) {
    Invoke-External -Exe $generateProjectFilesBat -ArgumentList @(
        "-project=$uprojectPath",
        "-game",
        "-engine"
    )
}
else {
    Invoke-External -Exe $buildBat -ArgumentList @(
        "-projectfiles",
        "-project=$uprojectPath",
        "-game",
        "-engine"
    )
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

$sw.Stop()
Write-Host ("NUCLEAR pipeline completed. Wall clock: {0:n2} min ({1:n2} s)." -f $sw.Elapsed.TotalMinutes, $sw.Elapsed.TotalSeconds) -ForegroundColor Green
