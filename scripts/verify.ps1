param(
    [ValidateSet("all", "debug", "release")]
    [string]$Preset = "all"
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

function Invoke-NativeCommand {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Executable,

        [Parameter(Mandatory = $true)]
        [string[]]$CommandArguments
    )

    & $Executable @CommandArguments
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
}

$CMakeCommand = Get-Command cmake -ErrorAction SilentlyContinue
$CTestCommand = Get-Command ctest -ErrorAction SilentlyContinue

if ($null -eq $CMakeCommand -or $null -eq $CTestCommand) {
    $CandidateDirectories = @(
        "C:\msys64\ucrt64\bin",
        "C:\msys64\mingw64\bin",
        (Join-Path $env:ProgramFiles "CMake\bin")
    )

    foreach ($CandidateDirectory in $CandidateDirectories) {
        $CMakeCandidate = Join-Path $CandidateDirectory "cmake.exe"
        $CTestCandidate = Join-Path $CandidateDirectory "ctest.exe"

        if ((Test-Path $CMakeCandidate) -and (Test-Path $CTestCandidate)) {
            $env:Path = "$CandidateDirectory;$env:Path"
            $CMakeCommand = Get-Command $CMakeCandidate
            $CTestCommand = Get-Command $CTestCandidate
            break
        }
    }
}

if ($null -eq $CMakeCommand -or $null -eq $CTestCommand) {
    throw "CMake and CTest were not found. Install CMake 3.22 or newer."
}

$SelectedPresets = if ($Preset -eq "all") {
    @("debug", "release")
} else {
    @($Preset)
}

foreach ($SelectedPreset in $SelectedPresets) {
    Write-Host "Configuring the $SelectedPreset preset..."
    Invoke-NativeCommand -Executable $CMakeCommand.Source -CommandArguments @("--preset", $SelectedPreset)

    Write-Host "Building the $SelectedPreset preset..."
    Invoke-NativeCommand -Executable $CMakeCommand.Source -CommandArguments @("--build", "--preset", $SelectedPreset)

    Write-Host "Testing the $SelectedPreset preset..."
    Invoke-NativeCommand -Executable $CTestCommand.Source -CommandArguments @("--preset", $SelectedPreset)
}

Write-Host "Verification completed successfully."
