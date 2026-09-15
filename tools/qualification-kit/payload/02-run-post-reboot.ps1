[CmdletBinding()]
param()

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'Kit-Common.ps1')

try {
    & (Join-Path $PSScriptRoot 'VERIFY-KIT.ps1') | Out-Host
    if ($LASTEXITCODE -ne 0) { throw 'KIT_INTEGRITY_FAILED' }
    $manifest = Assert-KitBinding $PSScriptRoot
    $sessionPath = Join-Path $PSScriptRoot 'output\qualification-session.json'
    $session = Read-KitJson $sessionPath
    if ($session.source_revision -ne $manifest.source_revision -or
        $session.axlic_exe_sha256 -ne $manifest.axlic_exe_sha256 -or
        -not [bool]$session.reviewer_physical_machine_confirmation) {
        throw 'INITIAL_SESSION_BINDING_INVALID'
    }

    $environmentPath = Join-Path $PSScriptRoot 'evidence\environment-post-reboot.json'
    & (Join-Path $PSScriptRoot '00-check-machine.ps1') -ClassificationOnly -ReportPath $environmentPath | Out-Host
    if ($LASTEXITCODE -ne 0) { throw 'ENVIRONMENT_CHECK_FAILED' }
    $environment = Read-KitJson $environmentPath
    if ($environment.qualification_status -ne 'CANDIDATE_PHYSICAL_MACHINE') {
        throw 'DRY_RUN_ONLY: NOT_VALID_FOR_T-A1-04'
    }
    $environment.physical_tpm_attestation.reviewer_physical_machine_confirmation = $true
    Write-KitJson $environment $environmentPath

    $exe = Join-Path $PSScriptRoot 'bin\axlic.exe'
    $python = Join-Path $PSScriptRoot 'runtime\python-3.13.15-embed-amd64\python.exe'
    $oracle = Join-Path $PSScriptRoot 'oracle\a1_physical_tpm.py'
    $postReport = Join-Path $PSScriptRoot 'evidence\physical-tpm-post-reboot.json'
    $oracleCall = Invoke-CapturedProcess $python @(
        $oracle, '--exe', $exe, '--phase', 'post-reboot', '--correlation-id', [string]$session.correlation_id,
        '--implementation-revision', [string]$session.source_revision,
        '--expected-identity-sha256', [string]$session.public_identity_sha256,
        '--initial-boot-marker', [string]$session.initial_boot_marker,
        '--report', $postReport
    )
    if ($oracleCall.ExitCode -ne 0) {
        throw "FROZEN_ORACLE_FAILED: $($oracleCall.Stderr.Trim())"
    }
    $oracleResult = $oracleCall.Stdout.Trim() | ConvertFrom-Json
    if ($oracleResult.result -ne 'PASS' -or $oracleResult.public_identity_sha256 -ne $session.public_identity_sha256) {
        throw 'POST_REBOOT_ORACLE_RESULT_INVALID'
    }

    & (Join-Path $PSScriptRoot '03-package-evidence.ps1') -CorrelationId ([string]$session.correlation_id) | Out-Host
    if ($LASTEXITCODE -ne 0) { throw 'EVIDENCE_PACKAGING_FAILED' }
    Write-Host ''
    Write-Host 'POST-REBOOT 阶段完成。请将 output 目录中的 Evidence ZIP 上传回 ChatGPT/Aegis 控制会话。'
} catch {
    [Console]::Error.WriteLine("POST_REBOOT_FAILED: $($_.Exception.Message)")
    exit 1
}
