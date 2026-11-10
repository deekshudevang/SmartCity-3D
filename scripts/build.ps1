# SmartCity 3D — Build Script
param(
    [ValidateSet("Debug", "Release")]
    [string]$Config = "Debug",
    [switch]$Clean
)

$ErrorActionPreference = 'Stop'

# Ensure CMake and compiler are on PATH
if (Test-Path "C:\Users\deeks\tools\cmake\bin") {
    $env:PATH = "C:\Users\deeks\tools\cmake\bin;" + $env:PATH
}
if (Test-Path "C:\Users\deeks\tools\w64devkit\bin") {
    $cleanPath = ($env:PATH -split ';' | Where-Object { $_ -notmatch '(?i)mingw' }) -join ';'
    $env:PATH = "C:\Users\deeks\tools\w64devkit\bin;$cleanPath"
}

$RootDir = (Resolve-Path "$PSScriptRoot\..").Path
$BuildDir = Join-Path $RootDir "build"

Write-Host "=====================================================" -ForegroundColor Cyan
Write-Host " Building SmartCity 3D [$Config]                     " -ForegroundColor Cyan
Write-Host "=====================================================" -ForegroundColor Cyan

if ($Clean -and (Test-Path $BuildDir)) {
    Write-Host "Cleaning build directory $BuildDir..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force $BuildDir
}

if (-not (Test-Path $BuildDir)) {
    New-Item -ItemType Directory -Path $BuildDir | Out-Null
}

Push-Location $RootDir
try {
    # Check if Ninja is available, otherwise use MinGW Makefiles
    $generator = "MinGW Makefiles"
    if (Get-Command ninja -ErrorAction SilentlyContinue) {
        $generator = "Ninja"
    }

    Write-Host "Configuring CMake with generator '$generator'..." -ForegroundColor Cyan
    cmake -B build -G $generator -DCMAKE_BUILD_TYPE=$Config
    if ($LASTEXITCODE -ne 0) {
        throw "CMake configuration failed with exit code $LASTEXITCODE."
    }

    Write-Host "Compiling target SmartCity3D and test suite..." -ForegroundColor Cyan
    cmake --build build --config $Config -j
    if ($LASTEXITCODE -ne 0) {
        throw "Build failed with exit code $LASTEXITCODE."
    }

    Write-Host "[SUCCESS] Build completed successfully." -ForegroundColor Green
} finally {
    Pop-Location
}
