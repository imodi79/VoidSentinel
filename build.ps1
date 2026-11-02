param([ValidateSet('Debug','Release')][string]$Configuration = 'Release')
$ErrorActionPreference = 'Stop'
$vswherePath = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path -LiteralPath $vswherePath)) { throw 'Visual Studio Installer was not found.' }
$installation = & $vswherePath -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $installation) { throw 'Install Desktop development with C++ in Visual Studio Installer.' }
$cmakePath = Join-Path $installation 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
if (-not (Test-Path -LiteralPath $cmakePath)) { throw 'Install C++ CMake tools for Windows.' }
& $cmakePath -S $PSScriptRoot -B "$PSScriptRoot\out" -A x64
if ($LASTEXITCODE -ne 0) { throw 'CMake configuration failed.' }
& $cmakePath --build "$PSScriptRoot\out" --config $Configuration
if ($LASTEXITCODE -ne 0) { throw 'Compilation failed.' }
Write-Output "Executable: $PSScriptRoot\out\$Configuration\VoidSentinel.exe"
