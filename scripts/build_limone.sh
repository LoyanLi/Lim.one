#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"
PROJECT_DIR="${PROJECT_DIR:-"$ROOT_DIR/Limone"}"
BUILD_DIR="${BUILD_DIR:-"$ROOT_DIR/build_limone"}"
CONFIG="${CONFIG:-Release}"
PACKAGE="${PACKAGE:-0}"

print_usage() {
  echo "Usage: $(basename "$0") [--release|--debug|--config <name>] [--build-dir <dir>] [--package]"
  echo ""
  echo "Environment overrides:"
  echo "  PROJECT_DIR, BUILD_DIR, CONFIG, PACKAGE, CMAKE_GENERATOR"
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --release)
      CONFIG="Release"
      shift
      ;;
    --debug)
      CONFIG="Debug"
      shift
      ;;
    --config)
      CONFIG="${2:?missing config name}"
      shift 2
      ;;
    --build-dir)
      BUILD_DIR="${2:?missing build dir}"
      shift 2
      ;;
    --package)
      PACKAGE="1"
      shift
      ;;
    -h|--help)
      print_usage
      exit 0
      ;;
    *)
      echo "Unknown argument: $1" >&2
      print_usage >&2
      exit 2
      ;;
  esac
done

if [[ ! -d "$PROJECT_DIR" ]]; then
  echo "Project dir not found: $PROJECT_DIR" >&2
  exit 1
fi

mkdir -p "$BUILD_DIR"

cmake_args=()
if [[ -n "${CMAKE_GENERATOR:-}" ]]; then
  cmake_args+=(-G "$CMAKE_GENERATOR")
fi

cmake -S "$PROJECT_DIR" -B "$BUILD_DIR" ${cmake_args[@]+"${cmake_args[@]}"} -DCMAKE_BUILD_TYPE="$CONFIG"
cmake --build "$BUILD_DIR" --config "$CONFIG"

artefacts_dir="$BUILD_DIR/LimOne_artefacts"
if [[ -d "$artefacts_dir/$CONFIG" ]]; then
  artefacts_dir="$artefacts_dir/$CONFIG"
fi
echo "Artefacts: $artefacts_dir"

if [[ "$PACKAGE" == "1" ]]; then
  BUILD_DIR="$BUILD_DIR" ARTEFACTS_DIR="$artefacts_dir" MAKE_WIN=0 "$SCRIPT_DIR/make_installer_pkg.sh"
fi
