# SmartCity 3D - Environment Setup Script
$ErrorActionPreference = 'Stop'

Write-Host "=====================================================" -ForegroundColor Cyan
Write-Host " SmartCity 3D - Environment Verification and Setup   " -ForegroundColor Cyan
Write-Host "=====================================================" -ForegroundColor Cyan

# 1. Check Git
try {
    $gitVersion = git --version
    Write-Host "[OK] Git: $gitVersion" -ForegroundColor Green
} catch {
    Write-Error "Git is required but was not found on PATH."
}

# 2. Check CMake
if (Test-Path "C:\Users\deeks\tools\cmake\bin\cmake.exe") {
    $env:PATH = "C:\Users\deeks\tools\cmake\bin;$env:PATH"
}
$cmakeCmd = Get-Command cmake -ErrorAction SilentlyContinue
if (-not $cmakeCmd) {
    Write-Error "CMake >= 3.21 is required but not found."
} else {
    $cmakeVer = cmake --version | Select-Object -First 1
    Write-Host "[OK] $cmakeVer" -ForegroundColor Green
}

# 3. Check C++ Toolchain (prioritizing 64-bit toolchain)
if (Test-Path "C:\Users\deeks\tools\w64devkit\bin\gcc.exe") {
    $cleanPath = ($env:PATH -split ';' | Where-Object { $_ -notmatch '(?i)mingw' }) -join ';'
    $env:PATH = "C:\Users\deeks\tools\w64devkit\bin;$cleanPath"
}

$clCmd = Get-Command cl -ErrorAction SilentlyContinue
$gccCmd = Get-Command gcc -ErrorAction SilentlyContinue

if ($clCmd) {
    Write-Host "[OK] MSVC compiler found: $($clCmd.Source)" -ForegroundColor Green
} elseif ($gccCmd) {
    Write-Host "[OK] GCC compiler found: $($gccCmd.Source)" -ForegroundColor Green
} else {
    Write-Error "No supported C++17 compiler (MSVC cl.exe or GCC 64-bit) found."
}

# 4. vcpkg integration check / bootstrap
if (-not $env:VCPKG_ROOT) {
    if (Test-Path "$PSScriptRoot\..\external\vcpkg\vcpkg.exe") {
        $env:VCPKG_ROOT = (Resolve-Path "$PSScriptRoot\..\external\vcpkg").Path
        Write-Host "[OK] VCPKG_ROOT set to $env:VCPKG_ROOT" -ForegroundColor Green
    } else {
        Write-Host "[INFO] Using vendored third-party dependencies (GLAD, GLFW 3.4, GLM, ImGui, STB)." -ForegroundColor Yellow
    }
} else {
    Write-Host "[OK] VCPKG_ROOT is $env:VCPKG_ROOT" -ForegroundColor Green
}

Write-Host "Setup verification complete. Development environment is ready!" -ForegroundColor Green
