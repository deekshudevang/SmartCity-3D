# SmartCity 3D - Run Script
param(
    [ValidateSet("Debug", "Release")]
    [string]$Config = "Debug",
    [string[]]$AppArgs = @()
)

$ErrorActionPreference = 'Stop'

$RootDir = (Resolve-Path "$PSScriptRoot\..").Path
$ExePath = Join-Path $RootDir "build\SmartCity3D.exe"

if (-not (Test-Path $ExePath)) {
    Write-Host "Executable not found. Initiating build first..." -ForegroundColor Yellow
    & "$PSScriptRoot\build.ps1" -Config $Config
}

Write-Host "=====================================================" -ForegroundColor Cyan
Write-Host " Launching SmartCity 3D from $RootDir                 " -ForegroundColor Cyan
Write-Host "=====================================================" -ForegroundColor Cyan

Push-Location $RootDir
try {
    & $ExePath $AppArgs
    $code = $LASTEXITCODE
    if ($code -ne 0) {
        Write-Error "Application exited with code $code"
    }
} finally {
    Pop-Location
}
