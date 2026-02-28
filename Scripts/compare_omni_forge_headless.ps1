param(
    [string]$ProjectRoot = "",
    [string]$EngineRoot = "D:\\Epic Games\\UE_5.7",
    [string]$EditorCmdPath = "",
    [string]$UProject = "",
    [string]$ForgeExecCmd = "omni.forge.run manifestClass=/Script/OmniRuntime.OmniOfficialManifest requireContentAssets=1 root=/Game/Data",
    [string]$Artifact = "Saved/Omni/ResolvedManifest.json",
    [int]$RunTimeoutSeconds = 600,
    [string]$DiagnosticsDir = "",
    [switch]$NoNullRHI,
    [switch]$SingleRunOnly
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Resolve-ExecCmds {
    param([string]$RawExecCmd)

    if ([string]::IsNullOrWhiteSpace($RawExecCmd)) {
        throw "ForgeExecCmd must not be empty."
    }

    $normalized = $RawExecCmd.Trim().TrimEnd(';')
    if ($normalized -notmatch "(^|;)\s*quit\s*$") {
        $normalized = "$normalized;quit"
    }

    return $normalized
}

function Resolve-ProjectRoot {
    param([string]$InputRoot)
    if (-not [string]::IsNullOrWhiteSpace($InputRoot)) {
        return [System.IO.Path]::GetFullPath($InputRoot)
    }
    return [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
}

function Resolve-EditorCmdPath {
    param(
        [string]$PreferredEditorCmdPath,
        [string]$EngineRootPath
    )

    if (-not [string]::IsNullOrWhiteSpace($PreferredEditorCmdPath)) {
        $candidate = [System.IO.Path]::GetFullPath($PreferredEditorCmdPath)
        if (Test-Path $candidate) {
            return $candidate
        }
        throw "UnrealEditor-Cmd.exe not found at -EditorCmdPath '$candidate'."
    }

    if (-not [string]::IsNullOrWhiteSpace($env:OMNI_UNREAL_EDITOR_CMD)) {
        $candidate = [System.IO.Path]::GetFullPath($env:OMNI_UNREAL_EDITOR_CMD)
        if (Test-Path $candidate) {
            return $candidate
        }
        throw "UnrealEditor-Cmd.exe not found at env:OMNI_UNREAL_EDITOR_CMD '$candidate'."
    }

    if (-not [string]::IsNullOrWhiteSpace($EngineRootPath)) {
        $candidate = Join-Path $EngineRootPath "Engine\\Binaries\\Win64\\UnrealEditor-Cmd.exe"
        if (Test-Path $candidate) {
            return [System.IO.Path]::GetFullPath($candidate)
        }
    }

    $editorCmd = Get-Command "UnrealEditor-Cmd.exe" -ErrorAction SilentlyContinue
    if ($editorCmd -and -not [string]::IsNullOrWhiteSpace($editorCmd.Source)) {
        return [System.IO.Path]::GetFullPath($editorCmd.Source)
    }

    throw "UnrealEditor-Cmd.exe not found. Set -EditorCmdPath, -EngineRoot, or env:OMNI_UNREAL_EDITOR_CMD."
}

function Resolve-DiagnosticsDir {
    param(
        [string]$InputDiagnosticsDir,
        [string]$ResolvedProjectRoot
    )

    if ([string]::IsNullOrWhiteSpace($InputDiagnosticsDir)) {
        return Join-Path $ResolvedProjectRoot "Saved\\Logs\\Automation\\forge_headless"
    }

    if ([System.IO.Path]::IsPathRooted($InputDiagnosticsDir)) {
        return [System.IO.Path]::GetFullPath($InputDiagnosticsDir)
    }

    return [System.IO.Path]::GetFullPath((Join-Path $ResolvedProjectRoot $InputDiagnosticsDir))
}

function Save-RunLogSnapshot {
    param(
        [string]$Label,
        [string]$LogPath,
        [string]$OutputDir
    )

    if (-not (Test-Path $LogPath)) {
        return $null
    }

    if (-not (Test-Path $OutputDir)) {
        [void](New-Item -Path $OutputDir -ItemType Directory -Force)
    }

    $timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
    $destination = Join-Path $OutputDir ("forge_{0}_{1}.log" -f $Label, $timestamp)
    Copy-Item -Path $LogPath -Destination $destination -Force
    return $destination
}

$resolvedProjectRoot = Resolve-ProjectRoot -InputRoot $ProjectRoot

if ([string]::IsNullOrWhiteSpace($UProject)) {
    $uprojectFile = Get-ChildItem -Path $resolvedProjectRoot -Filter *.uproject -File | Select-Object -First 1
    if (-not $uprojectFile) {
        throw "Could not find .uproject in $resolvedProjectRoot"
    }
    $UProject = $uprojectFile.FullName
} else {
    $UProject = [System.IO.Path]::GetFullPath($UProject)
}

$editorCmd = Resolve-EditorCmdPath -PreferredEditorCmdPath $EditorCmdPath -EngineRootPath $EngineRoot

$artifactPath = if ([System.IO.Path]::IsPathRooted($Artifact)) {
    $Artifact
} else {
    Join-Path $resolvedProjectRoot $Artifact
}

$diagnosticsOutputDir = Resolve-DiagnosticsDir -InputDiagnosticsDir $DiagnosticsDir -ResolvedProjectRoot $resolvedProjectRoot
if (-not (Test-Path $diagnosticsOutputDir)) {
    [void](New-Item -Path $diagnosticsOutputDir -ItemType Directory -Force)
}

$effectiveExecCmd = Resolve-ExecCmds -RawExecCmd $ForgeExecCmd

function Read-NewLogChunk {
    param(
        [string]$Path,
        [long]$Offset,
        [ref]$NewOffset
    )

    $NewOffset.Value = $Offset
    if (-not (Test-Path $Path)) {
        return ""
    }

    $fileInfo = Get-Item -Path $Path
    if ($fileInfo.Length -lt $Offset) {
        $Offset = 0
    }
    if ($fileInfo.Length -eq $Offset) {
        $NewOffset.Value = $Offset
        return ""
    }

    $stream = [System.IO.File]::Open($Path, [System.IO.FileMode]::Open, [System.IO.FileAccess]::Read, [System.IO.FileShare]::ReadWrite)
    try {
        [void]$stream.Seek($Offset, [System.IO.SeekOrigin]::Begin)
        $sizeToRead = [int]($stream.Length - $Offset)
        $buffer = New-Object byte[] $sizeToRead
        $bytesRead = $stream.Read($buffer, 0, $sizeToRead)
        $NewOffset.Value = $Offset + $bytesRead
        return [System.Text.Encoding]::UTF8.GetString($buffer, 0, $bytesRead)
    }
    finally {
        $stream.Dispose()
    }
}

function Invoke-ForgeHeadlessRun {
    param([string]$Label)

    Write-Host "Running Forge ($Label)..."
    $execCmdsArg = "-ExecCmds=`"$effectiveExecCmd`""
    $args = @(
        $UProject,
        "-unattended",
        "-nop4",
        "-nosplash",
        "-NoSound"
    )
    if (-not $NoNullRHI) {
        $args += "-nullrhi"
    }
    $args += $execCmdsArg

    $projectName = [System.IO.Path]::GetFileNameWithoutExtension($UProject)
    $logPath = Join-Path $resolvedProjectRoot ("Saved\\Logs\\{0}.log" -f $projectName)
    $logOffset = if (Test-Path $logPath) { (Get-Item -Path $logPath).Length } else { 0L }

    $process = Start-Process -FilePath $editorCmd -ArgumentList $args -PassThru
    $deadline = (Get-Date).AddSeconds($RunTimeoutSeconds)
    $runStatus = ""

    while ((Get-Date) -lt $deadline) {
        $newOffset = $logOffset
        $chunk = Read-NewLogChunk -Path $logPath -Offset $logOffset -NewOffset ([ref]$newOffset)
        $logOffset = $newOffset

        if (-not [string]::IsNullOrWhiteSpace($chunk)) {
            if ($chunk -match "LogOmniForge:\s+Forge PASS\.") {
                $runStatus = "PASS"
                break
            }
            if ($chunk -match "LogOmniForge:\s+Forge FAIL\.") {
                $runStatus = "FAIL"
                break
            }
        }

        $process.Refresh()
        if ($process.HasExited) {
            break
        }

        Start-Sleep -Milliseconds 500
    }

    $process.Refresh()
    if (-not $process.HasExited) {
        Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
        [void](Wait-Process -Id $process.Id -Timeout 10 -ErrorAction SilentlyContinue)
    }

    $snapshotPath = Save-RunLogSnapshot -Label $Label -LogPath $logPath -OutputDir $diagnosticsOutputDir
    if ($snapshotPath) {
        Write-Host "Run log snapshot ($Label): $snapshotPath"
    }

    if ($runStatus -eq "PASS") {
        if (-not (Test-Path $artifactPath)) {
            throw "Artifact not found after run '$Label': $artifactPath"
        }
        return
    }

    if ($runStatus -eq "FAIL") {
        throw "Forge run '$Label' reported FAIL in log."
    }

    if ($process.ExitCode -ne 0) {
        throw "Forge run '$Label' failed with exit code $($process.ExitCode)."
    }

    throw "Forge run '$Label' timed out after $RunTimeoutSeconds seconds."
}

try {
    if ($SingleRunOnly) {
        Invoke-ForgeHeadlessRun -Label "single"
        Write-Host "SUCCESS: Forge single headless run passed."
        exit 0
    }

    Invoke-ForgeHeadlessRun -Label "first"
    $hash1 = (Get-FileHash -Path $artifactPath -Algorithm SHA256).Hash
    Write-Host "Hash #1: $hash1"

    Invoke-ForgeHeadlessRun -Label "second"
    $hash2 = (Get-FileHash -Path $artifactPath -Algorithm SHA256).Hash
    Write-Host "Hash #2: $hash2"

    if ($hash1 -eq $hash2) {
        Write-Host "SUCCESS: Hashes match."
        exit 0
    }

    Write-Host "FAIL: Hashes differ."
    exit 1
}
catch {
    Write-Host "FAIL: $($_.Exception.Message)"
    exit 1
}
