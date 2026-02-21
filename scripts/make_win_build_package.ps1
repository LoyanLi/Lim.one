Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

# ─────────────────────────────────────────────────────────────────────────────
# Windows ARM / x64 translation note
# ─────────────────────────────────────────────────────────────────────────────
# Windows on ARM (WoA) supports x64 translation:
#   - ARM64 machines can run x64 DAWs via the built-in translation layer.
#     Those DAWs will load the x64 VST3 DLL transparently.
#   - ARM64-native DAWs load the arm64-win DLL directly.
#
# VST3 bundles support per-architecture DLLs in sub-folders inside Contents/:
#   Lim.one.vst3/Contents/x86_64-win/Lim.one.vst3   ← x64 DAWs (incl. translated)
#   Lim.one.vst3/Contents/arm64-win/Lim.one.vst3    ← native ARM64 DAWs
#
# Set TARGET_ARCHS=x64,arm64 to build a universal bundle covering both.
# Set TARGET_ARCHS=x64  (default) for x64-only.
# Set TARGET_ARCHS=arm64 for ARM64-only (native ARM64 DAWs, no x64 translation).
# ─────────────────────────────────────────────────────────────────────────────

$ScriptDir  = Split-Path -Parent $MyInvocation.MyCommand.Path
$RootDir    = Split-Path -Parent $ScriptDir

# ── Project dir ──────────────────────────────────────────────────────────────
$ProjectDir = if ($env:PROJECT_DIR) { $env:PROJECT_DIR } else { "" }
if ([string]::IsNullOrWhiteSpace($ProjectDir)) {
  $clipperDir = Join-Path $RootDir "ClipperLimiter"
  $limoneDir  = Join-Path $RootDir "Limone"
  if     (Test-Path $clipperDir) { $ProjectDir = $clipperDir }
  elseif (Test-Path $limoneDir)  { $ProjectDir = $limoneDir  }
  else   { throw "Project dir not found: $clipperDir or $limoneDir. Set PROJECT_DIR to override." }
}

$CMakeProject = if ($env:CMAKE_PROJECT_NAME) { $env:CMAKE_PROJECT_NAME } else { "" }
if ([string]::IsNullOrWhiteSpace($CMakeProject)) {
  $leaf = Split-Path -Leaf $ProjectDir
  $CMakeProject = if ($leaf -ieq "ClipperLimiter") { "ClipperLimiter" } else { "LimOne" }
}

# ── Build options ─────────────────────────────────────────────────────────────
# Auto-detect build dir: prefer BUILD_DIR env, then check common names
$BaseBuildDir = if ($env:BUILD_DIR) {
  $env:BUILD_DIR
} else {
  $candidates = @("build", "build-win", "build-windows")
  $found = $candidates | ForEach-Object { Join-Path $RootDir $_ } | Where-Object { Test-Path $_ } | Select-Object -First 1
  if ($found) { $found } else { Join-Path $RootDir "build" }
}
$OutDir       = if ($env:OUT_DIR)   { $env:OUT_DIR   } else { Join-Path $RootDir "dist"      }
$Config       = if ($env:CONFIG)    { $env:CONFIG    } else { "Release"                      }
# Auto-detect Visual Studio generator via vswhere
function Get-VSGenerator {
  $vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
  if (Test-Path $vswhere) {
    $ver = & $vswhere -latest -property installationVersion 2>$null
    if ($ver) {
      $major = [int]($ver -split '\.')[0]
      $map = @{ 16 = "Visual Studio 16 2019"; 17 = "Visual Studio 17 2022"; 18 = "Visual Studio 18 2026" }
      if ($map.ContainsKey($major)) { return $map[$major] }
    }
  }
  return $null
}
$Generator = if ($env:CMAKE_GENERATOR) {
  $env:CMAKE_GENERATOR
} else {
  $detected = Get-VSGenerator
  if ($detected) { Write-Host "Detected: $detected"; $detected }
  else { throw "Visual Studio not found. Install VS 2022/2026 with C++ workload, or set CMAKE_GENERATOR." }
}
$BuildAAX     = if ($env:BUILD_AAX)   { [bool]::Parse($env:BUILD_AAX)   } else { $true  }
# $SkipBuild = $null means "auto-detect per arch" (skip if artefacts already exist, build if not)
$SkipBuild    = if ($env:SKIP_BUILD)  { [bool]::Parse($env:SKIP_BUILD)  } else { $null  }
$MakeNSIS     = if ($env:MAKE_NSIS)   { [bool]::Parse($env:MAKE_NSIS)   } else { $true  }

# ── WebView2 SDK ──────────────────────────────────────────────────────────────
# Required on Windows for juce_gui_extra WebView2 support.
# Set WEBVIEW2_INCLUDE_DIR to skip auto-download.
$WebView2IncludeDir = if ($env:WEBVIEW2_INCLUDE_DIR) {
  $env:WEBVIEW2_INCLUDE_DIR
} else {
  $sdkDir = Join-Path $RootDir "webview2_sdk"
  $includeDir = Join-Path $sdkDir "build\native\include"
  if (-not (Test-Path (Join-Path $includeDir "WebView2.h"))) {
    Write-Host "Downloading WebView2 SDK..."
    New-Item -ItemType Directory -Force -Path $sdkDir | Out-Null
    $zip = Join-Path $sdkDir "webview2.zip"
    Invoke-WebRequest -Uri 'https://www.nuget.org/api/v2/package/Microsoft.Web.WebView2' -OutFile $zip
    Expand-Archive -Path $zip -DestinationPath $sdkDir -Force
    Remove-Item $zip
    Write-Host "WebView2 SDK downloaded: $includeDir"
  } else {
    Write-Host "WebView2 SDK found: $includeDir"
  }
  $includeDir
}

# ── Target architectures ──────────────────────────────────────────────────────
# Accepted values: x64, arm64  (comma-separated for multi-arch)
$TargetArchsRaw = if ($env:TARGET_ARCHS) { $env:TARGET_ARCHS } else { "x64,arm64" }
$TargetArchs    = @($TargetArchsRaw -split ',' | ForEach-Object { $_.Trim().ToLower() } | Where-Object { $_ -ne '' })

$ValidArchs = @('x64', 'arm64')
foreach ($a in $TargetArchs) {
  if ($a -notin $ValidArchs) { throw "Invalid arch '$a'. Accepted: $($ValidArchs -join ', ')" }
}

# CMake -A flag per arch
$CMakeArchMap = @{ 'x64' = 'x64'; 'arm64' = 'ARM64' }

# VST3 Contents sub-folder name per arch
$Vst3PlatformMap = @{ 'x64' = 'x86_64-win'; 'arm64' = 'arm64-win' }

$IsMultiArch = ($TargetArchs.Count -gt 1)
$ArchTag     = if ($IsMultiArch) { "universal" } else { $TargetArchs[0] }

Write-Host "Target arch(s): $($TargetArchs -join ', ')  →  tag: $ArchTag"

if (-not (Test-Path $ProjectDir)) { throw "Project dir not found: $ProjectDir" }

New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

# ─────────────────────────────────────────────────────────────────────────────
# Build each architecture
# ─────────────────────────────────────────────────────────────────────────────
function Resolve-ArtefactsDir([string]$Dir) {
  if (-not (Test-Path $Dir)) { return $null }
  foreach ($cfg in @('Release','Debug','RelWithDebInfo','MinSizeRel')) {
    $sub = Join-Path $Dir $cfg
    if ((Test-Path (Join-Path $sub 'VST3')) -or
        (Test-Path (Join-Path $sub 'AAX'))  -or
        (Test-Path (Join-Path $sub 'Standalone'))) { return $sub }
  }
  if ((Test-Path (Join-Path $Dir 'VST3')) -or
      (Test-Path (Join-Path $Dir 'AAX'))  -or
      (Test-Path (Join-Path $Dir 'Standalone'))) { return $Dir }
  return $null
}

$BuiltArtefacts = @{}  # arch -> resolved artefacts dir

foreach ($arch in $TargetArchs) {
  $archUpper = $arch.ToUpper()  # X64 / ARM64

  # Per-arch build dir: BUILD_DIR_X64 / BUILD_DIR_ARM64 → BaseBuildDir-arch → BaseBuildDir
  $archDirEnv = [System.Environment]::GetEnvironmentVariable("BUILD_DIR_$archUpper")
  $buildDir   = if ($archDirEnv) {
    $archDirEnv
  } elseif ($IsMultiArch) {
    # Prefer arch-specific subdir; fall back to base dir for first arch if it exists
    $specific = "${BaseBuildDir}-${arch}"
    if (-not (Test-Path $specific) -and $arch -eq $TargetArchs[0] -and (Test-Path $BaseBuildDir)) {
      $BaseBuildDir
    } else { $specific }
  } else {
    $BaseBuildDir
  }

  # Per-arch skip priority:
  #   1. SKIP_BUILD_X64 / SKIP_BUILD_ARM64  (explicit per-arch)
  #   2. SKIP_BUILD                          (explicit global)
  #   3. auto-detect: skip if artefacts already exist, build if not
  $archSkipEnv = [System.Environment]::GetEnvironmentVariable("SKIP_BUILD_$archUpper")
  $archSkip = if ($archSkipEnv -ne $null) {
    [bool]::Parse($archSkipEnv)
  } elseif ($SkipBuild -ne $null) {
    $SkipBuild
  } else {
    # Auto-detect: if artefacts already exist → skip, otherwise → build
    $testBase = Join-Path $buildDir ("{0}_artefacts" -f $CMakeProject)
    $null -ne (Resolve-ArtefactsDir $testBase)
  }

  New-Item -ItemType Directory -Force -Path $buildDir | Out-Null
  Write-Host "$arch → build dir: $buildDir  (skip=$archSkip)"

  if (-not $archSkip) {
    $cmake = Get-Command cmake -ErrorAction Stop
    $cmakeArch = $CMakeArchMap[$arch]

    # Clear stale CMakeCache if the generator changed (avoids "generator mismatch" error)
    $cacheFile = Join-Path $buildDir "CMakeCache.txt"
    if (Test-Path $cacheFile) {
      $cachedGen = Select-String -Path $cacheFile -Pattern "^CMAKE_GENERATOR:INTERNAL=(.+)" | ForEach-Object { $_.Matches[0].Groups[1].Value.Trim() }
      if ($cachedGen -and $cachedGen -ne $Generator) {
        Write-Host "Generator changed ($cachedGen → $Generator), clearing CMake cache..."
        Remove-Item -Force $cacheFile
        $cmakeFilesDir = Join-Path $buildDir "CMakeFiles"
        if (Test-Path $cmakeFilesDir) { Remove-Item -Recurse -Force $cmakeFilesDir }
      }
    }

    Write-Host "── Configuring for $arch ($cmakeArch) ──"
    & $cmake.Source -S $ProjectDir -B $buildDir -G $Generator -A $cmakeArch `
      -DWEBVIEW2_INCLUDE_DIR="$WebView2IncludeDir"

    $targets = @("${CMakeProject}_VST3", "${CMakeProject}_Standalone")
    if ($BuildAAX) { $targets += "${CMakeProject}_AAX" }

    foreach ($t in $targets) {
      try   { & $cmake.Source --build $buildDir --config $Config --target $t }
      catch {
        if ($t -eq "${CMakeProject}_AAX") { Write-Host "AAX build skipped: $($_.Exception.Message)" }
        else { throw }
      }
    }
  }

  $artefactsBase = Join-Path $buildDir ("{0}_artefacts" -f $CMakeProject)
  $resolved = Resolve-ArtefactsDir $artefactsBase
  if (-not $resolved) {
    if ($archSkip) {
      Write-Warning "$arch artefacts not found at '$artefactsBase' — skipping this arch. Set SKIP_BUILD_$archUpper=false to build it."
      continue
    } else {
      throw "Artefacts dir not found: $artefactsBase"
    }
  }
  $BuiltArtefacts[$arch] = $resolved
  Write-Host "$arch artefacts: $resolved"
}

# ─────────────────────────────────────────────────────────────────────────────
# Recalculate effective arches and tag after skipped arches are removed
# ─────────────────────────────────────────────────────────────────────────────
$EffectiveArchs = @($TargetArchs | Where-Object { $BuiltArtefacts.ContainsKey($_) })
if ($EffectiveArchs.Count -eq 0) { throw "No artefacts found for any architecture. Nothing to package." }

$IsMultiArch = ($EffectiveArchs.Count -gt 1)
$ArchTag     = if ($IsMultiArch) { "universal" } else { $EffectiveArchs[0] }
Write-Host "Effective arch(s): $($EffectiveArchs -join ', ')  →  tag: $ArchTag"

# ─────────────────────────────────────────────────────────────────────────────
# Detect plugin name + version from primary (first) arch
# ─────────────────────────────────────────────────────────────────────────────
$primaryArtefacts = $BuiltArtefacts[$EffectiveArchs[0]]

$PluginName = if ($env:PLUGIN_NAME) { $env:PLUGIN_NAME } else { "" }
if ([string]::IsNullOrWhiteSpace($PluginName)) {
  $vst3 = Get-ChildItem -Path (Join-Path $primaryArtefacts "VST3") -Filter "*.vst3" -Directory -ErrorAction SilentlyContinue | Select-Object -First 1
  if ($vst3) {
    $PluginName = [System.IO.Path]::GetFileNameWithoutExtension($vst3.Name)
  } else {
    $aax = Get-ChildItem -Path (Join-Path $primaryArtefacts "AAX") -Filter "*.aaxplugin" -Directory -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($aax) { $PluginName = [System.IO.Path]::GetFileNameWithoutExtension($aax.Name) }
    else       { $PluginName = if ($CMakeProject -eq "LimOne") { "Lim.one" } else { $CMakeProject } }
  }
}

$Version = if ($env:VERSION) { $env:VERSION } else { "" }
if ([string]::IsNullOrWhiteSpace($Version)) {
  $cmakeList = Join-Path $ProjectDir "CMakeLists.txt"
  if (Test-Path $cmakeList) {
    $text    = Get-Content -Raw -Path $cmakeList
    $pattern = ('project\s*\(\s*{0}\s+VERSION\s+([0-9]+\.[0-9]+\.[0-9]+)\s*\)' -f [regex]::Escape($CMakeProject))
    $m = [regex]::Match($text, $pattern, [System.Text.RegularExpressions.RegexOptions]::IgnoreCase)
    if ($m.Success) { $Version = $m.Groups[1].Value }
  }
  if ([string]::IsNullOrWhiteSpace($Version)) { $Version = "0.0.0" }
}

Write-Host "Plugin: $PluginName  Version: $Version  Arch tag: $ArchTag"

# ─────────────────────────────────────────────────────────────────────────────
# Stage: copy primary arch artifacts, then merge additional arch VST3 DLLs
# ─────────────────────────────────────────────────────────────────────────────
$StageRoot = Join-Path $env:TEMP ("{0}-{1}-win-{2}" -f $PluginName, $Version, $ArchTag)
if (Test-Path $StageRoot) { Remove-Item -Recurse -Force $StageRoot }
New-Item -ItemType Directory -Force -Path $StageRoot | Out-Null

$Vst3Bundle = Join-Path $primaryArtefacts ("VST3\{0}.vst3" -f $PluginName)
$AaxBundle  = Join-Path $primaryArtefacts ("AAX\{0}.aaxplugin" -f $PluginName)
$StandaloneSrc = Join-Path $primaryArtefacts "Standalone"

# Copy primary VST3 bundle
if (Test-Path $Vst3Bundle) {
  New-Item -ItemType Directory -Force -Path (Join-Path $StageRoot "VST3") | Out-Null
  Copy-Item -Recurse -Force $Vst3Bundle (Join-Path $StageRoot "VST3")
}

# Merge additional arch VST3 DLLs into the same bundle (universal bundle)
# The VST3 spec allows multiple Contents/<platform>-win/ folders in one bundle.
# DAWs auto-select the matching platform folder at load time.
if ($IsMultiArch) {
  foreach ($arch in $EffectiveArchs[1..($EffectiveArchs.Count - 1)]) {
    $addlArtefacts  = $BuiltArtefacts[$arch]
    $platformFolder = $Vst3PlatformMap[$arch]
    $srcPlatformDir = Join-Path $addlArtefacts ("VST3\{0}.vst3\Contents\{1}" -f $PluginName, $platformFolder)
    $dstPlatformDir = Join-Path $StageRoot ("VST3\{0}.vst3\Contents\{1}" -f $PluginName, $platformFolder)

    if (Test-Path $srcPlatformDir) {
      Write-Host "Merging $arch DLL into universal bundle ($platformFolder)"
      New-Item -ItemType Directory -Force -Path $dstPlatformDir | Out-Null
      Copy-Item -Recurse -Force (Join-Path $srcPlatformDir "*") $dstPlatformDir
    } else {
      Write-Warning "VST3 platform dir not found for $arch, skipping merge: $srcPlatformDir"
    }
  }
}

# Copy AAX (primary arch only — AAX does not support multi-arch bundles this way)
if (Test-Path $AaxBundle) {
  New-Item -ItemType Directory -Force -Path (Join-Path $StageRoot "AAX") | Out-Null
  Copy-Item -Recurse -Force $AaxBundle (Join-Path $StageRoot "AAX")
}

# Copy Standalone (primary arch; ARM64 users should use the arm64-specific build)
if (Test-Path $StandaloneSrc) {
  New-Item -ItemType Directory -Force -Path (Join-Path $StageRoot "Standalone") | Out-Null
  Copy-Item -Recurse -Force (Join-Path $StandaloneSrc "*") (Join-Path $StageRoot "Standalone")
}

# ─────────────────────────────────────────────────────────────────────────────
# Create .zip
# ─────────────────────────────────────────────────────────────────────────────
$ZipPath = Join-Path $OutDir ("{0}-{1}-win-{2}.zip" -f $PluginName, $Version, $ArchTag)
if (Test-Path $ZipPath) { Remove-Item -Force $ZipPath }
Compress-Archive -Path (Join-Path $StageRoot "*") -DestinationPath $ZipPath -Force
Write-Host "ZIP: $ZipPath"

# ─────────────────────────────────────────────────────────────────────────────
# Create NSIS installer (.exe)
# ─────────────────────────────────────────────────────────────────────────────
if ($MakeNSIS) {
  $makensis = Get-Command makensis -ErrorAction SilentlyContinue

  # Fallback: search common NSIS install paths if not in PATH
  if (-not $makensis) {
    $nsisCandidates = @(
      "C:\Program Files (x86)\NSIS\makensis.exe",
      "C:\Program Files\NSIS\makensis.exe"
    )
    foreach ($c in $nsisCandidates) {
      if (Test-Path $c) { $makensis = @{ Source = $c }; break }
    }
  }

  if (-not $makensis) {
    Write-Warning "makensis not found — skipping installer creation. Install NSIS: https://nsis.sourceforge.io"
  } else {
    # Download WebView2 Runtime bootstrapper (silent installer)
    $WebView2Url = "https://go.microsoft.com/fwlink/p/?LinkId=2124703"
    $WebView2BootstrapperPath = Join-Path $env:TEMP "MicrosoftEdgeWebView2Setup.exe"

    if (-not (Test-Path $WebView2BootstrapperPath)) {
      try {
        Write-Host "Downloading WebView2 Runtime bootstrapper..."
        Invoke-WebRequest -Uri $WebView2Url -OutFile $WebView2BootstrapperPath -ErrorAction Stop
        Write-Host "WebView2 bootstrapper downloaded: $WebView2BootstrapperPath"
      } catch {
        Write-Warning "Failed to download WebView2 bootstrapper: $_. Installer will not include WebView2 auto-install."
      }
    }

    $NsisScript  = Join-Path $env:TEMP ("{0}-{1}-{2}-installer.nsi" -f $PluginName, $Version, $ArchTag)
    # Resolve to absolute path — NSIS runs from %TEMP%, relative paths fail
    $OutDirAbs   = [System.IO.Path]::GetFullPath($OutDir)
    $ExePath     = Join-Path $OutDirAbs ("{0}-{1}-win-{2}-installer.exe" -f $PluginName, $Version, $ArchTag)
    $PayloadDir  = [System.IO.Path]::GetFullPath($StageRoot)

    $nsis = New-Object System.Collections.Generic.List[string]
    $nsis.Add('Unicode true')
    $nsis.Add('RequestExecutionLevel admin')
    $nsis.Add('')
    # Arch / translation notes embedded as installer comments
    $nsis.Add('; ── Architecture support ─────────────────────────────────────────────')
    if ($IsMultiArch) {
      $nsis.Add('; This installer contains a UNIVERSAL VST3 bundle:')
      $nsis.Add(';   x86_64-win/  — loaded by x64 DAWs (including x64 DAWs running via')
      $nsis.Add(';                  Windows ARM x64 translation on ARM64 machines)')
      $nsis.Add(';   arm64-win/   — loaded by native ARM64 DAWs on ARM64 Windows')
      $nsis.Add('; The DAW automatically selects the correct DLL at runtime.')
    } else {
      $archLabel = $EffectiveArchs[0]
      if ($archLabel -eq 'x64') {
        $nsis.Add('; x64 build. On Windows ARM machines, x64 DAWs load this plugin via')
        $nsis.Add('; the built-in x64 translation layer — no separate ARM64 build needed')
        $nsis.Add('; for those users as long as they run an x64 DAW.')
      } else {
        $nsis.Add('; ARM64 native build. Requires an ARM64-native DAW on ARM64 Windows.')
        $nsis.Add('; x64 DAW users on ARM machines should use the x64 installer instead.')
      }
    }
    $nsis.Add('; ─────────────────────────────────────────────────────────────────────')
    $nsis.Add('')
    $nsis.Add(('Name "{0}"' -f $PluginName))
    $nsis.Add(('OutFile "{0}"' -f $ExePath))
    $nsis.Add(('InstallDir "$PROGRAMFILES64\{0}"' -f $PluginName))
    $nsis.Add('ShowInstDetails show')
    $nsis.Add('ShowUninstDetails show')
    $nsis.Add('')
    $nsis.Add('!include "x64.nsh"')
    $nsis.Add('!include "LogicLib.nsh"')
    $nsis.Add('')
    $nsis.Add('Section "Install"')
    $nsis.Add('  SetShellVarContext all')
    $nsis.Add('  DetailPrint ""')
    $nsis.Add('  DetailPrint "=========================================="')
    $nsis.Add("  DetailPrint `"$PluginName v$Version Installer`"")
    $nsis.Add('  DetailPrint "=========================================="')
    $nsis.Add('  DetailPrint ""')
    $nsis.Add('')

    # Add WebView2 installation step if bootstrapper is available
    if (Test-Path $WebView2BootstrapperPath) {
      $nsis.Add('  ; Install WebView2 Runtime (required for plugin UI)')
      $nsis.Add('  DetailPrint "Installing Microsoft Edge WebView2 Runtime..."')
      $nsis.Add("  File /oname=WebView2Setup.exe `"$WebView2BootstrapperPath`"")
      $nsis.Add('  ExecWait "$INSTDIR\WebView2Setup.exe /silent /install"')
      $nsis.Add('  Delete "$INSTDIR\WebView2Setup.exe"')
      $nsis.Add('')
    }

    # Add WebView2 info message
    if (Test-Path $WebView2BootstrapperPath) {
      $nsis.Add('  DetailPrint ""')
      $nsis.Add('  DetailPrint "========== IMPORTANT =========="')
      $nsis.Add('  DetailPrint "Installing Microsoft Edge WebView2 Runtime..."')
      $nsis.Add('  DetailPrint "This is required for the plugin UI."')
      $nsis.Add('  DetailPrint "=============================="')
      $nsis.Add('  DetailPrint ""')
    }

    # Architecture check: universal and x64 accept both x64 and ARM64 Windows.
    # ARM64-only build requires ARM64 or x64 (x64 translation still runs the DLL fine
    # if somehow the installer is run, but the DLL won't load in an x64 DAW).
    if ($EffectiveArchs -contains 'arm64' -and -not ($EffectiveArchs -contains 'x64')) {
      # ARM64-only: warn on pure x86/x86_64 non-ARM Windows
      $nsis.Add('  ; ARM64-only build: intended for native ARM64 DAWs on ARM64 Windows.')
      $nsis.Add('  ; Installation is still allowed on x64 Windows but the plugin will not')
      $nsis.Add('  ; load in an x64 DAW — use the x64 installer for that scenario.')
      $nsis.Add('  ${IfNot} ${RunningX64}')
      $nsis.Add('    MessageBox MB_ICONSTOP "Requires 64-bit (x64 or ARM64) Windows."')
      $nsis.Add('    Abort')
      $nsis.Add('  ${EndIf}')
    } else {
      # x64 or universal: require 64-bit Windows (covers native x64 and ARM64 via translation)
      $nsis.Add('  ${IfNot} ${RunningX64}')
      $nsis.Add('    MessageBox MB_ICONSTOP "Requires 64-bit Windows (x64 or Windows on ARM)."')
      $nsis.Add('    Abort')
      $nsis.Add('  ${EndIf}')
    }

    $nsis.Add('')
    $nsis.Add('  CreateDirectory "$INSTDIR"')
    $nsis.Add('  SetOutPath "$INSTDIR"')
    $nsis.Add('  WriteUninstaller "$INSTDIR\Uninstall.exe"')
    $nsis.Add('')

    $payloadVst3 = Join-Path $PayloadDir "VST3"
    if (Test-Path $payloadVst3) {
      $nsis.Add('  ; Install VST3 to the standard shared VST3 folder')
      $nsis.Add('  CreateDirectory "$COMMONFILES64\VST3"')
      $nsis.Add('  SetOutPath "$COMMONFILES64\VST3"')
      $nsis.Add(('  File /r "{0}\{1}.vst3"' -f $payloadVst3, $PluginName))
      $nsis.Add('')
    }

    $payloadAax = Join-Path $PayloadDir "AAX"
    if (Test-Path $payloadAax) {
      $nsis.Add('  CreateDirectory "$COMMONFILES64\Avid\Audio\Plug-Ins"')
      $nsis.Add('  SetOutPath "$COMMONFILES64\Avid\Audio\Plug-Ins"')
      $nsis.Add(('  File /r "{0}\{1}.aaxplugin"' -f $payloadAax, $PluginName))
      $nsis.Add('')
    }

    $payloadStandalone = Join-Path $PayloadDir "Standalone"
    if (Test-Path $payloadStandalone) {
      $nsis.Add('  SetOutPath "$INSTDIR"')
      $nsis.Add(('  File /r "{0}\*.*"' -f $payloadStandalone))
      $nsis.Add('')
    }

    $nsis.Add('SectionEnd')
    $nsis.Add('')
    $nsis.Add('Section "Uninstall"')
    $nsis.Add('  SetShellVarContext all')
    $nsis.Add(('  RMDir /r "$COMMONFILES64\VST3\{0}.vst3"' -f $PluginName))
    if (Test-Path (Join-Path $PayloadDir "AAX")) {
      $nsis.Add(('  RMDir /r "$COMMONFILES64\Avid\Audio\Plug-Ins\{0}.aaxplugin"' -f $PluginName))
    }
    $nsis.Add('  Delete "$INSTDIR\Uninstall.exe"')
    $nsis.Add('  RMDir /r "$INSTDIR"')
    $nsis.Add('SectionEnd')

    $nsis | Set-Content -Encoding UTF8 -Path $NsisScript

    try {
      & $makensis.Source $NsisScript | Out-Null
      Write-Host "EXE: $ExePath"
    } catch {
      Write-Warning "NSIS installer creation failed (non-fatal): $_"
    }
  }
}
