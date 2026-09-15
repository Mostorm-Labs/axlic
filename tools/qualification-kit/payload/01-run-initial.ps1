[CmdletBinding()]
param([switch]$ReviewerConfirmsPhysicalMachine)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'Kit-Common.ps1')

try {
    & (Join-Path $PSScriptRoot 'VERIFY-KIT.ps1') | Out-Host
    if ($LASTEXITCODE -ne 0) { throw 'KIT_INTEGRITY_FAILED' }
    $manifest = Assert-KitBinding $PSScriptRoot
    $environmentPath = Join-Path $PSScriptRoot 'evidence\environment-initial.json'
    & (Join-Path $PSScriptRoot '00-check-machine.ps1') -ClassificationOnly -ReportPath $environmentPath | Out-Host
    if ($LASTEXITCODE -ne 0) { throw 'ENVIRONMENT_CHECK_FAILED' }
    $environment = Read-KitJson $environmentPath
    if ($environment.qualification_status -ne 'CANDIDATE_PHYSICAL_MACHINE') {
        throw 'DRY_RUN_ONLY: NOT_VALID_FOR_T-A1-04'
    }

    $confirmed = [bool]$ReviewerConfirmsPhysicalMachine
    if (-not $confirmed) {
        $answer = Read-Host '请确认这是 Windows 11 物理机器且使用物理 TPM 2.0（输入 YES 继续）'
        $confirmed = $answer -eq 'YES'
    }
    if (-not $confirmed) {
        throw 'REVIEWER_PHYSICAL_MACHINE_CONFIRMATION_REQUIRED'
    }
    $environment.physical_tpm_attestation.reviewer_physical_machine_confirmation = $true
    Write-KitJson $environment $environmentPath

    $exe = Join-Path $PSScriptRoot 'bin\axlic.exe'
    $identityCall = Invoke-CapturedProcess $exe @('identity')
    if ($identityCall.ExitCode -ne 0 -or -not [string]::IsNullOrWhiteSpace($identityCall.Stderr)) {
        throw "AXLIC_IDENTITY_FAILED: exit=$($identityCall.ExitCode)"
    }
    $identityResult = $identityCall.Stdout | ConvertFrom-Json
    if ($identityResult.code -ne 'OK' -or $identityResult.data.scheme_id -ne 'axl-win-cng-tpm-p256-v1' -or
        [int]$identityResult.data.identity_epoch -ne 1) {
        throw 'TPM_IDENTITY_NOT_SELECTED'
    }
    $identityValue = [string]$identityResult.data.identity_value
    $identityHash = Get-AsciiSha256 $identityValue

    $correlation = [guid]::NewGuid().ToString('D')
    $python = Join-Path $PSScriptRoot 'runtime\python-3.13.15-embed-amd64\python.exe'
    $oracle = Join-Path $PSScriptRoot 'oracle\a1_physical_tpm.py'
    $initialReport = Join-Path $PSScriptRoot 'evidence\physical-tpm-initial.json'
    $oracleCall = Invoke-CapturedProcess $python @(
        $oracle, '--exe', $exe, '--phase', 'initial', '--correlation-id', $correlation,
        '--implementation-revision', $manifest.source_revision, '--report', $initialReport
    )
    if ($oracleCall.ExitCode -ne 0) {
        throw "FROZEN_ORACLE_FAILED: $($oracleCall.Stderr.Trim())"
    }
    $oracleResult = $oracleCall.Stdout.Trim() | ConvertFrom-Json
    if ($oracleResult.public_identity_sha256 -ne $identityHash -or $oracleResult.result -ne 'PENDING_POST_REBOOT') {
        throw 'INITIAL_ORACLE_RESULT_INVALID'
    }

    $session = [ordered]@{
        correlation_id = $correlation
        public_identity_sha256 = $oracleResult.public_identity_sha256
        initial_boot_marker = $oracleResult.boot_marker
        source_revision = $manifest.source_revision
        axlic_exe_sha256 = $manifest.axlic_exe_sha256
        reviewer_physical_machine_confirmation = $true
    }
    Write-KitJson $session (Join-Path $PSScriptRoot 'output\qualification-session.json')

    Write-Host ''
    Write-Host 'INITIAL 阶段完成。'
    Write-Host ''
    Write-Host '现在请真实重启这台 Windows 11 物理机器。'
    Write-Host ''
    Write-Host '不要注销。'
    Write-Host '不要休眠。'
    Write-Host '不要恢复虚拟机快照。'
    Write-Host ''
    Write-Host '重启完成后运行：'
    Write-Host 'START-POST-REBOOT.cmd'
} catch {
    [Console]::Error.WriteLine("INITIAL_FAILED: $($_.Exception.Message)")
    exit 1
}
