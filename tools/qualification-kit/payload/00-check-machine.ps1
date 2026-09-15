[CmdletBinding()]
param(
    [switch]$ClassificationOnly,
    [string]$ReportPath
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
if ([string]::IsNullOrWhiteSpace($ReportPath)) {
    $ReportPath = Join-Path $PSScriptRoot 'evidence\environment-report.json'
}
. (Join-Path $PSScriptRoot 'Kit-Common.ps1')

try {
    $admin = Test-IsAdministrator
    $operatingSystem = Get-CimInstance Win32_OperatingSystem
    $computer = Get-CimInstance Win32_ComputerSystem
    $bios = Get-CimInstance Win32_BIOS
    $caption = [string]$operatingSystem.Caption
    $build = [string]$operatingSystem.BuildNumber
    $windows11 = $caption -match 'Windows 11' -and [int]$build -ge 22000
    $x64 = [Environment]::Is64BitOperatingSystem

    $tpmPresent = $false
    $tpmReady = $false
    $tpmQuery = 'unavailable'
    try {
        $tpm = Get-Tpm
        $tpmPresent = [bool]$tpm.TpmPresent
        $tpmReady = [bool]$tpm.TpmReady
        $tpmQuery = 'available'
    } catch {
        $tpmQuery = 'query_failed'
    }

    $specVersion = 'inconclusive'
    $tpm20 = $false
    try {
        $tpmCim = Get-CimInstance -Namespace 'Root\CIMV2\Security\MicrosoftTpm' -ClassName Win32_Tpm
        if ($null -ne $tpmCim -and -not [string]::IsNullOrWhiteSpace([string]$tpmCim.SpecVersion)) {
            $specVersion = [string]$tpmCim.SpecVersion
            $tpm20 = $specVersion -match '(^|[^0-9])2\.0([^0-9]|$)'
        }
    } catch {
        $specVersion = 'inconclusive'
    }

    $providerAvailable = $false
    try {
        $providerList = (& "$env:SystemRoot\System32\certutil.exe" -csplist 2>&1 | Out-String)
        $providerAvailable = $providerList -match [regex]::Escape('Microsoft Platform Crypto Provider')
    } catch {
        $providerAvailable = $false
    }

    $manufacturer = [string]$computer.Manufacturer
    $model = [string]$computer.Model
    $biosManufacturer = [string]$bios.Manufacturer
    $virtualPattern = '(?i)hyper-v|virtual machine|vmware|virtualbox|qemu|kvm|xen|parallels|bochs'
    $virtualSignals = "$manufacturer $model $biosManufacturer"
    $virtualMachineDetected = $virtualSignals -match $virtualPattern
    $requiredChecks = $windows11 -and $x64 -and $admin -and $tpmPresent -and $tpmReady -and $providerAvailable
    $qualificationStatus = if ($requiredChecks -and -not $virtualMachineDetected) { 'CANDIDATE_PHYSICAL_MACHINE' } else { 'DRY_RUN_ONLY' }
    $notices = @('NOT_VALID_FOR_T-A1-04')
    if ($qualificationStatus -eq 'DRY_RUN_ONLY') {
        $notices = @('DRY_RUN_ONLY', 'NOT_VALID_FOR_T-A1-04')
    } else {
        $notices += 'REVIEWER_PHYSICAL_MACHINE_CONFIRMATION_REQUIRED'
    }

    $report = [ordered]@{
        artifact_id = 'AXL-V1-A1-T-A1-04-environment'
        qualification_status = $qualificationStatus
        notices = $notices
        development_tools_required = @()
        checks = [ordered]@{
            windows_11 = $windows11
            x64 = $x64
            administrator = $admin
            tpm_present = $tpmPresent
            tpm_ready = $tpmReady
            tpm_2_0_where_determinable = if ($specVersion -eq 'inconclusive') { $null } else { $tpm20 }
            microsoft_platform_crypto_provider = $providerAvailable
            obvious_virtual_machine_signals_absent = (-not $virtualMachineDetected)
        }
        os = [ordered]@{
            caption = $caption
            build = $build
            architecture = [string]$operatingSystem.OSArchitecture
            powershell = [string]$PSVersionTable.PSVersion
        }
        tpm = [ordered]@{
            query = $tpmQuery
            spec_version = $specVersion
        }
        virtualization_signals = [ordered]@{
            detected = $virtualMachineDetected
            manufacturer = $manufacturer
            model = $model
            bios_manufacturer = $biosManufacturer
        }
        physical_tpm_attestation = [ordered]@{
            automatic_detection = 'inconclusive'
            reviewer_physical_machine_confirmation_required = $true
            reviewer_physical_machine_confirmation = $false
        }
    }
    Write-KitJson $report $ReportPath
    if ($qualificationStatus -eq 'DRY_RUN_ONLY') {
        Write-Host 'DRY_RUN_ONLY'
        Write-Host 'NOT_VALID_FOR_T-A1-04'
    } else {
        Write-Host 'physical_tpm_attestation.automatic_detection: inconclusive'
        Write-Host 'physical_tpm_attestation.reviewer_physical_machine_confirmation: required'
    }
    [ordered]@{ qualification_status = $qualificationStatus; report = $ReportPath } | ConvertTo-Json -Compress
    if (-not $ClassificationOnly -and $qualificationStatus -ne 'CANDIDATE_PHYSICAL_MACHINE') {
        exit 3
    }
} catch {
    [Console]::Error.WriteLine("MACHINE_CHECK_FAILED: $($_.Exception.Message)")
    exit 1
}
