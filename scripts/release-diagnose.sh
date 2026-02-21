#!/bin/bash

# Quick diagnostic script for release setup
# Check all paths and dependencies

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"
PROJECT_DIR="$REPO_ROOT/Limone"
CMAKE_FILE="$PROJECT_DIR/CMakeLists.txt"
BUILD_DIR="$REPO_ROOT/build_limone"

echo "╔════════════════════════════════════════════════════════════╗"
echo "║         Release Script Diagnostic                          ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

# 1. Check directories
echo "📁 Directory Structure:"
echo "  Repo Root:     $REPO_ROOT"
[ -d "$REPO_ROOT" ] && echo "    ✓ EXISTS" || echo "    ✗ MISSING"

echo "  Project Dir:   $PROJECT_DIR"
[ -d "$PROJECT_DIR" ] && echo "    ✓ EXISTS" || echo "    ✗ MISSING"

echo "  CMakeLists:    $CMAKE_FILE"
[ -f "$CMAKE_FILE" ] && echo "    ✓ EXISTS" || echo "    ✗ MISSING"

echo "  Build Dir:     $BUILD_DIR"
[ -d "$BUILD_DIR" ] && echo "    ✓ EXISTS" || echo "    ⚠ WILL CREATE"

echo ""

# 2. Check git
echo "🔧 Git Setup:"
cd "$REPO_ROOT"
BRANCH=$(git rev-parse --abbrev-ref HEAD)
REMOTE=$(git config --get remote.origin.url 2>/dev/null || echo "not configured")
echo "  Branch:        $BRANCH"
echo "  Remote:        $REMOTE"
echo "  Status:        $(git status --short | wc -l) changes"

echo ""

# 3. Check CMake
echo "🛠️  CMake:"
if command -v cmake &> /dev/null; then
    CMAKE_VERSION=$(cmake --version | head -n1)
    echo "  ✓ $CMAKE_VERSION"
else
    echo "  ✗ CMake not found - install with: brew install cmake"
fi

echo ""

# 4. Check version in CMakeLists.txt
echo "📦 Version Info:"
if grep -q VERSION "$CMAKE_FILE"; then
    VERSION=$(grep VERSION "$CMAKE_FILE" | head -n3)
    echo "  Version line found:"
    echo "  $VERSION"
else
    echo "  ⚠ No VERSION found in CMakeLists.txt"
    echo "    First version update will add it"
fi

echo ""

# 5. Test CMake configuration (dry run)
echo "🔍 CMake Test (configuration only):"
if [ -d "$BUILD_DIR" ]; then
    echo "  Build directory exists, skipping test"
else
    echo "  Testing CMake configuration..."
    mkdir -p "$BUILD_DIR"
    if cmake -S "$PROJECT_DIR" -B "$BUILD_DIR" --fresh 2>&1 | head -20; then
        echo "  ✓ CMake configuration successful"
    else
        echo "  ✗ CMake configuration failed"
    fi
fi

echo ""

# 6. Show release scripts
echo "📋 Available Release Scripts:"
for script in "$REPO_ROOT"/release*.sh; do
    if [ -f "$script" ]; then
        name=$(basename "$script")
        [ -x "$script" ] && status="✓ executable" || status="⚠ not executable"
        echo "  $name ($status)"
    fi
done

echo ""

# 7. Show command examples
echo "🚀 Quick Start Examples:"
echo ""
echo "  Interactive menu:"
echo "    ./release.sh"
echo ""
echo "  Check status:"
echo "    ./release-quick.sh status"
echo ""
echo "  Full release (all steps):"
echo "    ./release-quick.sh full"
echo ""
echo "  Individual steps:"
echo "    ./release-quick.sh commit 'message'"
echo "    ./release-quick.sh version"
echo "    ./release-quick.sh build"
echo "    ./release-quick.sh package"
echo ""

echo "✅ Diagnostic complete!"
