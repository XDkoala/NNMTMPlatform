[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$BuildDirectory,
    [Parameter(Mandatory)][string]$QtDirectory,
    [Parameter(Mandatory)][string]$QtLicenseDirectory,
    [Parameter(Mandatory)][string]$VcpkgDirectory,
    [Parameter(Mandatory)][string]$CrtDirectory,
    [Parameter(Mandatory)][string]$TorchLicenseWheel,
    [Parameter(Mandatory)][string]$QtSourceDirectory,
    [Parameter(Mandatory)][string]$DumpbinPath,
    [string]$OutputDirectory = 'dist',
    [ValidateSet('cpu','cuda')][string]$Backend = 'cpu',
    [string]$NvidiaLicenseDirectory,
    [string]$SourceRef = 'HEAD',
    [ValidateSet('zip','7z')][string]$ArchiveFormat = 'zip',
    [string]$SevenZipPath,
    [ValidateRange(1,32)][int]$CompressionThreads = 2
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$buildRoot = (Resolve-Path -LiteralPath $BuildDirectory).Path
$qtRoot = (Resolve-Path -LiteralPath $QtDirectory).Path
$vcpkgRoot = (Resolve-Path -LiteralPath $VcpkgDirectory).Path
$crtRoot = (Resolve-Path -LiteralPath $CrtDirectory).Path
$wheelPath = (Resolve-Path -LiteralPath $TorchLicenseWheel).Path
$qtSourceRoot = (Resolve-Path -LiteralPath $QtSourceDirectory).Path
$cmakeText = Get-Content -LiteralPath (Join-Path $projectRoot 'CMakeLists.txt') -Raw
if ($cmakeText -notmatch 'project\(NNMTMPlatform VERSION ([0-9.]+)') { throw 'Version not found.' }
$version = $Matches[1]
$outputRoot = [IO.Path]::GetFullPath((Join-Path $projectRoot $OutputDirectory))
if (!$outputRoot.StartsWith($projectRoot + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Output directory must be inside the project workspace.'
}
$suffix = if ($Backend -eq 'cuda') { 'cuda128' } else { 'cpu' }
$name = "NNMTMPlatform-v$version-windows-x64-$suffix"
$stage = Join-Path $outputRoot $name
$zipPath = Join-Path $outputRoot ($name + '.' + $ArchiveFormat)
if ($ArchiveFormat -eq '7z') {
    if (!$SevenZipPath) { throw '7-Zip command line path required.' }
    $SevenZipPath = (Resolve-Path -LiteralPath $SevenZipPath).Path
}
$qtSourceZip = Join-Path $outputRoot 'Qt-6.10.1-sources.zip'
if (Test-Path -LiteralPath $qtSourceZip) { throw 'Qt source ZIP already exists. Use a fresh output directory.' }
$qtSourceHashes = @{
    'qtbase-everywhere-src-6.10.1.tar.xz' = '5a6226f7e23db51fdc3223121eba53f3f5447cf0cc4d6cb82a3a2df7a65d265d'
    'qtsvg-everywhere-src-6.10.1.tar.xz' = 'c02f355a58f3bbcf404a628bf488b6aeb2d84a94c269afdb86f6e529343ab01f'
}
foreach ($file in $qtSourceHashes.Keys) {
    $actual = (Get-FileHash -LiteralPath (Join-Path $qtSourceRoot $file) -Algorithm SHA256).Hash
    if ($actual -ne $qtSourceHashes[$file]) { throw "Qt source hash mismatch: $file" }
}
if ((Test-Path -LiteralPath $stage) -or (Test-Path -LiteralPath $zipPath)) {
    throw 'Output already exists. Use a fresh output directory; this script never overwrites packages.'
}
if ($Backend -eq 'cpu' -and (Test-Path -LiteralPath (Join-Path $buildRoot 'torch_cuda.dll'))) {
    throw 'This script packages CPU builds only. Configure a separate CPU LibTorch build.'
}
if ($Backend -eq 'cuda') {
    if (!(Test-Path -LiteralPath (Join-Path $buildRoot 'torch_cuda.dll'))) { throw 'CUDA build required.' }
    if (!$NvidiaLicenseDirectory) { throw 'NVIDIA license directory required for CUDA distribution.' }
    foreach ($file in @('CUDA-12.8-EULA.html','cuDNN-9.10.2-EULA.html','CUPTI-12.8-LICENSE.html')) {
        if (!(Test-Path -LiteralPath (Join-Path $NvidiaLicenseDirectory $file))) { throw "Missing NVIDIA license: $file" }
    }
}
if (!(Test-Path -LiteralPath (Join-Path $buildRoot 'NNMTMPlatform.exe'))) { throw 'Build the application first.' }
New-Item -ItemType Directory -Path $stage -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $buildRoot 'NNMTMPlatform.exe') -Destination $stage

# Copy runtime DLLs only, never test executables, object files, caches or data/.
Get-ChildItem -LiteralPath $buildRoot -Filter '*.dll' -File | ForEach-Object {
    if ($_.Name -ne 'Qt6Charts.dll') { Copy-Item -LiteralPath $_.FullName -Destination $stage }
}
Get-ChildItem -LiteralPath $crtRoot -Filter '*.dll' -File | ForEach-Object {
    Copy-Item -LiteralPath $_.FullName -Destination $stage
}
& (Join-Path $qtRoot 'bin/windeployqt.exe') --release --no-translations --no-compiler-runtime `
    --no-system-d3d-compiler --no-opengl-sw --skip-plugin-types generic,networkinformation,tls `
    --dir $stage (Join-Path $stage 'NNMTMPlatform.exe')
if ($LASTEXITCODE -ne 0) { throw 'Qt deployment failed.' }

# windeployqt deploys Qt plugins, but does not fully resolve non-Qt transitive
# imports (e.g. OpenCV's JPEG/PNG/TIFF codecs). Walk PE imports and delay imports.
$dependencyQueue = [Collections.Generic.Queue[string]]::new()
$checked = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
Get-ChildItem -LiteralPath $stage -Recurse -File | Where-Object { $_.Extension -in @('.dll','.exe') } |
    ForEach-Object { $dependencyQueue.Enqueue($_.FullName) }
while ($dependencyQueue.Count -gt 0) {
    $binary = $dependencyQueue.Dequeue()
    if (!$checked.Add($binary)) { continue }
    $imports = & $DumpbinPath /nologo /dependents $binary
    if ($LASTEXITCODE -ne 0) { throw "Cannot inspect PE dependencies: $binary" }
    foreach ($line in $imports) {
        $dll = $line.Trim()
        if ($dll -notmatch '^[A-Za-z0-9_.-]+\.dll$') { continue }
        if ($dll -match '^(api-ms-win-|ext-ms-win-)') { continue }
        $local = Join-Path $stage $dll
        if (Test-Path -LiteralPath $local) { $dependencyQueue.Enqueue($local); continue }
        $found = $false
        foreach ($directory in @($buildRoot, (Join-Path $vcpkgRoot 'bin'), (Join-Path $qtRoot 'bin'), $crtRoot)) {
            $candidate = Join-Path $directory $dll
            if (Test-Path -LiteralPath $candidate) {
                Copy-Item -LiteralPath $candidate -Destination $local
                $dependencyQueue.Enqueue($local)
                $found = $true
                break
            }
        }
        if (!$found -and !(Test-Path -LiteralPath (Join-Path "$env:SystemRoot/System32" $dll))) {
            throw "Unresolved DLL dependency: $dll (from $binary)"
        }
    }
}
Copy-Item -LiteralPath (Join-Path $projectRoot 'examples') -Destination $stage -Recurse
Copy-Item -LiteralPath (Join-Path $projectRoot 'LICENSE') -Destination $stage
foreach ($file in @('Start.cmd', 'portable.ini')) {
    Copy-Item -LiteralPath (Join-Path $projectRoot "packaging/$file") -Destination $stage
}
$readme = if ($Backend -eq 'cuda') { 'README-Windows-CUDA.txt' } else { 'README-Windows.txt' }
$notice = if ($Backend -eq 'cuda') { 'THIRD_PARTY_NOTICES-CUDA.md' } else { 'THIRD_PARTY_NOTICES.md' }
Copy-Item -LiteralPath (Join-Path $projectRoot "packaging/$readme") -Destination (Join-Path $stage 'README-Windows.txt')
Copy-Item -LiteralPath (Join-Path $projectRoot "packaging/$notice") -Destination (Join-Path $stage 'THIRD_PARTY_NOTICES.md')

$licenses = Join-Path $stage 'licenses'
$qtLicenses = Join-Path $licenses 'Qt'
$torchLicenses = Join-Path $licenses 'PyTorch'
$vcpkgLicenses = Join-Path $licenses 'vcpkg'
New-Item -ItemType Directory -Path $qtLicenses,$torchLicenses,$vcpkgLicenses -Force | Out-Null
if ($Backend -eq 'cuda') {
    Copy-Item -LiteralPath $NvidiaLicenseDirectory -Destination (Join-Path $licenses 'NVIDIA') -Recurse
}
Copy-Item -LiteralPath (Join-Path $QtLicenseDirectory 'LICENSE') -Destination $qtLicenses
Copy-Item -LiteralPath (Join-Path $QtLicenseDirectory 'Copyright.txt') -Destination $qtLicenses
foreach ($module in @('qtbase', 'qtsvg')) {
    Copy-Item -LiteralPath (Join-Path $qtRoot "sbom/$module-6.10.1.spdx.json") -Destination $qtLicenses
    Copy-Item -LiteralPath (Join-Path $qtRoot "config_$module.opt") -Destination $qtLicenses
    Copy-Item -LiteralPath (Join-Path $qtRoot "config_$module.summary") -Destination $qtLicenses
}
# Preserve dependency notices from the installed vcpkg packages. Some notices
# cover embedded components too, so retain each package's complete file.
Get-ChildItem -LiteralPath (Join-Path $vcpkgRoot 'share') -Directory | ForEach-Object {
    $copyright = Join-Path $_.FullName 'copyright'
    if (Test-Path -LiteralPath $copyright) {
        $destination = Join-Path $vcpkgLicenses $_.Name
        New-Item -ItemType Directory -Path $destination -Force | Out-Null
        Copy-Item -LiteralPath $copyright -Destination $destination
    }
}
Add-Type -AssemblyName System.IO.Compression.FileSystem
[IO.Compression.ZipFile]::CreateFromDirectory($qtSourceRoot, $qtSourceZip, [IO.Compression.CompressionLevel]::NoCompression, $false)
$qtZipHash = (Get-FileHash -LiteralPath $qtSourceZip -Algorithm SHA256).Hash.ToLowerInvariant()
"$qtZipHash  Qt-6.10.1-sources.zip" | Set-Content -LiteralPath ($qtSourceZip + '.sha256') -Encoding ascii
$wheel = [IO.Compression.ZipFile]::OpenRead($wheelPath)
try {
    $variant = if ($Backend -eq 'cuda') { 'cu128' } else { 'cpu' }
    if (!($wheel.Entries | Where-Object { $_.FullName -match "torch-2\.9\.0\+$variant\.dist-info/LICENSE`$" })) {
        throw "License wheel must match PyTorch 2.9.0+$variant."
    }
    foreach ($file in @('LICENSE', 'NOTICE')) {
        $entries = @($wheel.Entries | Where-Object { $_.FullName -match "\.dist-info/$file`$" })
        if ($entries.Count -ne 1) { throw "Missing or ambiguous PyTorch $file." }
        [IO.Compression.ZipFileExtensions]::ExtractToFile($entries[0], (Join-Path $torchLicenses $file))
    }
} finally { $wheel.Dispose() }

# Exercise the actual GUI with only Windows system paths, from a different CWD.
# Clearing Qt overrides prevents use of a developer's Qt installation.
$startInfo = [Diagnostics.ProcessStartInfo]::new()
$startInfo.FileName = Join-Path $stage 'NNMTMPlatform.exe'
$startInfo.Arguments = '--smoke-test'
$startInfo.WorkingDirectory = $outputRoot
$startInfo.UseShellExecute = $false
$startInfo.CreateNoWindow = $true
$startInfo.EnvironmentVariables['PATH'] = "$env:SystemRoot\System32;$env:SystemRoot"
foreach ($variable in @('QT_PLUGIN_PATH','QT_QPA_PLATFORM_PLUGIN_PATH','QT_QPA_PLATFORM','QML2_IMPORT_PATH')) {
    $startInfo.EnvironmentVariables.Remove($variable)
}
$process = [Diagnostics.Process]::Start($startInfo)
try {
    if (!$process.WaitForExit(30000)) { $process.Kill(); throw 'GUI startup timed out.' }
    if ($process.ExitCode -ne 0) { throw "GUI startup failed: $($process.ExitCode)." }
} finally { $process.Dispose() }
if (!(Test-Path -LiteralPath (Join-Path $stage 'data/info'))) {
    throw 'Portable data directory was not created beside the application.'
}
$generatedData = Join-Path $stage 'data'
if ((Get-ChildItem -LiteralPath $generatedData -Recurse -File).Count -ne 0) {
    throw 'Smoke test generated unexpected data. Inspect before packaging.'
}
# Empty runtime directories are harmless; no user's data is copied or deleted.
if (Test-Path -LiteralPath (Join-Path $stage 'Qt6Charts.dll')) { throw 'GPL Qt Charts must not be bundled.' }
$sourceCommit = (& git -C $projectRoot rev-parse --verify "${SourceRef}^{commit}").Trim()
if ($LASTEXITCODE -ne 0) { throw 'Cannot read source commit.' }
$applicationChanges = & git -C $projectRoot diff $sourceCommit -- src resources CMakeLists.txt
if ($LASTEXITCODE -ne 0 -or $applicationChanges) { throw 'Application source differs from the specified source commit.' }
$manifest = [ordered]@{
    version = $version; platform = 'windows-x64'; backend = $Backend; sourceCommit = $sourceCommit
    packagingCommit = (& git -C $projectRoot rev-parse HEAD).Trim()
    files = @(Get-ChildItem -LiteralPath $stage -Recurse -File | Sort-Object FullName | ForEach-Object {
        [ordered]@{
            path = $_.FullName.Substring($stage.Length + 1).Replace('\','/')
            bytes = $_.Length
            sha256 = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
        }
    })
}
$manifest | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $stage 'manifest.json') -Encoding utf8
if ($ArchiveFormat -eq '7z') {
    & $SevenZipPath a -t7z $zipPath $stage '-mx=7' "-mmt=$CompressionThreads" '-md=64m' '-ms=on' '-bsp1'
    if ($LASTEXITCODE -ne 0) { throw '7-Zip compression failed.' }
} else {
    [IO.Compression.ZipFile]::CreateFromDirectory($stage, $zipPath, [IO.Compression.CompressionLevel]::Optimal, $true)
}
if ((Get-Item -LiteralPath $zipPath).Length -ge 2GB) { throw 'Archive exceeds GitHub single-asset limit.' }
$hash = (Get-FileHash -LiteralPath $zipPath -Algorithm SHA256).Hash.ToLowerInvariant()
"$hash  $name.$ArchiveFormat" | Set-Content -LiteralPath ($zipPath + '.sha256') -Encoding ascii
Write-Output "Created $zipPath"
Write-Output ('Archive size: {0:N1} MiB' -f ((Get-Item -LiteralPath $zipPath).Length / 1MB))
Write-Output "SHA256: $hash"
