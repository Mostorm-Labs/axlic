[CmdletBinding()]
param([string]$KitRoot = '')

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'Kit-Common.ps1')

try {
    if ([string]::IsNullOrWhiteSpace($KitRoot)) {
        $KitRoot = $PSScriptRoot
    }
    $root = [System.IO.Path]::GetFullPath($KitRoot)
    $manifest = Assert-KitBinding $root
    $sumFile = Join-Path $root 'manifest\SHA256SUMS.txt'
    foreach ($line in Get-Content -LiteralPath $sumFile) {
        if ([string]::IsNullOrWhiteSpace($line)) { continue }
        if ($line -notmatch '^(?<hash>[0-9a-f]{64})  (?<path>.+)$') {
            throw "HASH_MANIFEST_INVALID: $line"
        }
        $relative = $Matches['path'].Replace('/', [System.IO.Path]::DirectorySeparatorChar)
        $target = [System.IO.Path]::GetFullPath((Join-Path $root $relative))
        if (-not $target.StartsWith($root + [System.IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
            throw "HASH_PATH_OUTSIDE_KIT: $relative"
        }
        if (-not (Test-Path -LiteralPath $target -PathType Leaf)) {
            throw "HASH_TARGET_MISSING: $relative"
        }
        $actual = Get-LowerSha256 $target
        if ($actual -ne $Matches['hash']) {
            throw "HASH_MISMATCH: $relative"
        }
    }
    [ordered]@{
        status = 'PASS'
        source_revision = $manifest.source_revision
        axlic_exe_sha256 = $manifest.axlic_exe_sha256
    } | ConvertTo-Json -Compress
} catch {
    [Console]::Error.WriteLine("VERIFY_KIT_FAILED: $($_.Exception.Message)")
    exit 1
}
