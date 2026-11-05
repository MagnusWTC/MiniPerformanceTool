param(
    [string]$QtRoot = 'C:\Qt\6.8.3\msvc2022_64',
    [switch]$SkipTests,
    [switch]$Deploy
)
$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path $PSScriptRoot -Parent
$vsRoot = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\2022\BuildTools'
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (Test-Path $vswhere) {
    $detected = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if ($detected) { $vsRoot = $detected.Trim() }
}
$vcvars = Join-Path $vsRoot 'VC\Auxiliary\Build\vcvars64.bat'
if (!(Test-Path $vcvars)) { throw 'Please install Visual Studio 2022 C++ Desktop Build Tools.' }
$compilerEnv = & cmd.exe /d /c "call `"$vcvars`" >nul && set"
if ($LASTEXITCODE -ne 0) { throw 'Failed to initialize MSVC environment.' }
foreach ($line in $compilerEnv) {
    if ($line -match '^([^=]+)=(.*)$') {
        [Environment]::SetEnvironmentVariable($matches[1], $matches[2], 'Process')
    }
}
$cmakeBin = Join-Path $vsRoot 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin'
$ninjaBin = Join-Path $vsRoot 'Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja'
$env:Path = "$cmakeBin;$ninjaBin;$QtRoot\bin;$env:Path"
$env:VSLANG = '1033'
$testOption = if ($SkipTests) { 'OFF' } else { 'ON' }
& cmake -S $repoRoot -B "$repoRoot\build" -G Ninja '-DCMAKE_BUILD_TYPE=Release' "-DCMAKE_PREFIX_PATH=$QtRoot" "-DBUILD_TESTING=$testOption"
if ($LASTEXITCODE -ne 0) { throw 'CMake configuration failed.' }
& cmake --build "$repoRoot\build" --parallel
if ($LASTEXITCODE -ne 0) { throw 'Build failed.' }
if (!$SkipTests) {
    & ctest --test-dir "$repoRoot\build" --output-on-failure
    if ($LASTEXITCODE -ne 0) { throw 'Tests failed.' }
}
if ($Deploy) {
    $packageDir = Join-Path $repoRoot 'dist'
    New-Item -ItemType Directory -Force $packageDir | Out-Null
    Copy-Item "$repoRoot\build\MiniPerformanceTool.exe" $packageDir
    & "$QtRoot\bin\windeployqt.exe" --release --no-translations --no-opengl-sw "$packageDir\MiniPerformanceTool.exe"
    if ($LASTEXITCODE -ne 0) { throw 'Qt deployment failed.' }
}
