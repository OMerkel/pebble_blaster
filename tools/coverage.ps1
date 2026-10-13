param(
    [double] $MinimumLines = 95,
    [double] $MinimumBranches = 95
)

$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$binaryDirectory = & "$PSScriptRoot\invoke-bazel.ps1" info bazel-bin
if ($LASTEXITCODE -ne 0) { throw 'Bazel initialization failed.' }

$candidates = @(
    $env:BAZEL_LLVM,
    (Join-Path $env:BAZEL_VC 'Tools\Llvm\x64'),
    (Join-Path $env:ProgramFiles 'LLVM')
)
$llvmRoot = $candidates | ForEach-Object {
    if ($_ -and (Test-Path (Join-Path $_ 'x64\bin\llvm-cov.exe'))) {
        Join-Path $_ 'x64'
    } elseif ($_ -and (Test-Path (Join-Path $_ 'bin\llvm-cov.exe'))) {
        $_
    }
} | Select-Object -First 1
if (-not $llvmRoot) {
    throw 'Coverage needs LLVM. Add C++ Clang tools for Windows in Visual Studio Installer, or install LLVM and set BAZEL_LLVM.'
}
$llvm = Join-Path $llvmRoot 'bin'
foreach ($tool in @('clang-cl.exe', 'llvm-profdata.exe', 'llvm-cov.exe')) {
    if (-not (Test-Path (Join-Path $llvm $tool))) { throw "LLVM tool missing: $tool" }
}

& "$PSScriptRoot\invoke-bazel.ps1" build //:core_test --repo_env=USE_CLANG_CL=1 "--repo_env=BAZEL_LLVM=$llvmRoot" --copt=/clang:-fprofile-instr-generate --copt=/clang:-fcoverage-mapping
if ($LASTEXITCODE -ne 0) { throw 'Coverage build failed.' }

$directory = Join-Path $root '.artifacts\coverage'
New-Item -ItemType Directory -Force -Path $directory | Out-Null
$raw = Join-Path $directory 'core.profraw'
$coverageProfilePath = Join-Path $directory 'core.profdata'
$binary = Join-Path ($binaryDirectory | Select-Object -Last 1) 'core_test.exe'
$previousProfile = $env:LLVM_PROFILE_FILE
try {
    $env:LLVM_PROFILE_FILE = $raw
    if (Test-Path $raw) { Remove-Item $raw }
    & $binary --gtest_brief=1
    if ($LASTEXITCODE -ne 0) { throw 'Unit tests failed.' }
} finally {
    $env:LLVM_PROFILE_FILE = $previousProfile
}
if (-not (Test-Path $raw)) { throw 'No LLVM profile was emitted. Check the Clang toolchain configuration.' }
& "$llvm\llvm-profdata.exe" merge -sparse $raw -o $coverageProfilePath
if ($LASTEXITCODE -ne 0) { throw 'Profile merge failed.' }
$report = & "$llvm\llvm-cov.exe" export $binary "-instr-profile=$coverageProfilePath" '-ignore-filename-regex=(external|tests)' -summary-only
if ($LASTEXITCODE -ne 0) { throw 'Coverage export failed.' }
$summary = $report | ConvertFrom-Json
$report | Set-Content (Join-Path $directory 'summary.json') -Encoding UTF8
& "$llvm\llvm-cov.exe" show $binary "-instr-profile=$coverageProfilePath" '-ignore-filename-regex=(external|tests)' -format=html "-output-dir=$directory\html" | Out-Null
if ($LASTEXITCODE -ne 0) { throw 'HTML report generation failed.' }
$totals = $summary.data[0].totals
Write-Host ('Core coverage: lines {0:N2}%, branches {1:N2}%, functions {2:N2}%' -f $totals.lines.percent, $totals.branches.percent, $totals.functions.percent)
Write-Host "HTML report: $directory\html\index.html"
if ($totals.lines.count -eq 0 -or $totals.branches.count -eq 0 -or
    $totals.lines.percent -lt $MinimumLines -or $totals.branches.percent -lt $MinimumBranches) {
    throw "Coverage gate failed: need $MinimumLines% lines and $MinimumBranches% branches."
}
exit 0