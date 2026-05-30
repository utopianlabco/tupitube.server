###########################################################################
#   Project TupiTube Server                                               #
#   Project Contact: info@tupitube.com                                    #
#   Project Website: http://www.tupitube.com                              #
#                                                                         #
#   License: GNU General Public License v2                                #
###########################################################################
#
# configure.ps1 - Windows configure script for TupiTube Server
#
# Usage:
#   .\configure.ps1 -TupitubeDir <path> [-DataDir <path>] [-DebugBuild]
#                   [-QtDir <path>] [-FfmpegDir <path>] [-QuazipDir <path>]
#
# Parameters:
#   -TupitubeDir   (required) Path to the TupiTube Desk build/install directory
#   -DataDir       (optional) Default data directory for database and projects
#   -DebugBuild    (optional) Enable debug output (adds TUP_DEBUG define)
#   -QtDir         (optional) Qt 5.15 MinGW-64 kit root (default: C:\devel\Qt\5.15.2\mingw81_64)
#   -FfmpegDir     (optional) Path to FFmpeg root (default: C:/ffmpeg)
#   -QuazipDir     (optional) Path to QuaZip root  (default: C:/Quazip)

param(
    [Parameter(Mandatory = $true,
               HelpMessage = "Path to TupiTube Desk build/install directory")]
    [string]$TupitubeDir,

    [Parameter(HelpMessage = "Default data directory for database and projects")]
    [string]$DataDir = "",

    [Parameter(HelpMessage = "Enable debug output")]
    [switch]$DebugBuild,

    [Parameter(HelpMessage = "Path to FFmpeg root directory (default: C:/ffmpeg)")]
    [string]$FfmpegDir = "C:/ffmpeg",

    [Parameter(HelpMessage = "Path to QuaZip root directory (default: C:/Quazip)")]
    [string]$QuazipDir = "C:/Quazip",

    [Parameter(HelpMessage = "Qt 5.15 MinGW-64 kit root (default: C:\\devel\\Qt\\5.15.2\\mingw81_64)")]
    [string]$QtDir = "C:\devel\Qt\5.15.2\mingw81_64"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

# Convert a filesystem path to forward-slash form suitable for qmake .pri files
function ConvertTo-QmakePath([string]$path) {
    return $path.Replace('\', '/')
}

Write-Host ""
Write-Host "TupiTube Server - Windows Configure"
Write-Host "====================================`n"

# ---------------------------------------------------------------------------
# Validate Qt directory and add its bin to PATH for this session
# ---------------------------------------------------------------------------
$qtBinDir = Join-Path $QtDir "bin"
if (-not (Test-Path $qtBinDir)) {
    Write-Error "Qt bin directory not found: $qtBinDir`nEnsure -QtDir points to a valid Qt 5.15 MinGW-64 installation."
    exit 1
}
# Prepend Qt bin so qmake, windeployqt and lrelease are all found
$env:PATH = "$qtBinDir;$env:PATH"

$qmakeCmd = Get-Command qmake -ErrorAction SilentlyContinue
if (-not $qmakeCmd) {
    Write-Error "qmake not found in '$qtBinDir'. Check your Qt installation."
    exit 1
}

$qtVersion = (& qmake -query QT_VERSION 2>$null).Trim()
Write-Host "Qt version ............... $qtVersion"
Write-Host "Qt directory ............. $QtDir"

# ---------------------------------------------------------------------------
# Validate TupiTube Desk directory
# ---------------------------------------------------------------------------
if (-not (Test-Path $TupitubeDir)) {
    Write-Error "TupiTube Desk directory not found: $TupitubeDir"
    exit 1
}
$tupiDir = ConvertTo-QmakePath((Resolve-Path $TupitubeDir).Path)
Write-Host "TupiTube Desk dir ........ $tupiDir"

# ---------------------------------------------------------------------------
# Validate FFmpeg directory
# ---------------------------------------------------------------------------
if (-not (Test-Path $FfmpegDir)) {
    Write-Warning "FFmpeg directory not found at '$FfmpegDir'. The build may fail."
}
$_ffmpegResolved = Resolve-Path $FfmpegDir -ErrorAction SilentlyContinue
$_ffmpegPath = if ($_ffmpegResolved) { $_ffmpegResolved.Path } else { $FfmpegDir }
$ffmpegDir = ConvertTo-QmakePath $_ffmpegPath
Write-Host "FFmpeg dir ............... $ffmpegDir"

# ---------------------------------------------------------------------------
# Validate QuaZip directory
# ---------------------------------------------------------------------------
if (-not (Test-Path $QuazipDir)) {
    Write-Warning "QuaZip directory not found at '$QuazipDir'. The build may fail."
}
$_quazipResolved = Resolve-Path $QuazipDir -ErrorAction SilentlyContinue
$_quazipPath = if ($_quazipResolved) { $_quazipResolved.Path } else { $QuazipDir }
$quazipDir = ConvertTo-QmakePath $_quazipPath
Write-Host "QuaZip dir ............... $quazipDir"

# ---------------------------------------------------------------------------
# Update quazip.win.pri and ffmpeg.win.pri if non-default paths were supplied
# ---------------------------------------------------------------------------
$projectRoot = $PSScriptRoot

if ($FfmpegDir -ne "C:/ffmpeg") {
    $ffmpegPri = Join-Path $projectRoot "ffmpeg.win.pri"
    $ffmpegLibs = @(
        "LIBS += -L$ffmpegDir/bin -lavdevice-61 -lavformat-61 -lavfilter-10 -lavcodec-61 -lswresample-5 -lswscale-8 -lavutil-59",
        "INCLUDEPATH += $ffmpegDir/include"
    )
    $ffmpegLibs | Set-Content -Path $ffmpegPri -Encoding ASCII
    Write-Host "Updated ffmpeg.win.pri with custom path."
}

if ($QuazipDir -ne "C:/Quazip") {
    $quazipPri = Join-Path $projectRoot "quazip.win.pri"
    $quazipLibs = @(
        "LIBS += -L$quazipDir/bin/ -lquazip1-qt5",
        "INCLUDEPATH += $quazipDir/include/quazip"
    )
    $quazipLibs | Set-Content -Path $quazipPri -Encoding ASCII
    Write-Host "Updated quazip.win.pri with custom path."
}

# ---------------------------------------------------------------------------
# Create bin/ output directory
# ---------------------------------------------------------------------------
$binDir = Join-Path $projectRoot "bin"
if (-not (Test-Path $binDir)) {
    New-Item -ItemType Directory -Path $binDir | Out-Null
    Write-Host "Created output directory: bin/"
}

# ---------------------------------------------------------------------------
# Build tupitube_config.pri content
# ---------------------------------------------------------------------------
$includePaths = @(
    "$tupiDir/src/framework",
    "$tupiDir/src/framework/core",
    "$tupiDir/src/framework/gui",
    "$tupiDir/src/libbase",
    "$tupiDir/src/store",
    "$tupiDir/src/libtupi",
    "$tupiDir/src/plugins/export/ffmpegplugin"
)

$libs = @(
    "-L$tupiDir/src/framework/core/release -ltupifwcore",
    "-L$tupiDir/src/framework/gui/release -ltupifwgui",
    "-L$tupiDir/src/libbase/release -ltupibase",
    "-L$tupiDir/src/store/release -ltupistore",
    "-L$tupiDir/src/libtupi/release -ltupi"
)

$qtModules = "core gui svg xml network"

$defines = [System.Collections.Generic.List[string]]::new()

if ($DebugBuild) {
    $defines.Add("TUP_DEBUG")
    Write-Host "Debug support ............ [ ON ]"
} else {
    $defines.Add("TUP_NODEBUG")
    Write-Host "Debug support ............ [ OFF ]"
}

if ($DataDir -ne "") {
    $_dataDirResolved = Resolve-Path $DataDir -ErrorAction SilentlyContinue
    $_dataDirPath = if ($_dataDirResolved) { $_dataDirResolved.Path } else { $DataDir }
    $resolvedDataDir = ConvertTo-QmakePath $_dataDirPath
    # Escape quotes for qmake DEFINES
    $defines.Add('DEFAULT_DATA_PATH=\\\"' + $resolvedDataDir + '\\\"')
    Write-Host "Data directory ........... $resolvedDataDir"
}

$configOptions = [System.Collections.Generic.List[string]]::new()
if (-not $DebugBuild) {
    $configOptions.Add("silent")
}

$timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"

$priLines = [System.Collections.Generic.List[string]]::new()
$priLines.Add("# Generated automatically at $timestamp! PLEASE DO NOT EDIT!")
$priLines.Add("INCLUDEPATH += $($includePaths -join ' ')")
$priLines.Add("LIBS += $($libs -join ' ')")
$priLines.Add("QT += $qtModules")
$priLines.Add("DEFINES += $($defines -join ' ')")
$priLines.Add("win32 {")
$priLines.Add("    MOC_DIR = .moc")
$priLines.Add("    UI_DIR = .ui")
$priLines.Add("    OBJECTS_DIR = .obj")
$priLines.Add("}")
if ($configOptions.Count -gt 0) {
    $priLines.Add("CONFIG += $($configOptions -join ' ')")
}

$priPath = Join-Path $projectRoot "tupitube_config.pri"
$utf8NoBom = [System.Text.UTF8Encoding]::new($false)
[System.IO.File]::WriteAllLines($priPath, $priLines, $utf8NoBom)
Write-Host "Generated: tupitube_config.pri"

# ---------------------------------------------------------------------------
# Compile translation files (.ts -> .qm) if lrelease is available
# ---------------------------------------------------------------------------
$lrelease = Get-Command lrelease -ErrorAction SilentlyContinue
$tsDir    = Join-Path $projectRoot "src\shell\data\translations"
if ($lrelease -and (Test-Path $tsDir)) {
    $tsFiles = @(Get-ChildItem -Path $tsDir -Filter "*.ts")
    if ($tsFiles.Count -gt 0) {
        foreach ($ts in $tsFiles) {
            & lrelease $ts.FullName 2>$null
        }
        Write-Host "Translations compiled .... [ OK ]"
    } else {
        Write-Host "Translations compiled .... [ NO FILES ]"
    }
} else {
    Write-Host "Translations compiled .... [ SKIPPED - lrelease not found ]"
}

# ---------------------------------------------------------------------------
# Run qmake to generate Makefiles
# ---------------------------------------------------------------------------
Write-Host "`nRunning qmake..."
Push-Location $projectRoot
try {
    & qmake -r tupitube.server.pro
    if ($LASTEXITCODE -ne 0) {
        Write-Error "qmake failed with exit code $LASTEXITCODE."
        exit 1
    }
} finally {
    Pop-Location
}

Write-Host ""
Write-Host "Configuration complete."
Write-Host "----------------------------------------------------"
Write-Host "Next steps:"
Write-Host "  1. make -j4"
Write-Host "  2. powershell -ExecutionPolicy Bypass -File .\deploy.ps1 -TupitubeDir $TupitubeDir"
Write-Host "  3. `"C:\Program Files (x86)\Inno Setup 6\iscc.exe`" installer\tupitube_server.iss"
Write-Host ""
