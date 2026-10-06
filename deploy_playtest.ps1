[CmdletBinding()]
param (
    [switch]$Win,
    [switch]$Linux,
    [switch]$All
)

# Default to running both if no specific platform switch is provided
if (-not $Win -and -not$Linux) {
    $All =$true
}

$UnrealEnginePath = "C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\RunUAT.bat"
$ProjectPath      = "$PSScriptRoot\Unreal.uproject"

# Staging Directories
$WinStagingDir   = "$PSScriptRoot\Build\Windows"
$LinuxStagingDir = "$PSScriptRoot\Build\Linux"

# Itch Targets
$WinItchTarget   = "dennolouis/play-test-unreal:win"
$LinuxItchTarget = "dennolouis/play-test-unreal:linux"

$BuildFailed =$false

# Helper function to clear a directory safely
function Clean-StagingDir {
    param ([string]$Path)
    if (Test-Path $Path) {
        Write-Host "Cleaning staging directory: $Path..." -ForegroundColor DarkGray
        Remove-Item -Path $Path -Recurse -Force
    }
}

# -------------------------------------------------------------------
# WINDOWS BUILD & DEPLOY
# -------------------------------------------------------------------
if ($Win -or$All) {
    Clean-StagingDir -Path $WinStagingDir

    Write-Host "`n[1/2] Packaging Windows Game..." -ForegroundColor Cyan
    & $UnrealEnginePath BuildCookRun "-project=$ProjectPath" -noP4 -platform=Win64 -clientconfig=Shipping -cook -build -stage -pak -archive "-archivedirectory=$WinStagingDir"

    if ($LASTEXITCODE -eq 0) {
        Write-Host "Uploading Windows build to Itch.io..." -ForegroundColor Green
        butler push "$WinStagingDir" $WinItchTarget
    } else {
        Write-Host "Windows Build Failed. Skipping upload." -ForegroundColor Red
        $BuildFailed = $true
    }
}

# -------------------------------------------------------------------
# LINUX BUILD & DEPLOY
# -------------------------------------------------------------------
if ($Linux -or $All) {
    Clean-StagingDir -Path $LinuxStagingDir

    Write-Host "`n[2/2] Packaging Linux Game..." -ForegroundColor Cyan
    & $UnrealEnginePath BuildCookRun "-project=$ProjectPath" -noP4 -platform=Linux -clientconfig=Shipping -cook -build -stage -pak -archive "-archivedirectory=$LinuxStagingDir"

    if ($LASTEXITCODE -eq 0) {
        Write-Host "Uploading Linux build to Itch.io..." -ForegroundColor Green
        $LinuxFolder = if (Test-Path "$LinuxStagingDir") { "$LinuxStagingDir" } else { "$LinuxStagingDir\LinuxNoEditor" }
        butler push $LinuxFolder $LinuxItchTarget
    } else {
        Write-Host "Linux Build Failed. Skipping upload." -ForegroundColor Red
        $BuildFailed =$true
    }
}

# -------------------------------------------------------------------
# SUMMARY
# -------------------------------------------------------------------
if ($BuildFailed) {
    Write-Host "`nProcess finished with errors." -ForegroundColor Yellow
} else {
    Write-Host "`nSelected targets completed successfully!" -ForegroundColor Green
}