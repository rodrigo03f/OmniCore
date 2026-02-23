param(
    [string]$ProjectRoot = "",
    [string[]]$SourceRoots = @("Plugins/Omni/Source"),
    [string[]]$AllowedDirectMapAccessFileSuffixes = @(
        "Plugins/Omni/Source/OmniCore/Private/Manifest/OmniManifest.cpp",
        "Plugins/Omni/Source/OmniCore/Public/Systems/OmniSystemMessaging.h",
        "Plugins/Omni/Source/OmniCore/Private/Systems/OmniSystemMessageSchemas.cpp"
    ),
    [string[]]$AllowedPublicTMapExposureFileSuffixes = @(
        "Plugins/Omni/Source/OmniCore/Public/Systems/OmniSystemMessaging.h"
    )
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

function To-NormalizedRelativePath {
    param(
        [string]$RootPath,
        [string]$FilePath
    )

    $normalizedRoot = [System.IO.Path]::GetFullPath($RootPath)
    $normalizedFile = [System.IO.Path]::GetFullPath($FilePath)

    if ($normalizedFile.StartsWith($normalizedRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        $relative = $normalizedFile.Substring($normalizedRoot.Length).TrimStart("\", "/")
        return $relative.Replace("\", "/")
    }

    return $normalizedFile.Replace("\", "/")
}

function Is-AllowedPath {
    param(
        [string]$RelativePath,
        [string[]]$AllowedSuffixes
    )

    $normalized = $RelativePath.Replace("\", "/")
    foreach ($suffix in $AllowedSuffixes) {
        $normalizedSuffix = $suffix.Replace("\", "/")
        if ($normalized.EndsWith($normalizedSuffix, [System.StringComparison]::OrdinalIgnoreCase)) {
            return $true
        }
    }

    return $false
}

$projectRoot = Resolve-ProjectRoot -InputRoot $ProjectRoot
Write-Host "Conformance gate root: $projectRoot" -ForegroundColor Green

$protectedMapNames = @("Settings", "Payload", "Arguments", "Output")
$mapMutationOrLookupOps = @("Add", "Find", "Contains")

$files = @()
foreach ($sourceRoot in $SourceRoots) {
    $absoluteSourceRoot = Join-Path $projectRoot $sourceRoot
    if (-not (Test-Path $absoluteSourceRoot)) {
        continue
    }

    $files += Get-ChildItem -Path $absoluteSourceRoot -Recurse -File -Include *.h,*.cpp
}

if ($files.Count -eq 0) {
    Write-Host "No source files found for the gate." -ForegroundColor DarkYellow
    exit 0
}

$directAccessPatterns = @()
foreach ($mapName in $protectedMapNames) {
    $directAccessPatterns += @{
        Name = "direct-field-dot-$mapName"
        Regex = "\.\s*$mapName\s*\."
    }
    $directAccessPatterns += @{
        Name = "direct-field-arrow-$mapName"
        Regex = "->\s*$mapName\s*\."
    }

    foreach ($op in $mapMutationOrLookupOps) {
        $directAccessPatterns += @{
            Name = "direct-op-dot-$mapName-$op"
            Regex = "\.\s*$mapName\s*\.\s*$op\s*\("
        }
        $directAccessPatterns += @{
            Name = "direct-op-arrow-$mapName-$op"
            Regex = "->\s*$mapName\s*\.\s*$op\s*\("
        }
    }

    $directAccessPatterns += @{
        Name = "direct-index-dot-$mapName"
        Regex = "\.\s*$mapName\s*\["
    }
    $directAccessPatterns += @{
        Name = "direct-index-arrow-$mapName"
        Regex = "->\s*$mapName\s*\["
    }
}

$mapGetterRefPatterns = @(
    @{
        Name = "map-ref-getter"
        Regex = "(?:const\s+)?TMap\s*<[^>]+>\s*&\s*Get(?:Settings|Payload|Arguments|Output)\s*\("
    }
)

function Get-PublicTMapFieldViolations {
    param(
        [System.IO.FileInfo]$File,
        [string]$RelativePath
    )

    $result = @()
    $currentAccess = "private"
    $lastDeclaredType = ""
    $lineNumber = 0

    foreach ($line in Get-Content -Path $File.FullName) {
        $lineNumber++

        if ($line -match "^\s*struct\b") {
            $lastDeclaredType = "struct"
            $currentAccess = "public"
        }
        elseif ($line -match "^\s*class\b") {
            $lastDeclaredType = "class"
            $currentAccess = "private"
        }

        if ($line -match "^\s*public\s*:") {
            $currentAccess = "public"
            continue
        }
        if ($line -match "^\s*private\s*:") {
            $currentAccess = "private"
            continue
        }
        if ($line -match "^\s*protected\s*:") {
            $currentAccess = "protected"
            continue
        }

        # Public structs default to public fields even without an explicit access label.
        if ($currentAccess -ne "public") {
            continue
        }

        if ($line -match "^\s*TMap\s*<[^>]+>\s+\w+\s*(?:=[^;]+)?;" -and $line -notmatch "\(") {
            $result += [PSCustomObject]@{
                File = $RelativePath
                Line = $lineNumber
                Rule = "public-tmap-field"
                Text = $line.Trim()
            }
        }
    }

    return $result
}

$violations = @()
foreach ($file in $files) {
    $relativePath = To-NormalizedRelativePath -RootPath $projectRoot -FilePath $file.FullName

    if (-not (Is-AllowedPath -RelativePath $relativePath -AllowedSuffixes $AllowedDirectMapAccessFileSuffixes)) {
        foreach ($pattern in $directAccessPatterns) {
            $matches = Select-String -Path $file.FullName -Pattern $pattern.Regex -CaseSensitive
            foreach ($match in $matches) {
                $violations += [PSCustomObject]@{
                    File = $relativePath
                    Line = $match.LineNumber
                    Rule = $pattern.Name
                    Text = $match.Line.Trim()
                }
            }
        }

        foreach ($pattern in $mapGetterRefPatterns) {
            $matches = Select-String -Path $file.FullName -Pattern $pattern.Regex -CaseSensitive
            foreach ($match in $matches) {
                $violations += [PSCustomObject]@{
                    File = $relativePath
                    Line = $match.LineNumber
                    Rule = $pattern.Name
                    Text = $match.Line.Trim()
                }
            }
        }
    }

    $isPublicHeader = $relativePath.Contains("/Public/") -and (
        $relativePath.EndsWith(".h", [System.StringComparison]::OrdinalIgnoreCase) -or
        $relativePath.EndsWith(".hpp", [System.StringComparison]::OrdinalIgnoreCase)
    )
    if ($isPublicHeader -and -not (Is-AllowedPath -RelativePath $relativePath -AllowedSuffixes $AllowedPublicTMapExposureFileSuffixes)) {
        $violations += Get-PublicTMapFieldViolations -File $file -RelativePath $relativePath
    }
}

if ($violations.Count -gt 0) {
    Write-Host "Conformance gate FAILED. B0 map-conformance violations were found." -ForegroundColor Red
    foreach ($violation in $violations) {
        Write-Host (" - {0}:{1} [{2}] {3}" -f $violation.File, $violation.Line, $violation.Rule, $violation.Text) -ForegroundColor Red
    }
    exit 1
}

Write-Host "Conformance gate PASSED. No B0 map-conformance violations found." -ForegroundColor Green
exit 0
