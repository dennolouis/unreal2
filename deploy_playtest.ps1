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

# -------------------------------------------------------------------
# WINDOWS BUILD & DEPLOY
# -------------------------------------------------------------------
if ($Win -or$All) {
    Write-Host "`n[1/2] Packaging Windows Game..." -ForegroundColor Cyan
    & $UnrealEnginePath BuildCookRun "-project=$ProjectPath" -noP4 -platform=Win64 -clientconfig=Shipping -cook -build -stage -pak -archive "-archivedirectory=$WinStagingDir"

    if ($LASTEXITCODE -eq 0) {
        Write-Host "Uploading Windows build to Itch.io..." -ForegroundColor Green
        butler push "$WinStagingDir\Windows" $WinItchTarget
    } else {
        Write-Host "Windows Build Failed. Skipping upload." -ForegroundColor Red
        $BuildFailed = $true
    }
}

# -------------------------------------------------------------------
# LINUX BUILD & DEPLOY
# -------------------------------------------------------------------
if ($Linux -or $All) {
    Write-Host "`n[2/2] Packaging Linux Game..." -ForegroundColor Cyan
    & $UnrealEnginePath BuildCookRun "-project=$ProjectPath" -noP4 -platform=Linux -clientconfig=Shipping -cook -build -stage -pak -archive "-archivedirectory=$LinuxStagingDir"

    if ($LASTEXITCODE -eq 0) {
        Write-Host "Uploading Linux build to Itch.io..." -ForegroundColor Green
        # UE usually names the packaged output folder "Linux" or "LinuxNoEditor"
        $LinuxFolder = if (Test-Path "$LinuxStagingDir\Linux") { "$LinuxStagingDir\Linux" } else { "$LinuxStagingDir\LinuxNoEditor" }
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