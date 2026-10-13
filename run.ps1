param(
    [ValidateSet('run', 'test', 'coverage', 'smoke')]
    [string] $Action = 'run'
)

$ErrorActionPreference = 'Stop'
switch ($Action) {
    'run' { & "$PSScriptRoot\tools\invoke-bazel.ps1" run //:pebble_blaster }
    'test' { & "$PSScriptRoot\tools\invoke-bazel.ps1" test //... }
    'coverage' { & "$PSScriptRoot\tools\coverage.ps1" }
    'smoke' {
        $arguments = @('run', '//:pebble_blaster', '--', '--smoke-test', "$PSScriptRoot\.artifacts\screenshots")
        & "$PSScriptRoot\tools\invoke-bazel.ps1" @arguments
    }
}
exit $LASTEXITCODE