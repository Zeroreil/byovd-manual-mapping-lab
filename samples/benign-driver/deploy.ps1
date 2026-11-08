#requires -RunAsAdministrator
<#
.SYNOPSIS
    Deploy/cleanup the benign LabDrv test-signed kernel driver.

.DESCRIPTION
    This script only loads the benign LabDrv.sys (PING/VERSION). It does not
    use a vulnerable driver, manual mapping, or any hiding technique.

.PARAMETER Cleanup
    Stop/delete the service and remove the staged driver.

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File .\deploy.ps1

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File .\deploy.ps1 -Cleanup
#>
param(
    [switch]$Cleanup,
    [string]$DriverPath = (Join-Path $PSScriptRoot 'LabDrv.sys'),
    [string]$ClientPath = (Join-Path $PSScriptRoot 'labctl.exe'),
    [string]$DestDir = 'C:\Lab'
)

$ErrorActionPreference = 'Stop'
$serviceName = 'LabDrv'

function Stop-LabService {
    sc.exe stop $serviceName 2>$null | Out-Null
    sc.exe delete $serviceName 2>$null | Out-Null
    Start-Sleep -Milliseconds 300
}

if ($Cleanup) {
    Write-Host "== cleanup =="
    Stop-LabService
    if (Test-Path (Join-Path $DestDir 'LabDrv.sys')) {
        Remove-Item (Join-Path $DestDir 'LabDrv.sys') -Force
    }
    Write-Host "cleanup done"
    exit 0
}

if (-not (Test-Path $DriverPath)) {
    throw "LabDrv.sys not found: $DriverPath"
}
if (-not (Test-Path $ClientPath)) {
    throw "labctl.exe not found: $ClientPath"
}

$bcd = (bcdedit /enum "{current}" 2>&1 | Out-String)
if ($bcd -notmatch 'testsigning\s+Yes') {
    Write-Warning 'testsigning does not look enabled; the driver may fail to load (error 577).'
}

Write-Host "== [1/4] stage =="
New-Item -ItemType Directory -Path $DestDir -Force | Out-Null
Copy-Item $DriverPath (Join-Path $DestDir 'LabDrv.sys') -Force
Write-Host "  $DestDir\LabDrv.sys"

Write-Host "== [2/4] service =="
Stop-LabService
sc.exe create $serviceName type= kernel start= demand binPath= "$DestDir\LabDrv.sys" | Out-Host
if ($LASTEXITCODE -ne 0) { throw "sc create failed: $LASTEXITCODE" }

sc.exe start $serviceName | Out-Host
if ($LASTEXITCODE -ne 0) {
    sc.exe query $serviceName | Out-Host
    throw "sc start failed: $LASTEXITCODE"
}

Write-Host "== [3/4] query =="
sc.exe query $serviceName | Out-Host

Write-Host "== [4/4] IOCTL client =="
& $ClientPath all
if ($LASTEXITCODE -ne 0) {
    Write-Warning "labctl returned $LASTEXITCODE"
}

Write-Host ""
Write-Host "deployed. cleanup with: .\deploy.ps1 -Cleanup"
