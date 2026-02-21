# Lim.one Release & Deployment Script (PowerShell Version)
# For Windows systems

param(
    [Parameter(Position=0)]
    [ValidateSet("status", "commit", "version", "build", "package", "full", "menu")]
    [string]$Command = "menu"
)

# Configuration
$REPO_ROOT = Split-Path -Parent $MyInvocation.MyCommand.Path
$PROJECT_NAME = "Lim.one"
$PROJECT_DIR = Join-Path $REPO_ROOT "Limone"
$CMAKE_FILE = Join-Path $PROJECT_DIR "CMakeLists.txt"
$BUILD_DIR = Join-Path $REPO_ROOT "build_limone"
$DIST_DIR = Join-Path $REPO_ROOT "dist"

# Colors
$Colors = @{
    Green  = 'Green'
    Red    = 'Red'
    Yellow = 'Yellow'
    Cyan   = 'Cyan'
}

# Helper Functions
function Write-Header {
    param([string]$Message)
    Write-Host ""
    Write-Host "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" -ForegroundColor $Colors.Cyan
    Write-Host "  $Message" -ForegroundColor $Colors.Cyan
    Write-Host "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" -ForegroundColor $Colors.Cyan
    Write-Host ""
}

function Write-Success {
    param([string]$Message)
    Write-Host "✓ $Message" -ForegroundColor $Colors.Green
}

function Write-Error2 {
    param([string]$Message)
    Write-Host "✗ $Message" -ForegroundColor $Colors.Red
}

function Write-Warning2 {
    param([string]$Message)
    Write-Host "⚠ $Message" -ForegroundColor $Colors.Yellow
}

function Write-Info {
    param([string]$Message)
    Write-Host "ℹ $Message" -ForegroundColor $Colors.Cyan
}

function Ask-YesNo {
    param([string]$Prompt)
    while ($true) {
        $response = Read-Host "$Prompt (y/n)"
        if ($response -eq 'y' -or $response -eq 'Y') { return $true }
        if ($response -eq 'n' -or $response -eq 'N') { return $false }
        Write-Host "Please answer y or n"
    }
}

# Git Functions
function Check-GitStatus {
    Write-Header "Git Status Check"

    Set-Location $REPO_ROOT

    $branch = git rev-parse --abbrev-ref HEAD
    Write-Info "Current branch: $branch"

    $status = git status --short
    if ($status) {
        Write-Warning2 "Uncommitted changes detected!"
        $status
        return $false
    }
    else {
        Write-Success "All changes committed"
        return $true
    }
}

function Commit-AndPush {
    Write-Header "Git Commit & Push"

    Set-Location $REPO_ROOT

    $status = git status --short
    if (-not $status) {
        Write-Info "No changes to commit"
        return
    }

    Write-Info "Files with changes:"
    $status

    $message = Read-Host "Commit message"
    if (-not $message) {
        Write-Error2 "Commit message cannot be empty!"
        return
    }

    git add -A
    if (git commit -m $message) {
        Write-Success "Commit created: '$message'"
    }
    else {
        Write-Error2 "Failed to create commit"
        return
    }

    if (Ask-YesNo "Push to remote?") {
        if (git push) {
            Write-Success "Pushed to remote"
        }
        else {
            Write-Error2 "Failed to push"
        }
    }
}

function Get-CurrentVersion {
    if (Test-Path $CMAKE_FILE) {
        $content = Get-Content $CMAKE_FILE
        $match = [regex]::Match($content, 'VERSION\s+(\d+\.\d+\.\d+)')
        if ($match.Success) {
            return $match.Groups[1].Value
        }
    }
    return "1.0.0"
}

# Version Management
function Update-Version {
    Write-Header "Version Update"

    $currentVersion = Get-CurrentVersion
    Write-Info "Current version: $currentVersion"

    $versionParts = $currentVersion.Split('.')
    $major = [int]$versionParts[0]
    $minor = [int]$versionParts[1]
    $patch = [int]$versionParts[2]

    Write-Host ""
    Write-Host "Select version bump:" -ForegroundColor $Colors.Cyan
    Write-Host "1) Major ($($major+1).0.0)"
    Write-Host "2) Minor ($major.$($minor+1).0)"
    Write-Host "3) Patch ($major.$minor.$($patch+1))"
    Write-Host "4) Custom version"
    Write-Host "5) Skip"

    $choice = Read-Host "Choice (1-5)"

    $newVersion = ""

    switch ($choice) {
        "1" { $newVersion = "$($major+1).0.0" }
        "2" { $newVersion = "$major.$($minor+1).0" }
        "3" { $newVersion = "$major.$minor.$($patch+1)" }
        "4" {
            $newVersion = Read-Host "Enter new version (X.Y.Z)"
            if ($newVersion -notmatch '^\d+\.\d+\.\d+$') {
                Write-Error2 "Invalid version format!"
                return
            }
        }
        "5" { return }
        default { Write-Error2 "Invalid choice"; return }
    }

    Write-Info "Updating version to $newVersion"

    # Update CMakeLists.txt
    if (Test-Path $CMAKE_FILE) {
        $content = Get-Content $CMAKE_FILE
        $content = [regex]::Replace($content, 'VERSION\s+\d+\.\d+\.\d+', "VERSION $newVersion")
        Set-Content $CMAKE_FILE $content
        Write-Success "Updated CMakeLists.txt"
    }

    if (Ask-YesNo "Commit version change?") {
        Set-Location $REPO_ROOT
        git add -A
        git commit -m "chore: bump version to $newVersion"
        Write-Success "Version updated and committed"
    }
}

# Build & Package Functions
function Build-Project {
    Write-Header "Build Project"

    if (-not (Test-Path $BUILD_DIR)) {
        Write-Info "Build directory not found, creating..."
        New-Item -ItemType Directory -Path $BUILD_DIR | Out-Null
    }

    if (-not (Test-Path $PROJECT_DIR)) {
        Write-Error2 "Project directory not found: $PROJECT_DIR"
        return
    }

    Write-Info "Running CMake configuration..."
    if (-not (cmake -S $PROJECT_DIR -B $BUILD_DIR -DCMAKE_BUILD_TYPE=Release)) {
        Write-Error2 "CMake configuration failed"
        return
    }
    Write-Success "CMake configuration succeeded"

    Write-Info "Building project..."
    if (-not (cmake --build $BUILD_DIR --config Release)) {
        Write-Error2 "Build failed"
        return
    }
    Write-Success "Build succeeded"
}

function Create-ReleasePackage {
    Write-Header "Create Release Package"

    $version = Get-CurrentVersion
    $packageName = "$PROJECT_NAME-$version"
    $packageDir = Join-Path $DIST_DIR $packageName

    if (-not (Test-Path $DIST_DIR)) {
        New-Item -ItemType Directory -Path $DIST_DIR | Out-Null
    }

    Write-Info "Creating package: $packageName"

    # Create package structure
    New-Item -ItemType Directory -Path "$packageDir\docs" -Force | Out-Null
    New-Item -ItemType Directory -Path "$packageDir\builds" -Force | Out-Null

    # Copy files
    Write-Info "Copying files..."
    if (Test-Path $BUILD_DIR) {
        Copy-Item -Path "$BUILD_DIR\*" -Destination "$packageDir\builds\" -Recurse -Force -ErrorAction SilentlyContinue
    }

    if (Test-Path "$REPO_ROOT\README.md") {
        Copy-Item -Path "$REPO_ROOT\README.md" -Destination "$packageDir\"
    }

    if (Test-Path "$REPO_ROOT\LICENSE") {
        Copy-Item -Path "$REPO_ROOT\LICENSE" -Destination "$packageDir\"
    }

    if (Test-Path "$REPO_ROOT\Docs") {
        Copy-Item -Path "$REPO_ROOT\Docs\*" -Destination "$packageDir\docs\" -Recurse -Force -ErrorAction SilentlyContinue
    }

    # Create manifest
    $manifest = @"
Package: $PROJECT_NAME
Version: $version
Created: $(Get-Date)
Platform: Windows

Contents:
- builds/: Compiled binaries and artifacts
- docs/: Documentation
- README.md: Project readme
- LICENSE: License information
"@

    Set-Content -Path "$packageDir\MANIFEST.txt" -Value $manifest

    Write-Success "Package created at: $packageDir"

    if (Ask-YesNo "Create ZIP archive?") {
        $zipPath = "$DIST_DIR\$packageName.zip"
        Compress-Archive -Path $packageDir -DestinationPath $zipPath -Force
        Write-Success "Archive created: $packageName.zip"
    }
}

function Run-FullRelease {
    Write-Header "Running Full Release Pipeline"

    if (-not (Check-GitStatus)) {
        if (-not (Ask-YesNo "Continue with uncommitted changes?")) {
            return
        }
    }

    Commit-AndPush
    Update-Version
    Build-Project
    Create-ReleasePackage

    Write-Success "Full release pipeline completed!"
}

function Show-Menu {
    Write-Header "$PROJECT_NAME Release & Deployment Tool"

    $version = Get-CurrentVersion
    Write-Host "Current version: $version"
    Write-Host ""
    Write-Host "Operations:" -ForegroundColor $Colors.Cyan
    Write-Host "1) Check git status"
    Write-Host "2) Git commit & push"
    Write-Host "3) Update version"
    Write-Host "4) Build project"
    Write-Host "5) Create release package"
    Write-Host "6) Full release"
    Write-Host "7) Exit"
    Write-Host ""

    $choice = Read-Host "Select operation (1-7)"

    switch ($choice) {
        "1" { Check-GitStatus }
        "2" { Commit-AndPush }
        "3" { Update-Version }
        "4" { Build-Project }
        "5" { Create-ReleasePackage }
        "6" { Run-FullRelease }
        "7" { exit }
        default { Write-Error2 "Invalid choice" }
    }

    Read-Host "Press Enter to continue"
    Show-Menu
}

# Main
switch ($Command) {
    "status" { Check-GitStatus }
    "commit" {
        if ($args.Count -eq 0) {
            Write-Error2 "Usage: .\release.ps1 commit <message>"
            exit 1
        }
        Set-Location $REPO_ROOT
        git add -A
        git commit -m $args
        if (Ask-YesNo "Push to remote?") {
            git push
        }
    }
    "version" { Update-Version }
    "build" { Build-Project }
    "package" { Create-ReleasePackage }
    "full" { Run-FullRelease }
    "menu" { Show-Menu }
    default { Write-Error2 "Unknown command"; exit 1 }
}
