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

if ($null -eq (Get-Command cmake -ErrorAction SilentlyContinue)) {
    throw "CMake was not found in PATH. Install CMake 3.22 or newer."
}

if ($null -eq (Get-Command ctest -ErrorAction SilentlyContinue)) {
    throw "CTest was not found in PATH. Install the CMake test tools."
}

$SelectedPresets = if ($Preset -eq "all") {
    @("debug", "release")
} else {
    @($Preset)
}

foreach ($SelectedPreset in $SelectedPresets) {
    Write-Host "Configuring the $SelectedPreset preset..."
    Invoke-NativeCommand -Executable "cmake" -CommandArguments @("--preset", $SelectedPreset)

    Write-Host "Building the $SelectedPreset preset..."
    Invoke-NativeCommand -Executable "cmake" -CommandArguments @("--build", "--preset", $SelectedPreset)

    Write-Host "Testing the $SelectedPreset preset..."
    Invoke-NativeCommand -Executable "ctest" -CommandArguments @("--preset", $SelectedPreset)
}

Write-Host "Verification completed successfully."
