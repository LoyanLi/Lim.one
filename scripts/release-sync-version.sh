#!/bin/bash

# Version synchronization tool
# Single source of truth: CMakeLists.txt project(VERSION x.y.z)

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"
CMAKE_FILE="$REPO_ROOT/Limone/CMakeLists.txt"

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

print_header() {
    echo -e "\n${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${BLUE}  $1${NC}"
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}\n"
}
print_success() { echo -e "${GREEN}✓ $1${NC}"; }
print_error()   { echo -e "${RED}✗ $1${NC}"; }
print_info()    { echo -e "${BLUE}ℹ $1${NC}"; }

get_version() {
    grep -m1 'project(LimOne VERSION' "$CMAKE_FILE" | grep -oE '[0-9]+\.[0-9]+\.[0-9]+'
}

# Update only project() VERSION line (juce_add_plugin uses ${PROJECT_VERSION})
update_version() {
    local new_version="$1"
    cp "$CMAKE_FILE" "$CMAKE_FILE.bak"
    sed -i.tmp "s/\(project(LimOne VERSION\) [0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*/\1 $new_version/" "$CMAKE_FILE"
    rm -f "$CMAKE_FILE.tmp" "$CMAKE_FILE.bak"
    print_success "CMakeLists.txt updated to $new_version"
}

find_version_files() {
    echo -e "\n${BLUE}Source files containing version strings:${NC}\n"
    grep -r "[0-9]\+\.[0-9]\+\.[0-9]\+" \
        "$REPO_ROOT/Limone/Source" \
        --include="*.h" --include="*.cpp" --include="*.hpp" \
        -l 2>/dev/null | grep -v "juce\|boost" || echo "  (none found)"
}

show_menu() {
    print_header "Version Synchronization Tool"
    local ver
    ver="$(get_version)"
    echo "  CMakeLists.txt version: ${GREEN}${ver:-'not found'}${NC}"
    echo ""
    echo "1) Bump Major / Minor / Patch"
    echo "2) Set custom version"
    echo "3) List source files with version strings"
    echo "4) Exit"
    echo ""
}

main() {
    while true; do
        show_menu
        local choice
        read -p "Select option (1-4): " choice

        case $choice in
            1)
                local current
                current="$(get_version)"
                current="${current:-1.0.0}"

                local IFS='.'
                read -ra parts <<< "$current"
                local major="${parts[0]}" minor="${parts[1]}" patch="${parts[2]}"

                echo ""
                echo "Current: $current"
                echo "1) Major → $((major+1)).0.0"
                echo "2) Minor → $major.$((minor+1)).0"
                echo "3) Patch → $major.$minor.$((patch+1))"
                local bump new_version
                read -p "Select (1-3): " bump
                case $bump in
                    1) new_version="$((major+1)).0.0" ;;
                    2) new_version="$major.$((minor+1)).0" ;;
                    3) new_version="$major.$minor.$((patch+1))" ;;
                    *) print_error "Invalid"; continue ;;
                esac

                read -p "Update to $new_version? (y/n): " confirm
                [[ $confirm == [yY] ]] && update_version "$new_version"
                ;;
            2)
                local new_version
                read -p "Enter new version (X.Y.Z): " new_version
                if ! [[ $new_version =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
                    print_error "Invalid format (must be X.Y.Z)"
                    continue
                fi
                update_version "$new_version"
                ;;
            3)
                find_version_files
                ;;
            4)
                return 0
                ;;
            *)
                print_error "Invalid choice"
                ;;
        esac

        echo ""
        read -p "Press Enter to continue..."
    done
}

main
