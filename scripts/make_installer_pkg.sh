#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="${BUILD_DIR:-"$ROOT_DIR/build_limone"}"
ARTEFACTS_DIR="${ARTEFACTS_DIR:-"$BUILD_DIR"}"
OUT_DIR="${OUT_DIR:-"$ROOT_DIR/dist"}"

HOST_OS="$(uname -s)"

resolve_artefacts_dir() {
  local dir="$1"
  if [[ -z "$dir" ]]; then
    return 1
  fi

  for config in "$dir/Release" "$dir/Debug" "$dir/RelWithDebInfo" "$dir/MinSizeRel"; do
    if [[ -d "$config/VST3" || -d "$config/AU" || -d "$config/AAX" || -d "$config/Standalone" ]]; then
      echo "$config"
      return 0
    fi
  done

  if [[ -d "$dir/VST3" || -d "$dir/AU" || -d "$dir/AAX" || -d "$dir/Standalone" ]]; then
    echo "$dir"
    return 0
  fi

  if compgen -G "$dir/*_artefacts" > /dev/null; then
    local first
    first="$(ls -1d "$dir"/*_artefacts | head -n 1)"
    local resolved
    if resolved="$(resolve_artefacts_dir "$first" 2>/dev/null)"; then
      echo "$resolved"
    else
      echo "$first"
    fi
    return 0
  fi

  return 1
}

detect_plugin_name() {
  local artefacts_dir="$1"
  local vst3_dir="$artefacts_dir/VST3"
  local au_dir="$artefacts_dir/AU"
  local aax_dir="$artefacts_dir/AAX"

  if compgen -G "$vst3_dir/*.vst3" > /dev/null; then
    basename "$(ls -1d "$vst3_dir"/*.vst3 | head -n 1)" .vst3
    return 0
  fi

  if compgen -G "$au_dir/*.component" > /dev/null; then
    basename "$(ls -1d "$au_dir"/*.component | head -n 1)" .component
    return 0
  fi

  if compgen -G "$aax_dir/*.aaxplugin" > /dev/null; then
    basename "$(ls -1d "$aax_dir"/*.aaxplugin | head -n 1)" .aaxplugin
    return 0
  fi

  return 1
}

detect_version_from_plist() {
  local artefacts_dir="$1"
  local plugin_name="$2"

  local vst3_bundle="$artefacts_dir/VST3/$plugin_name.vst3"
  local au_bundle="$artefacts_dir/AU/$plugin_name.component"
  local aax_bundle="$artefacts_dir/AAX/$plugin_name.aaxplugin"

  local info_plist=""
  if [[ -f "$vst3_bundle/Contents/Info.plist" ]]; then
    info_plist="$vst3_bundle/Contents/Info.plist"
  elif [[ -f "$au_bundle/Contents/Info.plist" ]]; then
    info_plist="$au_bundle/Contents/Info.plist"
  elif [[ -f "$aax_bundle/Contents/Info.plist" ]]; then
    info_plist="$aax_bundle/Contents/Info.plist"
  fi

  if [[ -z "$info_plist" ]]; then
    return 1
  fi

  local version=""
  version="$(/usr/libexec/PlistBuddy -c 'Print :CFBundleShortVersionString' "$info_plist" 2>/dev/null || true)"
  if [[ -z "$version" ]]; then
    version="$(/usr/libexec/PlistBuddy -c 'Print :CFBundleVersion' "$info_plist" 2>/dev/null || true)"
  fi

  if [[ -n "$version" ]]; then
    echo "$version"
    return 0
  fi

  return 1
}

detect_version_from_cmake() {
  local cmake_file="$1"
  if [[ ! -f "$cmake_file" ]]; then
    return 1
  fi

  local version=""
  version="$(grep -m1 'project(LimOne VERSION' "$cmake_file" 2>/dev/null \
    | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' || true)"

  if [[ -n "$version" ]]; then
    echo "$version"
    return 0
  fi

  return 1
}

make_macos_pkg() {
  local artefacts_dir="$1"
  local plugin_name="$2"
  local version="$3"

  local vst3_bundle="$artefacts_dir/VST3/$plugin_name.vst3"
  local au_bundle="$artefacts_dir/AU/$plugin_name.component"
  local aax_bundle="$artefacts_dir/AAX/$plugin_name.aaxplugin"

  if [[ ! -d "$vst3_bundle" && ! -d "$au_bundle" && ! -d "$aax_bundle" ]]; then
    echo "macOS bundles not found for PLUGIN_NAME=$plugin_name in: $artefacts_dir" >&2
    return 1
  fi

  local identifier="${IDENTIFIER:-"com.lim.one.pkg"}"
  local pkg_name="${PKG_NAME:-"$plugin_name-$version.pkg"}"
  local pkg_path="$OUT_DIR/$pkg_name"

  local work_dir
  work_dir="$(mktemp -d)"
  local staging_root="$work_dir/root"
  mkdir -p "$staging_root"

  if [[ -d "$vst3_bundle" ]]; then
    mkdir -p "$staging_root/Library/Audio/Plug-Ins/VST3"
    ditto "$vst3_bundle" "$staging_root/Library/Audio/Plug-Ins/VST3/$plugin_name.vst3"
  fi

  if [[ -d "$au_bundle" ]]; then
    mkdir -p "$staging_root/Library/Audio/Plug-Ins/Components"
    ditto "$au_bundle" "$staging_root/Library/Audio/Plug-Ins/Components/$plugin_name.component"
  fi

  if [[ -d "$aax_bundle" ]]; then
    mkdir -p "$staging_root/Library/Application Support/Avid/Audio/Plug-Ins"
    ditto "$aax_bundle" "$staging_root/Library/Application Support/Avid/Audio/Plug-Ins/$plugin_name.aaxplugin"
  fi

  local codesign_identity="${CODESIGN_IDENTITY:-""}"
  if [[ -n "$codesign_identity" ]]; then
    local sign_opts
    sign_opts=(--force --sign "$codesign_identity" --timestamp --options runtime)
    if [[ -d "$staging_root/Library/Audio/Plug-Ins/VST3/$plugin_name.vst3" ]]; then
      codesign "${sign_opts[@]}" --deep "$staging_root/Library/Audio/Plug-Ins/VST3/$plugin_name.vst3"
    fi
    if [[ -d "$staging_root/Library/Audio/Plug-Ins/Components/$plugin_name.component" ]]; then
      codesign "${sign_opts[@]}" --deep "$staging_root/Library/Audio/Plug-Ins/Components/$plugin_name.component"
    fi
    if [[ -d "$staging_root/Library/Application Support/Avid/Audio/Plug-Ins/$plugin_name.aaxplugin" ]]; then
      codesign "${sign_opts[@]}" --deep "$staging_root/Library/Application Support/Avid/Audio/Plug-Ins/$plugin_name.aaxplugin"
    fi
  fi

  pkgbuild \
    --root "$staging_root" \
    --install-location "/" \
    --identifier "$identifier" \
    --version "$version" \
    "$pkg_path"

  rm -rf "$work_dir"
  echo "$pkg_path"
}

make_windows_packages() {
  local artefacts_dir="$1"
  local plugin_name="$2"
  local version="$3"

  local resolved_dir=""
  if resolved_dir="$(resolve_artefacts_dir "$artefacts_dir")"; then
    artefacts_dir="$resolved_dir"
  fi

  local vst3_bundle="$artefacts_dir/VST3/$plugin_name.vst3"
  local aax_bundle="$artefacts_dir/AAX/$plugin_name.aaxplugin"
  local standalone_dir="$artefacts_dir/Standalone"

  if [[ ! -d "$vst3_bundle" && ! -d "$aax_bundle" && ! -d "$standalone_dir" ]]; then
    local detected=""
    if detected="$(detect_plugin_name "$artefacts_dir" 2>/dev/null || true)"; then
      if [[ -n "$detected" && "$detected" != "$plugin_name" ]]; then
        local alt_vst3="$artefacts_dir/VST3/$detected.vst3"
        local alt_aax="$artefacts_dir/AAX/$detected.aaxplugin"
        if [[ -d "$alt_vst3" || -d "$alt_aax" ]]; then
          plugin_name="$detected"
          vst3_bundle="$artefacts_dir/VST3/$plugin_name.vst3"
          aax_bundle="$artefacts_dir/AAX/$plugin_name.aaxplugin"
        fi
      fi
    fi

    if [[ ! -d "$vst3_bundle" && ! -d "$aax_bundle" && ! -d "$standalone_dir" ]]; then
      echo "Windows bundles not found for PLUGIN_NAME=$plugin_name in: $artefacts_dir" >&2
      echo "Expected layout: <artefacts>/VST3/*.vst3 and/or <artefacts>/AAX/*.aaxplugin and/or <artefacts>/Standalone/*" >&2
      if [[ -d "$artefacts_dir" ]]; then
        echo "Found VST3:" >&2
        ls -1d "$artefacts_dir/VST3/"*.vst3 2>/dev/null || true
        echo "Found AAX:" >&2
        ls -1d "$artefacts_dir/AAX/"*.aaxplugin 2>/dev/null || true
        echo "Found Standalone:" >&2
        ls -1 "$artefacts_dir/Standalone" 2>/dev/null || true
      else
        echo "WIN_ARTEFACTS_DIR does not exist: $artefacts_dir" >&2
      fi
      return 1
    fi
  fi

  local work_dir
  work_dir="$(mktemp -d)"
  local payload_dir="$work_dir/payload"
  mkdir -p "$payload_dir"

  if [[ -d "$vst3_bundle" ]]; then
    mkdir -p "$payload_dir/VST3"
    ditto "$vst3_bundle" "$payload_dir/VST3/$plugin_name.vst3"
  fi

  if [[ -d "$aax_bundle" ]]; then
    mkdir -p "$payload_dir/AAX"
    ditto "$aax_bundle" "$payload_dir/AAX/$plugin_name.aaxplugin"
  fi

  if [[ -d "$standalone_dir" ]]; then
    mkdir -p "$payload_dir/Standalone"
    if compgen -G "$standalone_dir/*.exe" > /dev/null; then
      for exe in "$standalone_dir"/*.exe; do
        ditto "$exe" "$payload_dir/Standalone/$(basename "$exe")"
      done
    else
      ditto "$standalone_dir" "$payload_dir/Standalone"
    fi
  fi

  local zip_root="$work_dir/${plugin_name}-${version}-win"
  mkdir -p "$zip_root"
  ditto "$payload_dir" "$zip_root"

  local zip_path="$OUT_DIR/${plugin_name}-${version}-win.zip"
  ditto -c -k --sequesterRsrc --keepParent "$zip_root" "$zip_path"
  echo "$zip_path"

  if command -v makensis >/dev/null 2>&1; then
    local nsis_script="$work_dir/installer.nsi"
    local exe_path="$OUT_DIR/${plugin_name}-${version}-win-installer.exe"

    cat > "$nsis_script" <<EOF
Unicode true
RequestExecutionLevel admin
Name "${plugin_name}"
OutFile "${exe_path}"
InstallDir "\$PROGRAMFILES64\\${plugin_name}"
ShowInstDetails show
ShowUninstDetails show

!include "x64.nsh"

Section "Install"
  SetShellVarContext all
  \${IfNot} \${RunningX64}
    MessageBox MB_ICONSTOP "Requires 64-bit Windows."
    Abort
  \${EndIf}

  CreateDirectory "\$INSTDIR"
  SetOutPath "\$INSTDIR"
  WriteUninstaller "\$INSTDIR\\Uninstall.exe"

EOF

    if [[ -d "$payload_dir/VST3/$plugin_name.vst3" ]]; then
      cat >> "$nsis_script" <<EOF
  CreateDirectory "\$COMMONFILES64\\VST3"
  SetOutPath "\$COMMONFILES64\\VST3"
  File /r "payload\\VST3\\${plugin_name}.vst3"

EOF
    fi

    if [[ -d "$payload_dir/AAX/$plugin_name.aaxplugin" ]]; then
      cat >> "$nsis_script" <<EOF
  CreateDirectory "\$COMMONFILES64\\Avid\\Audio\\Plug-Ins"
  SetOutPath "\$COMMONFILES64\\Avid\\Audio\\Plug-Ins"
  File /r "payload\\AAX\\${plugin_name}.aaxplugin"

EOF
    fi

    if [[ -d "$payload_dir/Standalone" ]]; then
      cat >> "$nsis_script" <<EOF
  SetOutPath "\$INSTDIR"
  File /r "payload\\Standalone\\*.*"

EOF
    fi

    cat >> "$nsis_script" <<EOF
SectionEnd

Section "Uninstall"
  SetShellVarContext all
  Delete "\$INSTDIR\\Uninstall.exe"
  RMDir /r "\$INSTDIR"

EOF

    if [[ -d "$payload_dir/VST3/$plugin_name.vst3" ]]; then
      cat >> "$nsis_script" <<EOF
  RMDir /r "\$COMMONFILES64\\VST3\\${plugin_name}.vst3"

EOF
    fi

    if [[ -d "$payload_dir/AAX/$plugin_name.aaxplugin" ]]; then
      cat >> "$nsis_script" <<EOF
  RMDir /r "\$COMMONFILES64\\Avid\\Audio\\Plug-Ins\\${plugin_name}.aaxplugin"

EOF
    fi

    cat >> "$nsis_script" <<EOF
SectionEnd
EOF

    (cd "$work_dir" && makensis "$(basename "$nsis_script")" >/dev/null)
    echo "$exe_path"
  fi

  rm -rf "$work_dir"
}

mkdir -p "$OUT_DIR"

if resolved="$(resolve_artefacts_dir "$ARTEFACTS_DIR" 2>/dev/null)"; then
  ARTEFACTS_DIR="$resolved"
elif resolved="$(resolve_artefacts_dir "$BUILD_DIR" 2>/dev/null)"; then
  ARTEFACTS_DIR="$resolved"
fi

PLUGIN_NAME="${PLUGIN_NAME:-""}"
if [[ -z "$PLUGIN_NAME" ]]; then
  if PLUGIN_NAME="$(detect_plugin_name "$ARTEFACTS_DIR")"; then
    :
  else
    PLUGIN_NAME="Lim.one"
  fi
fi

VERSION="${VERSION:-""}"
if [[ -z "$VERSION" ]]; then
  VERSION="$(detect_version_from_cmake "$ROOT_DIR/Limone/CMakeLists.txt" 2>/dev/null || true)"
fi

if [[ -z "$VERSION" ]]; then
  if [[ "$HOST_OS" == "Darwin" ]]; then
    VERSION="$(detect_version_from_plist "$ARTEFACTS_DIR" "$PLUGIN_NAME" 2>/dev/null || true)"
  fi
  VERSION="${VERSION:-0.0.0}"
fi

MAKE_MAC="${MAKE_MAC:-""}"
MAKE_WIN="${MAKE_WIN:-""}"

if [[ -z "$MAKE_MAC" ]]; then
  MAKE_MAC="0"
  if [[ "$HOST_OS" == "Darwin" ]]; then
    MAKE_MAC="1"
  fi
fi

WIN_ARTEFACTS_DIR="${WIN_ARTEFACTS_DIR:-""}"
if [[ -z "$WIN_ARTEFACTS_DIR" ]]; then
  for cand in "$ROOT_DIR/build-win/Limone_artefacts" "$ROOT_DIR/build_win/Limone_artefacts" "$ROOT_DIR/build-windows/Limone_artefacts"; do
    if [[ -d "$cand" ]]; then
      WIN_ARTEFACTS_DIR="$cand"
      break
    fi
  done
fi

if [[ -n "$WIN_ARTEFACTS_DIR" ]]; then
  if resolved="$(resolve_artefacts_dir "$WIN_ARTEFACTS_DIR" 2>/dev/null)"; then
    WIN_ARTEFACTS_DIR="$resolved"
  fi
fi

if [[ -z "$MAKE_WIN" ]]; then
  MAKE_WIN="0"
  if [[ -n "$WIN_ARTEFACTS_DIR" ]]; then
    MAKE_WIN="1"
  fi
fi

if [[ "$MAKE_MAC" == "1" ]]; then
  MAC_PKG_PATH="$(make_macos_pkg "$ARTEFACTS_DIR" "$PLUGIN_NAME" "$VERSION")"
fi

if [[ "$MAKE_WIN" == "1" ]]; then
  if [[ -z "$WIN_ARTEFACTS_DIR" ]]; then
    echo "WIN_ARTEFACTS_DIR 未设置，且未找到默认的 build-win artefacts；无法生成 Windows 安装包/压缩包。" >&2
    echo "解决方案：在 Windows 机器/CI 上构建后，把 Limone_artefacts 拷贝/下载到本机，再指定 WIN_ARTEFACTS_DIR 运行。" >&2
    exit 2
  else
    WIN_PLUGIN_NAME="${WIN_PLUGIN_NAME:-""}"
    if [[ -n "$WIN_PLUGIN_NAME" ]]; then
      WIN_ZIP_PATH="$(make_windows_packages "$WIN_ARTEFACTS_DIR" "$WIN_PLUGIN_NAME" "$VERSION" | head -n 1)"
    else
      WIN_ZIP_PATH="$(make_windows_packages "$WIN_ARTEFACTS_DIR" "$PLUGIN_NAME" "$VERSION" | head -n 1)"
    fi
  fi
fi

if [[ -n "${MAC_PKG_PATH:-}" ]]; then
  echo "$MAC_PKG_PATH"
fi

if [[ -n "${WIN_ZIP_PATH:-}" ]]; then
  echo "$WIN_ZIP_PATH"
fi
