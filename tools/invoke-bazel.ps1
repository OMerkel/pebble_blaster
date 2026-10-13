$ErrorActionPreference = 'Stop'
$arguments = @($args)
$root = Split-Path $PSScriptRoot -Parent

if (-not $env:BAZEL_VC) {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (-not (Test-Path $vswhere)) {
        throw 'Install Visual Studio 2022 or newer with Desktop development with C++ (including the Windows SDK).'
    }
    $installation = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (-not $installation) {
        throw 'Visual Studio is installed but its C++ toolchain is missing. Add Desktop development with C++ in Visual Studio Installer.'
    }
    $env:BAZEL_VC = Join-Path $installation 'VC'
}

$directory = Join-Path $env:LOCALAPPDATA 'pebble-blaster-tools'
$bazelisk = Join-Path $directory 'bazelisk.exe'
$expectedHash = '023734F33ED6B9C6D65468FE20BB2C5FB32473CCB8ACA2FC5BF1521E61CE1622'
if (-not (Test-Path $bazelisk)) {
    New-Item -ItemType Directory -Force -Path $directory | Out-Null
    Write-Host 'Downloading Bazelisk 1.26.0...'
    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
    Invoke-WebRequest 'https://github.com/bazelbuild/bazelisk/releases/download/v1.26.0/bazelisk-windows-amd64.exe' -OutFile $bazelisk -UseBasicParsing
}
if ((Get-FileHash $bazelisk -Algorithm SHA256).Hash -ne $expectedHash) {
    throw "Bazelisk checksum mismatch. Remove $bazelisk and rerun to download a verified copy."
}

Push-Location $root
try {
    & $bazelisk '--host_jvm_args=-Djavax.net.ssl.trustStoreType=Windows-ROOT' '--host_jvm_args=-Djavax.net.ssl.trustStore=NONE' @arguments
    $result = $LASTEXITCODE
} finally {
    Pop-Location
}
exit $result