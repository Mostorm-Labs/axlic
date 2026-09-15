[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$AxlicExe,
    [Parameter(Mandatory = $true)][string]$PythonRuntimeDirectory,
    [Parameter(Mandatory = $true)][ValidatePattern('^[0-9a-fA-F]{64}$')][string]$PythonPackageSha256,
    [Parameter(Mandatory = $true)][string]$OraclePath,
    [Parameter(Mandatory = $true)][string]$VcRuntimeDirectory,
    [Parameter(Mandatory = $true)][string]$RuntimeDependencyReport,
    [Parameter(Mandatory = $true)][string]$OutputDirectory,
    [Parameter(Mandatory = $true)][ValidatePattern('^[0-9a-f]{40}$')][string]$SourceRevision,
    [Parameter(Mandatory = $true)][ValidatePattern('^[0-9a-f]{40}$')][string]$PackagingRevision,
    [Parameter(Mandatory = $true)][string]$BuildRun,
    [Parameter(Mandatory = $true)][string]$BuildAttempt,
    [Parameter(Mandatory = $true)][string]$BuildJob,
    [Parameter(Mandatory = $true)][string]$CMakeVersion,
    [Parameter(Mandatory = $true)][string]$MsvcVersion,
    [Parameter(Mandatory = $true)][string]$ToolsetVersion,
    [Parameter(Mandatory = $true)][string]$WindowsSdk
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Get-Sha256 {
    param([Parameter(Mandatory = $true)][string]$Path)
    $stream = [System.IO.File]::OpenRead($Path)
    $sha = [System.Security.Cryptography.SHA256]::Create()
    try {
        return ([System.BitConverter]::ToString($sha.ComputeHash($stream))).Replace('-', '').ToLowerInvariant()
    } finally {
        $sha.Dispose()
        $stream.Dispose()
    }
}

$expectedSourceRevision = 'a1a37d07d421320d95b7d49ed94ad0ffa7c3cd25'
if ($SourceRevision -ne $expectedSourceRevision) {
    throw "SOURCE_REVISION_MISMATCH: expected $expectedSourceRevision"
}

$requiredInputs = @($AxlicExe, $PythonRuntimeDirectory, $OraclePath, $VcRuntimeDirectory, $RuntimeDependencyReport)
foreach ($inputPath in $requiredInputs) {
    if (-not (Test-Path -LiteralPath $inputPath)) {
        throw "MISSING_INPUT: $inputPath"
    }
}

$outputRoot = [System.IO.Path]::GetFullPath($OutputDirectory)
if (-not (Test-Path -LiteralPath $outputRoot)) {
    New-Item -ItemType Directory -Path $outputRoot | Out-Null
}
$kitName = 'AXL-V1-A1-Physical-TPM-Runtime-Kit'
$kitRoot = Join-Path $outputRoot $kitName
$archivePath = Join-Path $outputRoot 'AXL-V1-A1-Physical-TPM-Runtime-Kit-a1a37d07.zip'
if ((Test-Path -LiteralPath $kitRoot) -or (Test-Path -LiteralPath $archivePath)) {
    throw "OUTPUT_ALREADY_EXISTS: $outputRoot"
}

$payloadRoot = Join-Path $PSScriptRoot 'payload'
$payloadFiles = @(
    'README_CN.md',
    'START-INITIAL.cmd',
    'START-POST-REBOOT.cmd',
    'VERIFY-KIT.ps1',
    '00-check-machine.ps1',
    '01-run-initial.ps1',
    '02-run-post-reboot.ps1',
    '03-package-evidence.ps1',
    'Kit-Common.ps1'
)
foreach ($payloadFile in $payloadFiles) {
    if (-not (Test-Path -LiteralPath (Join-Path $payloadRoot $payloadFile))) {
        throw "MISSING_PAYLOAD: $payloadFile"
    }
}

New-Item -ItemType Directory -Path $kitRoot | Out-Null
foreach ($directory in @('bin', 'runtime', 'oracle', 'manifest', 'evidence', 'output')) {
    New-Item -ItemType Directory -Path (Join-Path $kitRoot $directory) | Out-Null
}
foreach ($payloadFile in $payloadFiles) {
    Copy-Item -LiteralPath (Join-Path $payloadRoot $payloadFile) -Destination (Join-Path $kitRoot $payloadFile)
}
Copy-Item -LiteralPath $AxlicExe -Destination (Join-Path $kitRoot 'bin\axlic.exe')
Copy-Item -LiteralPath $PythonRuntimeDirectory -Destination (Join-Path $kitRoot 'runtime') -Recurse
Copy-Item -LiteralPath $VcRuntimeDirectory -Destination (Join-Path $kitRoot 'runtime') -Recurse
Copy-Item -LiteralPath $OraclePath -Destination (Join-Path $kitRoot 'oracle\a1_physical_tpm.py')
Copy-Item -LiteralPath $RuntimeDependencyReport -Destination (Join-Path $kitRoot 'manifest\runtime-dependencies.txt')
Set-Content -LiteralPath (Join-Path $kitRoot 'evidence\README.txt') -Value 'Runtime evidence is written here.' -Encoding UTF8
Set-Content -LiteralPath (Join-Path $kitRoot 'output\README.txt') -Value 'The final evidence ZIP is written here.' -Encoding UTF8

$axlicHash = Get-Sha256 $AxlicExe
$vcFiles = @()
Get-ChildItem -LiteralPath $VcRuntimeDirectory -File -Recurse | Sort-Object FullName | ForEach-Object {
    $vcFiles += [ordered]@{
        path = $_.Name
        sha256 = Get-Sha256 $_.FullName
    }
}
$buildManifest = [ordered]@{
    artifact_id = 'AXL-V1-A1-Physical-TPM-Runtime-Kit'
    source_repository = 'github/Mostorm-Labs/axlic'
    source_revision = $SourceRevision
    packaging_revision = $PackagingRevision
    build = [ordered]@{
        run = $BuildRun
        attempt = $BuildAttempt
        job = $BuildJob
    }
    axlic_exe_sha256 = $axlicHash
    target = [ordered]@{
        architecture = 'x64'
        configuration = 'Release'
        cpp_standard = 'C++20'
    }
    python = [ordered]@{
        bundled = $true
        version = '3.13.15'
        architecture = 'x64'
        distribution = 'official Windows embeddable package'
        package_sha256 = $PythonPackageSha256.ToLowerInvariant()
    }
    vc_runtime = [ordered]@{
        strategy = 'app-local Microsoft VC143 CRT'
        files = $vcFiles
    }
    qualification_semantics = [ordered]@{
        t_a1_04 = 'STILL_PENDING_PHYSICAL_MACHINE'
        p34 = 'NOT_EVALUATED'
    }
}
$buildManifest | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath (Join-Path $kitRoot 'manifest\build-manifest.json') -Encoding UTF8

$toolchainReport = [ordered]@{
    source_revision = $SourceRevision
    cmake = $CMakeVersion
    msvc = $MsvcVersion
    toolset = $ToolsetVersion
    windows_sdk = $WindowsSdk
    generator = 'Visual Studio 17 2022'
    platform = 'x64'
    configuration = 'Release'
}
$toolchainReport | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $kitRoot 'manifest\toolchain-report.json') -Encoding UTF8

$hashLines = @()
Get-ChildItem -LiteralPath $kitRoot -File -Recurse | Where-Object {
    $_.FullName -ne (Join-Path $kitRoot 'manifest\SHA256SUMS.txt')
} | Sort-Object FullName | ForEach-Object {
    $relative = $_.FullName.Substring($kitRoot.Length + 1).Replace('\', '/')
    $hash = Get-Sha256 $_.FullName
    $hashLines += "$hash  $relative"
}
$hashLines | Set-Content -LiteralPath (Join-Path $kitRoot 'manifest\SHA256SUMS.txt') -Encoding ASCII

Compress-Archive -LiteralPath $kitRoot -DestinationPath $archivePath -CompressionLevel Optimal
$kitHash = Get-Sha256 $archivePath
[ordered]@{
    zip_path = $archivePath
    kit_sha256 = $kitHash
    axlic_sha256 = $axlicHash
    source_revision = $SourceRevision
} | ConvertTo-Json -Compress
