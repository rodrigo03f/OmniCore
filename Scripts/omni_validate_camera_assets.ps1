param(
    [string]$ProjectRoot = ""
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

function Get-CameraSectionBody {
    param([string]$IniText)

    $sectionMatch = [regex]::Match(
        $IniText,
        "(?ms)^\[Omni\.Camera\]\s*(?<body>.*?)(?=^\[[^\]]+\]|\z)"
    )

    if (-not $sectionMatch.Success) {
        return $null
    }

    return $sectionMatch.Groups["body"].Value
}

function Normalize-GameAssetPath {
    param([string]$RawPath)

    $path = $RawPath.Trim()
    if ($path.Contains(".")) {
        return $path.Split(".", 2)[0]
    }
    return $path
}

$resolvedProjectRoot = Resolve-ProjectRoot -InputRoot $ProjectRoot
$defaultGameIniPath = Join-Path $resolvedProjectRoot "Config/DefaultGame.ini"

if (-not (Test-Path $defaultGameIniPath)) {
    Write-Host "FAIL: DefaultGame.ini nao encontrado em $defaultGameIniPath" -ForegroundColor Red
    exit 1
}

$iniText = Get-Content -Raw -Path $defaultGameIniPath
$cameraSectionBody = Get-CameraSectionBody -IniText $iniText
if ($null -eq $cameraSectionBody) {
    Write-Host "FAIL: Secao [Omni.Camera] nao encontrada em Config/DefaultGame.ini." -ForegroundColor Red
    exit 1
}

$defaultRigMatch = [regex]::Match(
    $cameraSectionBody,
    "(?im)^\s*DefaultRig\s*=\s*(?<value>[^\r\n]+)\s*$"
)
if (-not $defaultRigMatch.Success) {
    Write-Host "FAIL: Chave DefaultRig ausente em [Omni.Camera]." -ForegroundColor Red
    exit 1
}

$defaultRigPath = Normalize-GameAssetPath -RawPath $defaultRigMatch.Groups["value"].Value
if ([string]::IsNullOrWhiteSpace($defaultRigPath) -or -not $defaultRigPath.StartsWith("/Game/")) {
    Write-Host "FAIL: DefaultRig invalido. Esperado caminho /Game/...; recebido: '$defaultRigPath'." -ForegroundColor Red
    exit 1
}

$contentRelativePath = ("Content" + $defaultRigPath.Substring("/Game".Length) + ".uasset").TrimStart("/", "\")
$contentAbsolutePath = Join-Path $resolvedProjectRoot $contentRelativePath

if (-not (Test-Path $contentAbsolutePath)) {
    Write-Host ("FAIL: Asset de camera ausente: {0} (config: {1})" -f $contentRelativePath, $defaultRigPath) -ForegroundColor Red
    Write-Host "Acao recomendada: criar o DataAsset UOmniCameraRigSpecDataAsset no caminho configurado." -ForegroundColor Yellow
    exit 1
}

Write-Host ("PASS: Asset de camera encontrado: {0}" -f $contentRelativePath) -ForegroundColor Green
exit 0
