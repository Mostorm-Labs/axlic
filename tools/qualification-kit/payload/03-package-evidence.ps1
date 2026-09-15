[CmdletBinding()]
param([Parameter(Mandatory = $true)][ValidatePattern('^[0-9a-fA-F-]{36}$')][string]$CorrelationId)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'Kit-Common.ps1')

try {
    $manifest = Assert-KitBinding $PSScriptRoot
    $initial = Read-KitJson (Join-Path $PSScriptRoot 'evidence\physical-tpm-initial.json')
    $post = Read-KitJson (Join-Path $PSScriptRoot 'evidence\physical-tpm-post-reboot.json')
    $session = Read-KitJson (Join-Path $PSScriptRoot 'output\qualification-session.json')
    if ($initial.correlation_id -ne $CorrelationId -or $post.correlation_id -ne $CorrelationId -or
        $session.correlation_id -ne $CorrelationId -or $post.result -ne 'PASS' -or
        $initial.implementation_revision -ne $manifest.source_revision -or
        $post.implementation_revision -ne $manifest.source_revision) {
        throw 'EVIDENCE_BINDING_INVALID'
    }

    $bundleName = "AXL-V1-A1-T-A1-04-Physical-TPM-Evidence-$CorrelationId"
    $stage = Join-Path (Join-Path $PSScriptRoot 'output') $bundleName
    $archive = "$stage.zip"
    if ((Test-Path -LiteralPath $stage) -or (Test-Path -LiteralPath $archive)) {
        throw "EVIDENCE_OUTPUT_ALREADY_EXISTS: $bundleName"
    }
    New-Item -ItemType Directory -Path $stage | Out-Null
    foreach ($file in @(
        'evidence\physical-tpm-initial.json',
        'evidence\physical-tpm-post-reboot.json',
        'evidence\environment-initial.json',
        'evidence\environment-post-reboot.json',
        'output\qualification-session.json',
        'manifest\build-manifest.json',
        'manifest\toolchain-report.json',
        'manifest\runtime-dependencies.txt',
        'manifest\SHA256SUMS.txt'
    )) {
        Copy-Item -LiteralPath (Join-Path $PSScriptRoot $file) -Destination $stage
    }
    $summary = [ordered]@{
        artifact_id = 'EV-03-A1-physical-TPM-complete'
        source_revision = $manifest.source_revision
        axlic_exe_sha256 = $manifest.axlic_exe_sha256
        correlation_id = $CorrelationId
        initial_boot_marker = $session.initial_boot_marker
        post_reboot_boot_marker = $post.boot_marker
        public_identity_sha256 = $session.public_identity_sha256
        scheme_id = $post.scheme_id
        identity_epoch = $post.identity_epoch
        t_a1_04_execution = 'PASS'
        control_review = 'PENDING'
        p34 = 'NOT_EVALUATED'
    }
    Write-KitJson $summary (Join-Path $stage 'evidence-summary.json')
    Compress-Archive -LiteralPath $stage -DestinationPath $archive -CompressionLevel Optimal
    $hash = Get-LowerSha256 $archive
    "$hash  $([System.IO.Path]::GetFileName($archive))" | Set-Content -LiteralPath "$archive.sha256" -Encoding ASCII
    [ordered]@{ status = 'EVIDENCE_ZIP_READY'; path = $archive; sha256 = $hash } | ConvertTo-Json -Compress
} catch {
    [Console]::Error.WriteLine("EVIDENCE_PACKAGING_FAILED: $($_.Exception.Message)")
    exit 1
}
