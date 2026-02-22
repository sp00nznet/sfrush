# San Francisco Rush: Extreme Racing - Recompiled
# Windows Build Script (PowerShell)
#
# Prerequisites:
#   - Visual Studio 2022 with C++ workload
#   - LLVM/Clang (for shader preprocessing)
#   - CMake 3.20+
#   - Ninja

param(
    [string]$BuildType = "RelWithDebInfo",
    [switch]$Clean
)

$ErrorActionPreference = "Stop"

# Find Visual Studio
$vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (Test-Path $vsWhere) {
    $vsPath = & $vsWhere -latest -property installationPath
    $vcvarsall = Join-Path $vsPath "VC\Auxiliary\Build\vcvarsall.bat"
} else {
    Write-Error "Visual Studio not found. Install VS 2022 with C++ workload."
    exit 1
}

# Import MSVC environment
Write-Host "Importing MSVC environment..." -ForegroundColor Cyan
cmd /c "`"$vcvarsall`" x64 && set" | ForEach-Object {
    if ($_ -match "^(.+?)=(.*)$") {
        [System.Environment]::SetEnvironmentVariable($matches[1], $matches[2])
    }
}

# Add LLVM to PATH if available
$llvmPath = "${env:ProgramFiles}\LLVM\bin"
if (Test-Path $llvmPath) {
    $env:PATH = "$llvmPath;$env:PATH"
    Write-Host "LLVM found at $llvmPath" -ForegroundColor Green
}

# Build directory
$buildDir = Join-Path $PSScriptRoot "build"

if ($Clean -and (Test-Path $buildDir)) {
    Write-Host "Cleaning build directory..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force $buildDir
}

if (-not (Test-Path $buildDir)) {
    New-Item -ItemType Directory -Path $buildDir | Out-Null
}

# Configure
Write-Host "`nConfiguring ($BuildType)..." -ForegroundColor Cyan
cmake -S $PSScriptRoot -B $buildDir `
    -G Ninja `
    -DCMAKE_BUILD_TYPE=$BuildType `
    -DCMAKE_C_COMPILER=clang-cl `
    -DCMAKE_CXX_COMPILER=clang-cl `
    -DCMAKE_LINKER=lld-link

if ($LASTEXITCODE -ne 0) {
    Write-Error "CMake configure failed"
    exit 1
}

# Build
Write-Host "`nBuilding..." -ForegroundColor Cyan
cmake --build $buildDir --config $BuildType

if ($LASTEXITCODE -ne 0) {
    Write-Error "Build failed"
    exit 1
}

Write-Host "`nBuild complete!" -ForegroundColor Green
Write-Host "Output: $buildDir\SFRushRecompiled.exe" -ForegroundColor Green
