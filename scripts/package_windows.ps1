param(
    [string]$BuildDir = 'build-release',
    [Parameter(Mandatory = $true)][string]$QtRoot,
    [string]$OutputDir = 'dist/TechDictionary-v0.2.0-win64'
)

$ErrorActionPreference = 'Stop'
$projectRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$buildPath = [System.IO.Path]::GetFullPath((Join-Path $projectRoot $BuildDir))
$outputPath = [System.IO.Path]::GetFullPath((Join-Path $projectRoot $OutputDir))
$exe = Join-Path $buildPath 'TechDictionary.exe'
$seed = Join-Path $projectRoot 'data/dictionary.txt'
$deployTool = Join-Path $QtRoot 'bin/windeployqt.exe'
$offscreenPlugin = Join-Path $QtRoot 'plugins/platforms/qoffscreen.dll'
$zipPath = "$outputPath.zip"

foreach ($required in @($exe, $seed, $deployTool, $offscreenPlugin)) {
    if (-not (Test-Path -LiteralPath $required -PathType Leaf)) {
        throw "Required file missing: $required"
    }
}
if ((Test-Path -LiteralPath $outputPath) -or (Test-Path -LiteralPath $zipPath)) {
    throw "Output already exists: $outputPath or $zipPath"
}

New-Item -ItemType Directory -Path (Join-Path $outputPath 'data') -Force | Out-Null
Copy-Item -LiteralPath $exe -Destination (Join-Path $outputPath 'TechDictionary.exe')
Copy-Item -LiteralPath $seed -Destination (Join-Path $outputPath 'data/dictionary.txt')
& $deployTool --release --compiler-runtime --dir $outputPath (Join-Path $outputPath 'TechDictionary.exe')
if ($LASTEXITCODE -ne 0) { throw "windeployqt failed with exit code $LASTEXITCODE" }
Copy-Item -LiteralPath $offscreenPlugin -Destination (Join-Path $outputPath 'platforms/qoffscreen.dll')

foreach ($relative in @('TechDictionary.exe', 'data/dictionary.txt', 'Qt6Core.dll',
                        'Qt6Gui.dll', 'Qt6Widgets.dll', 'platforms/qwindows.dll',
                        'platforms/qoffscreen.dll', 'libgcc_s_seh-1.dll',
                        'libstdc++-6.dll', 'libwinpthread-1.dll')) {
    if (-not (Test-Path -LiteralPath (Join-Path $outputPath $relative) -PathType Leaf)) {
        throw "Deployment missing: $relative"
    }
}
Compress-Archive -Path (Join-Path $outputPath '*') -DestinationPath $zipPath -CompressionLevel Optimal
Write-Output "Windows Release package: $zipPath"
