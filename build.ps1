<#
.SYNOPSIS
    Build script for MoonAvoidance plugin on Windows.

.DESCRIPTION
    This script automates the CMake configuration, build, and install process.
    It prompts for required paths if not provided.

.PARAMETER QtPath
    Path to the Qt installation (e.g., C:\Qt\6.5.3\msvc2019_64).

.PARAMETER StelRoot
    Path to the Stellarium source root.

.PARAMETER Clean
    Switch to clean the build directory before building.

.EXAMPLE
    .\build.ps1 -QtPath "C:\Qt\6.5.3\msvc2019_64"
#>

param (
    [string]$QtPath = "",
    [string]$StelRoot = "",
    [switch]$Clean
)

$BuildDir = "build"

Write-Host "=== MoonAvoidance Build Helper (Windows) ===" -ForegroundColor Cyan

# Check for cmake
if (-not (Get-Command "cmake" -ErrorAction SilentlyContinue)) {
    Write-Error "cmake is not found in PATH. Please install CMake."
    exit 1
}

# Prompt for QtPath if not provided
if ([string]::IsNullOrWhiteSpace($QtPath)) {
    Write-Host "Please enter the path to your Qt installation (e.g., C:\Qt\6.5.3\msvc2019_64):"
    $QtPath = Read-Host
}

# Validate QtPath
if ([string]::IsNullOrWhiteSpace($QtPath)) {
    Write-Error "Qt path is required."
    exit 1
}

if (-not (Test-Path $QtPath)) {
    Write-Error "Directory '$QtPath' does not exist."
    exit 1
}

# Prompt for StelRoot if not provided
if ([string]::IsNullOrWhiteSpace($StelRoot)) {
    Write-Host "Please enter the path to Stellarium source root (leave empty if using installed Stellarium/environment variables):"
    $StelRoot = Read-Host
}

# Clean build directory
if ($Clean) {
    if (Test-Path $BuildDir) {
        Write-Host "Cleaning build directory..." -ForegroundColor Yellow
        Remove-Item -Path $BuildDir -Recurse -Force
    }
}

# Create build directory
if (-not (Test-Path $BuildDir)) {
    New-Item -ItemType Directory -Path $BuildDir | Out-Null
}

Set-Location $BuildDir

# Construct arguments
$cmakeArgs = @("..", "-DCMAKE_PREFIX_PATH=`"$QtPath`"")
if (-not [string]::IsNullOrWhiteSpace($StelRoot)) {
    $cmakeArgs += "-DSTELROOT=`"$StelRoot`""
}

# Configure
Write-Host "`nConfiguring CMake..." -ForegroundColor Green
Write-Host "cmake $cmakeArgs"
& cmake $cmakeArgs
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

# Build
Write-Host "`nBuilding..." -ForegroundColor Green
& cmake --build . --config Release
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

# Install
Write-Host "`nInstalling..." -ForegroundColor Green
& cmake --install . --config Release
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "`nBuild complete!" -ForegroundColor Cyan

