param(
    [string]$ProjectRoot = "",
    [string]$EngineRoot = "D:\\Epic Games\\UE_5.7",
    [string]$UProject = "",
    [string]$ForgeExecCmd = "omni.forge.run manifestClass=/Script/OmniRuntime.OmniOfficialManifest requireContentAssets=1 root=/Game/Data",
    [string]$Artifact = "Saved/Omni/ResolvedManifest.json",
    [int]$RunTimeoutSeconds = 600,
    [switch]$NoNullRHI
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

$editorCmd = Join-Path $EngineRoot "Engine\\Binaries\\Win64\\UnrealEditor-Cmd.exe"
if (-not (Test-Path $editorCmd)) {
    throw "UnrealEditor-Cmd.exe not found: $editorCmd"
}

$artifactPath = if ([System.IO.Path]::IsPathRooted($Artifact)) {
    $Artifact
} else {
    Join-Path $resolvedProjectRoot $Artifact
}

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
    $execCmdsArg = "-ExecCmds=`"$ForgeExecCmd`""
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
