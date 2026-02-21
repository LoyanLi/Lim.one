Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RootDir   = Split-Path -Parent $ScriptDir
$ProjectDir = if ($env:PROJECT_DIR) { $env:PROJECT_DIR } else { "" }
if ([string]::IsNullOrWhiteSpace($ProjectDir)) {
  $clipperDir = Join-Path $RootDir "ClipperLimiter"
  $limoneDir = Join-Path $RootDir "Limone"
  if (Test-Path $clipperDir) {
    $ProjectDir = $clipperDir
  } elseif (Test-Path $limoneDir) {
    $ProjectDir = $limoneDir
  } else {
    throw "Project dir not found: $clipperDir or $limoneDir. Set PROJECT_DIR to override."
  }
}
$CMakeProject = if ($env:CMAKE_PROJECT_NAME) { $env:CMAKE_PROJECT_NAME } else { "" }
if ([string]::IsNullOrWhiteSpace($CMakeProject)) {
  $leaf = Split-Path -Leaf $ProjectDir
  $CMakeProject = if ($leaf -ieq "ClipperLimiter") { "ClipperLimiter" } else { "LimOne" }
}
$BuildDir = if ($env:BUILD_DIR) { $env:BUILD_DIR } else { (Join-Path $RootDir "build-win") }
$OutDir = if ($env:OUT_DIR) { $env:OUT_DIR } else { (Join-Path $RootDir "dist") }
$Config = if ($env:CONFIG) { $env:CONFIG } else { "Release" }
$Generator = if ($env:CMAKE_GENERATOR) { $env:CMAKE_GENERATOR } else { "Visual Studio 17 2022" }
$Arch = if ($env:CMAKE_ARCH) { $env:CMAKE_ARCH } else { "x64" }
$BuildAAX  = if ($env:BUILD_AAX)    { [bool]::Parse($env:BUILD_AAX) }    else { $true }
$SkipBuild = if ($env:SKIP_BUILD)   { [bool]::Parse($env:SKIP_BUILD) }  else { $false }

if (-not (Test-Path $ProjectDir)) {
  throw "Project dir not found: $ProjectDir"
}

New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

if (-not $SkipBuild) {
  $CMake = Get-Command cmake -ErrorAction Stop

  & $CMake.Source -S $ProjectDir -B $BuildDir -G $Generator -A $Arch

  $Targets = @("${CMakeProject}_VST3", "${CMakeProject}_Standalone")
  if ($BuildAAX) { $Targets += "${CMakeProject}_AAX" }

  foreach ($t in $Targets) {
    try {
      & $CMake.Source --build $BuildDir --config $Config --target $t
    } catch {
      if ($t -eq "${CMakeProject}_AAX") {
        Write-Host "AAX build failed; continuing. $($_.Exception.Message)"
      } else {
        throw
      }
    }
  }
}

# Resolve artefacts dir: auto-detect Release/Debug config subfolder
function Resolve-ArtefactsDir([string]$Dir) {
  if (-not (Test-Path $Dir)) { return $null }
  foreach ($cfg in @('Release','Debug','RelWithDebInfo','MinSizeRel')) {
    $sub = Join-Path $Dir $cfg
    if ((Test-Path (Join-Path $sub 'VST3')) -or
        (Test-Path (Join-Path $sub 'AAX'))  -or
        (Test-Path (Join-Path $sub 'Standalone'))) {
      return $sub
    }
  }
  if ((Test-Path (Join-Path $Dir 'VST3')) -or
      (Test-Path (Join-Path $Dir 'AAX'))  -or
      (Test-Path (Join-Path $Dir 'Standalone'))) {
    return $Dir
  }
  return $null
}

$ArtefactsDir = if ($env:ARTEFACTS_DIR) { $env:ARTEFACTS_DIR } `
                else { Join-Path $BuildDir ("{0}_artefacts" -f $CMakeProject) }

$resolved = Resolve-ArtefactsDir $ArtefactsDir
if ($resolved) {
  $ArtefactsDir = $resolved
} else {
  throw "Artefacts dir not found: $ArtefactsDir"
}

$PluginName = if ($env:PLUGIN_NAME) { $env:PLUGIN_NAME } else { "" }
if ([string]::IsNullOrWhiteSpace($PluginName)) {
  $vst3 = Get-ChildItem -Path (Join-Path $ArtefactsDir "VST3") -Filter "*.vst3" -Directory -ErrorAction SilentlyContinue | Select-Object -First 1
  if ($vst3) {
    $PluginName = [System.IO.Path]::GetFileNameWithoutExtension($vst3.Name)
  } else {
    $aax = Get-ChildItem -Path (Join-Path $ArtefactsDir "AAX") -Filter "*.aaxplugin" -Directory -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($aax) {
      $PluginName = [System.IO.Path]::GetFileNameWithoutExtension($aax.Name)
    } else {
      $PluginName = if ($CMakeProject -eq "LimOne") { "Lim.one" } else { $CMakeProject }
    }
  }
}

$Version = if ($env:VERSION) { $env:VERSION } else { "" }
if ([string]::IsNullOrWhiteSpace($Version)) {
  $cmakeList = Join-Path $ProjectDir "CMakeLists.txt"
  if (Test-Path $cmakeList) {
    $text = Get-Content -Raw -Path $cmakeList
    $pattern = ('project\s*\(\s*{0}\s+VERSION\s+([0-9]+\.[0-9]+\.[0-9]+)\s*\)' -f [regex]::Escape($CMakeProject))
    $m = [regex]::Match($text, $pattern, [System.Text.RegularExpressions.RegexOptions]::IgnoreCase)
    if ($m.Success) { $Version = $m.Groups[1].Value }
  }
  if ([string]::IsNullOrWhiteSpace($Version)) { $Version = "0.0.0" }
}

$StageRoot = Join-Path $env:TEMP ("{0}-{1}-win" -f $PluginName, $Version)
if (Test-Path $StageRoot) { Remove-Item -Recurse -Force $StageRoot }
New-Item -ItemType Directory -Force -Path $StageRoot | Out-Null

$Vst3Src = Join-Path $ArtefactsDir "VST3"
$AaxSrc = Join-Path $ArtefactsDir "AAX"
$StandaloneSrc = Join-Path $ArtefactsDir "Standalone"

$Vst3Bundle = Join-Path $Vst3Src ("{0}.vst3" -f $PluginName)
$AaxBundle = Join-Path $AaxSrc ("{0}.aaxplugin" -f $PluginName)

if (Test-Path $Vst3Bundle) {
  New-Item -ItemType Directory -Force -Path (Join-Path $StageRoot "VST3") | Out-Null
  Copy-Item -Recurse -Force $Vst3Bundle (Join-Path $StageRoot "VST3")
}

if (Test-Path $AaxBundle) {
  New-Item -ItemType Directory -Force -Path (Join-Path $StageRoot "AAX") | Out-Null
  Copy-Item -Recurse -Force $AaxBundle (Join-Path $StageRoot "AAX")
}

if (Test-Path $StandaloneSrc) {
  New-Item -ItemType Directory -Force -Path (Join-Path $StageRoot "Standalone") | Out-Null
  Copy-Item -Recurse -Force (Join-Path $StandaloneSrc "*") (Join-Path $StageRoot "Standalone")
}

$ZipPath = Join-Path $OutDir ("{0}-{1}-win.zip" -f $PluginName, $Version)
if (Test-Path $ZipPath) { Remove-Item -Force $ZipPath }
Compress-Archive -Path $StageRoot -DestinationPath $ZipPath -Force

Write-Host $ZipPath

$MakeNSIS = if ($env:MAKE_NSIS) { [bool]::Parse($env:MAKE_NSIS) } else { $true }
if ($MakeNSIS) {
  $makensis = Get-Command makensis -ErrorAction SilentlyContinue
  if ($makensis) {
    $NsisScript = Join-Path $env:TEMP ("{0}-{1}-installer.nsi" -f $PluginName, $Version)
    $ExePath = Join-Path $OutDir ("{0}-{1}-win-installer.exe" -f $PluginName, $Version)
    $PayloadDir = $StageRoot

    $nsis = New-Object System.Collections.Generic.List[string]
    $nsis.Add('Unicode true')
    $nsis.Add('RequestExecutionLevel admin')
    $nsis.Add(('Name "{0}"' -f $PluginName))
    $nsis.Add(('OutFile "{0}"' -f $ExePath))
    $nsis.Add(('InstallDir "$PROGRAMFILES64\{0}"' -f $PluginName))
    $nsis.Add('ShowInstDetails show')
    $nsis.Add('ShowUninstDetails show')
    $nsis.Add('!include "x64.nsh"')
    $nsis.Add('')
    $nsis.Add('Section "Install"')
    $nsis.Add('  SetShellVarContext all')
    $nsis.Add('  ${IfNot} ${RunningX64}')
    $nsis.Add('    MessageBox MB_ICONSTOP "Requires 64-bit Windows."')
    $nsis.Add('    Abort')
    $nsis.Add('  ${EndIf}')
    $nsis.Add('')
    $nsis.Add('  CreateDirectory "$INSTDIR"')
    $nsis.Add('  SetOutPath "$INSTDIR"')
    $nsis.Add('  WriteUninstaller "$INSTDIR\Uninstall.exe"')
    $nsis.Add('')

    $payloadVst3 = Join-Path $PayloadDir "VST3"
    if (Test-Path $payloadVst3) {
      $nsis.Add('  CreateDirectory "$COMMONFILES64\VST3"')
      $nsis.Add('  SetOutPath "$COMMONFILES64\VST3"')
      $nsis.Add(('  File /r "{0}"' -f (Join-Path $payloadVst3 "*.*")))
      $nsis.Add('')
    }

    $payloadAax = Join-Path $PayloadDir "AAX"
    if (Test-Path $payloadAax) {
      $nsis.Add('  CreateDirectory "$COMMONFILES64\Avid\Audio\Plug-Ins"')
      $nsis.Add('  SetOutPath "$COMMONFILES64\Avid\Audio\Plug-Ins"')
      $nsis.Add(('  File /r "{0}"' -f (Join-Path $payloadAax "*.*")))
      $nsis.Add('')
    }

    $payloadStandalone = Join-Path $PayloadDir "Standalone"
    if (Test-Path $payloadStandalone) {
      $nsis.Add('  SetOutPath "$INSTDIR"')
      $nsis.Add(('  File /r "{0}"' -f (Join-Path $payloadStandalone "*.*")))
      $nsis.Add('')
    }

    $nsis.Add('SectionEnd')
    $nsis.Add('')
    $nsis.Add('Section "Uninstall"')
    $nsis.Add('  SetShellVarContext all')
    $nsis.Add('  Delete "$INSTDIR\Uninstall.exe"')
    $nsis.Add('  RMDir /r "$INSTDIR"')
    $nsis.Add('SectionEnd')

    $nsis | Set-Content -Encoding UTF8 -Path $NsisScript

    try {
      & $makensis.Source $NsisScript | Out-Null
      Write-Host $ExePath
    } catch {
      Write-Warning "NSIS installer creation failed (non-fatal): $_"
    }
  }
}
