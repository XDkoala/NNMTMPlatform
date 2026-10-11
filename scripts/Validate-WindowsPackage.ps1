[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$ArchivePath,
    [Parameter(Mandatory)][string]$BuildDirectory,
    [string]$OutputDirectory = 'build/portable-verify',
    [ValidateSet('cpu','cuda')][string]$Device = 'cpu'
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$archive = (Resolve-Path -LiteralPath $ArchivePath).Path
$buildRoot = (Resolve-Path -LiteralPath $BuildDirectory).Path
$verifyRoot = [IO.Path]::GetFullPath((Join-Path $projectRoot $OutputDirectory))
if (!$verifyRoot.StartsWith($projectRoot + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Validation output must be inside the workspace.'
}
if (Test-Path -LiteralPath $verifyRoot) { throw 'Use a fresh validation output directory.' }
Expand-Archive -LiteralPath $archive -DestinationPath $verifyRoot
$directories = @(Get-ChildItem -LiteralPath $verifyRoot -Directory)
if ($directories.Count -ne 1) { throw 'Expected one package directory.' }
$packageRoot = $directories[0].FullName
$manifest = Get-Content -LiteralPath (Join-Path $packageRoot 'manifest.json') -Raw | ConvertFrom-Json
foreach ($file in $manifest.files) {
    $path = [IO.Path]::GetFullPath((Join-Path $packageRoot $file.path))
    if (!$path.StartsWith($packageRoot + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
        throw 'Invalid manifest path.'
    }
    if ((Get-Item -LiteralPath $path).Length -ne $file.bytes -or
        (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash -ne $file.sha256) {
        throw "Manifest mismatch: $($file.path)"
    }
}
$checksRoot = Join-Path $verifyRoot 'checks'
New-Item -ItemType Directory -Path $checksRoot | Out-Null
foreach ($test in @('NNMTMAnnPipelineCheck.exe','NNMTMCnnPipelineCheck.exe')) {
    Copy-Item -LiteralPath (Join-Path $buildRoot $test) -Destination $checksRoot
}
$checks = @(
    @{ Path = (Join-Path $packageRoot 'NNMTMPlatform.exe'); Arguments = '--smoke-test' },
    @{ Path = (Join-Path $checksRoot 'NNMTMAnnPipelineCheck.exe'); Arguments = '' },
    @{ Path = (Join-Path $checksRoot 'NNMTMCnnPipelineCheck.exe'); Arguments = '' }
)
if ($Device -eq 'cuda') {
    Copy-Item -LiteralPath (Join-Path $buildRoot 'NNMTMCudaRuntimeCheck.exe') -Destination $checksRoot
    $checks += @{ Path = (Join-Path $checksRoot 'NNMTMCudaRuntimeCheck.exe'); Arguments = '' }
}
$modes = @($Device)
if ($Device -eq 'cuda') { $modes += @('cpu','auto-no-gpu') }
foreach ($mode in $modes) {
foreach ($check in $checks) {
    if ($mode -ne 'cuda' -and $check.Path.EndsWith('NNMTMCudaRuntimeCheck.exe')) { continue }
    $info = [Diagnostics.ProcessStartInfo]::new()
    $info.FileName = $check.Path
    $info.Arguments = $check.Arguments
    $info.WorkingDirectory = $checksRoot
    $info.UseShellExecute = $false
    $info.CreateNoWindow = $true
    $info.RedirectStandardOutput = $true
    $info.RedirectStandardError = $true
    $info.EnvironmentVariables['PATH'] = "$packageRoot;$env:SystemRoot\System32;$env:SystemRoot"
    $info.EnvironmentVariables['NNMTM_DEVICE'] = if ($mode -eq 'auto-no-gpu') { 'auto' } else { $mode }
    $info.EnvironmentVariables.Remove('CUDA_VISIBLE_DEVICES')
    if ($mode -eq 'auto-no-gpu') { $info.EnvironmentVariables['CUDA_VISIBLE_DEVICES'] = '-1' }
    foreach ($variable in @('QT_PLUGIN_PATH','QT_QPA_PLATFORM_PLUGIN_PATH','QT_QPA_PLATFORM','QML2_IMPORT_PATH')) {
        $info.EnvironmentVariables.Remove($variable)
    }
    $process = [Diagnostics.Process]::Start($info)
    try {
        $stdout = $process.StandardOutput.ReadToEndAsync()
        $stderr = $process.StandardError.ReadToEndAsync()
        if (!$process.WaitForExit(120000)) { $process.Kill(); throw 'Validation timed out.' }
        $text = $stdout.GetAwaiter().GetResult() + $stderr.GetAwaiter().GetResult()
        $logName = [IO.Path]::GetFileNameWithoutExtension($check.Path) + "-$mode.log"
        $text | Set-Content -LiteralPath (Join-Path $checksRoot $logName) -Encoding utf8
        if ($process.ExitCode -ne 0) { throw "Failed: $($check.Path), exit=$($process.ExitCode)\n$text" }
        if ($check.Path -match 'PipelineCheck\.exe$') {
            $expected = if ($mode -eq 'cuda') { 'CUDA:0' } else { 'CPU' }
            if (!$text.Contains("passed on $expected")) { throw "Expected pipeline device $expected." }
            if ($mode -eq 'auto-no-gpu' -and !$text.Contains('CUDA is unavailable; using CPU.')) {
                throw 'Automatic CPU fallback was not observed.'
            }
        }
        Write-Output "Passed [$mode]: $([IO.Path]::GetFileName($check.Path))"
    } finally { $process.Dispose() }
}
}
if (!(Test-Path -LiteralPath (Join-Path $packageRoot 'data/info'))) { throw 'Portable storage is missing.' }
Write-Output "Manifest, GUI startup and isolated ANN/CNN pipelines passed: $packageRoot"
