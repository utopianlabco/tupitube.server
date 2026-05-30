###########################################################################
#   Project TupiTube Server                                               #
#   Project Contact: info@tupitube.com                                    #
#   Project Website: http://www.tupitube.com                              #
#                                                                         #
#   License: GNU General Public License v2                                #
###########################################################################
#
# deploy.ps1 - Post-build deployment script for TupiTube Server (Windows)
#
# Run this AFTER a successful build (make) to collect all runtime
# dependencies into bin\ and prepare the directory for InnoSetup.
#
# Usage:
#   .\deploy.ps1 [-TupitubeDir <path>] [-QtDir <path>] [-FfmpegDir <path>] [-QuazipDir <path>]
#
# Parameters:
#   -TupitubeDir  Path to the TupiTube Desk source tree (same value as
#                 configure.ps1 -TupitubeDir; default: C:\devel\sources\tupitube.desk)
#   -QtDir        Qt 5.15 MinGW-64 kit root
#                 (default: C:\devel\Qt\5.15.2\mingw81_64)
#   -FfmpegDir    FFmpeg root directory (default: C:\ffmpeg)
#   -QuazipDir    QuaZip root directory (default: C:\Quazip)
#   -SndfileDir   libsndfile root directory (default: C:\devel\sources\libsndfile)

param(
    [Parameter(HelpMessage = "Path to the TupiTube Desk source tree")]
    [string]$TupitubeDir = "C:\devel\sources\tupitube.desk",

    [Parameter(HelpMessage = "Qt 5.15 MinGW-64 kit root")]
    [string]$QtDir = "C:\devel\Qt\5.15.2\mingw81_64",

    [Parameter(HelpMessage = "FFmpeg root directory")]
    [string]$FfmpegDir = "C:\ffmpeg",

    [Parameter(HelpMessage = "QuaZip root directory")]
    [string]$QuazipDir = "C:\Quazip",

    [Parameter(HelpMessage = "libsndfile root directory")]
    [string]$SndfileDir = "C:\devel\sources\libsndfile"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$projectRoot = $PSScriptRoot
$binDir      = Join-Path $projectRoot "bin"
$serverExe   = Join-Path $binDir "tupitube.server.exe"

Write-Host ""
Write-Host "TupiTube Server - Windows Deploy"
Write-Host "================================`n"

# ---------------------------------------------------------------------------
# Verify the binary exists
# ---------------------------------------------------------------------------
if (-not (Test-Path $serverExe)) {
    Write-Error "Binary not found: $serverExe`nBuild the project first with: make -j4"
    exit 1
}
Write-Host "Binary found ............. $serverExe"

# ---------------------------------------------------------------------------
# Validate TupiTube Desk source tree
# ---------------------------------------------------------------------------
if (-not (Test-Path $TupitubeDir)) {
    Write-Error "TupiTube Desk source tree not found: $TupitubeDir"
    exit 1
}

# ---------------------------------------------------------------------------
# Validate Qt directory
# ---------------------------------------------------------------------------
$qtBinDir = Join-Path $QtDir "bin"
if (-not (Test-Path $qtBinDir)) {
    Write-Error "Qt bin directory not found: $qtBinDir"
    exit 1
}
# Add Qt bin to PATH for windeployqt / lrelease
$env:PATH = "$qtBinDir;$env:PATH"

# ---------------------------------------------------------------------------
# Step 1: Copy TupiTube Desk runtime DLLs into bin\
# These are the specific DLLs the server links against, sourced directly
# from the source tree's release/ directories (not from the installed app).
# ---------------------------------------------------------------------------
Write-Host "`n[1/7] Copying TupiTube Desk DLLs..."
$tupiDlls = @(
    "src\framework\core\release\tupifwcore.dll",
    "src\framework\gui\release\tupifwgui.dll",
    "src\libbase\release\tupibase.dll",
    "src\store\release\tupistore.dll",
    "src\libtupi\release\tupi.dll"
)
$copiedCount = 0
foreach ($rel in $tupiDlls) {
    $src = Join-Path $TupitubeDir $rel
    if (Test-Path $src) {
        Copy-Item $src -Destination $binDir -Force
        $copiedCount++
    } else {
        Write-Warning "DLL not found (build TupiTube Desk first?): $src"
    }
}
Write-Host "      $copiedCount of $($tupiDlls.Count) TupiTube Desk DLL(s) copied"

# Copy tupiffmpegplugin.dll into bin\plugins\ so the server's QPluginLoader
# can find it at runtime (PLUGINS_DIR = <exe dir>/plugins on Windows).
$srcPlugin = Join-Path $TupitubeDir "src\plugins\export\ffmpegplugin\release\tupiffmpegplugin.dll"
if (Test-Path $srcPlugin) {
    $destPluginsDir = Join-Path $binDir "plugins"
    New-Item -ItemType Directory -Path $destPluginsDir -Force | Out-Null
    Copy-Item $srcPlugin -Destination $destPluginsDir -Force
    Write-Host "      Copied tupiffmpegplugin.dll -> bin\plugins\"
} else {
    Write-Warning "tupiffmpegplugin.dll not found: $srcPlugin`n      Build TupiTube Desk ffmpegplugin first."
}

# ---------------------------------------------------------------------------
# Step 2: Copy sndfile runtime DLL (required by tupifwcore.dll)
# ---------------------------------------------------------------------------
Write-Host "`n[2/7] Copying sndfile DLL..."
$sndfileBinDir = Join-Path $SndfileDir "bin"
$sndfileDll = Join-Path $sndfileBinDir "sndfile.dll"
if (-not (Test-Path $sndfileDll)) {
    Write-Error "sndfile.dll not found: $sndfileDll`nCheck your -SndfileDir parameter."
    exit 1
}
Copy-Item $sndfileDll -Destination $binDir -Force
Write-Host "      Copied sndfile.dll -> bin\"

# ---------------------------------------------------------------------------
# Step 3: Copy FFmpeg runtime DLLs
# ---------------------------------------------------------------------------
Write-Host "`n[3/7] Copying FFmpeg DLLs..."
$ffmpegBinDir = Join-Path $FfmpegDir "bin"
if (-not (Test-Path $ffmpegBinDir)) {
    Write-Error "FFmpeg bin directory not found: $ffmpegBinDir`nCheck your -FfmpegDir parameter."
    exit 1
}
$ffmpegDlls = @(
    "avutil-59.dll",
    "avcodec-61.dll",
    "avformat-61.dll",
    "avdevice-61.dll",
    "avfilter-10.dll",
    "swresample-5.dll",
    "swscale-8.dll"
)
$copiedCount = 0
foreach ($dll in $ffmpegDlls) {
    $src = Join-Path $ffmpegBinDir $dll
    if (Test-Path $src) {
        Copy-Item $src -Destination $binDir -Force
        $copiedCount++
    } else {
        Write-Warning "FFmpeg DLL not found: $src"
    }
}
Write-Host "      $copiedCount of $($ffmpegDlls.Count) FFmpeg DLL(s) copied"

# ---------------------------------------------------------------------------
# Step 4: Copy QuaZip runtime DLL
# ---------------------------------------------------------------------------
Write-Host "`n[4/7] Copying QuaZip DLL..."
$quazipBinDir = Join-Path $QuazipDir "bin"
$quazipDll = Join-Path $quazipBinDir "libquazip1-qt5.dll"
if (-not (Test-Path $quazipDll)) {
    Write-Error "QuaZip DLL not found: $quazipDll`nCheck your -QuazipDir parameter."
    exit 1
}
Copy-Item $quazipDll -Destination $binDir -Force
Write-Host "      Copied libquazip1-qt5.dll -> bin\"

# ---------------------------------------------------------------------------
# Step 5: Copy Qt5Sql.dll (not shipped with TupiTube Desk)
# ---------------------------------------------------------------------------
Write-Host "`n[5/7] Copying Qt5Sql.dll..."
$qt5SqlSrc = Join-Path $QtDir "bin\Qt5Sql.dll"
if (-not (Test-Path $qt5SqlSrc)) {
    Write-Error "Qt5Sql.dll not found at: $qt5SqlSrc`nCheck your -QtDir parameter."
    exit 1
}
Copy-Item $qt5SqlSrc -Destination $binDir -Force
Write-Host "      Copied Qt5Sql.dll -> bin\"

# ---------------------------------------------------------------------------
# Step 6: Copy sqldrivers\qsqlite.dll (not shipped with TupiTube Desk)
# ---------------------------------------------------------------------------
Write-Host "`n[6/7] Copying sqldrivers\qsqlite.dll..."
$qsqliteSrc = Join-Path $QtDir "plugins\sqldrivers\qsqlite.dll"
if (-not (Test-Path $qsqliteSrc)) {
    Write-Error "qsqlite.dll not found at: $qsqliteSrc`nCheck your -QtDir parameter."
    exit 1
}
$sqldriversDest = Join-Path $binDir "sqldrivers"
New-Item -ItemType Directory -Path $sqldriversDest -Force | Out-Null
Copy-Item $qsqliteSrc -Destination $sqldriversDest -Force
Write-Host "      Copied qsqlite.dll -> bin\sqldrivers\"

# ---------------------------------------------------------------------------
# Step 7: Run windeployqt to collect remaining Qt runtime dependencies
# Runs after our manual copies so windeployqt sees the full dependency set.
# Qt 5.15.2 MinGW windeployqt may report "Unable to find the platform plugin"
# and exit with code 1 even when it deploys successfully; we copy
# platforms\qwindows.dll explicitly as a safety net and treat that specific
# failure as a non-fatal warning.
# ---------------------------------------------------------------------------
Write-Host "`n[7/7] Running windeployqt..."
$windeployqt = Join-Path $qtBinDir "windeployqt.exe"
if (-not (Test-Path $windeployqt)) {
    Write-Error "windeployqt.exe not found at: $windeployqt"
    exit 1
}

# Ensure platforms\qwindows.dll is present regardless of windeployqt behaviour
$qwindowsSrc  = Join-Path $QtDir "plugins\platforms\qwindows.dll"
$platformsDest = Join-Path $binDir "platforms"
if (Test-Path $qwindowsSrc) {
    New-Item -ItemType Directory -Path $platformsDest -Force | Out-Null
    Copy-Item $qwindowsSrc -Destination $platformsDest -Force
    Write-Host "      Pre-copied platforms\qwindows.dll -> bin\platforms\"
} else {
    Write-Warning "qwindows.dll not found at: $qwindowsSrc"
}

& $windeployqt --release $serverExe
if ($LASTEXITCODE -ne 0) {
    Write-Warning "windeployqt exited with code $LASTEXITCODE (platform-plugin false positive is common with Qt 5.15.2 MinGW; verifying critical files...)"
    if (-not (Test-Path (Join-Path $platformsDest "qwindows.dll"))) {
        Write-Error "platforms\qwindows.dll is missing after windeployqt - deployment incomplete."
        exit 1
    }
    Write-Host "      Critical platform plugin present. Continuing."
}

# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------
$fileCount = (Get-ChildItem -Path $binDir -Recurse -File).Count
Write-Host ""
Write-Host "Deployment complete."
Write-Host "----------------------------------------------------"
Write-Host "bin\ now contains $fileCount file(s) and is ready for InnoSetup."
Write-Host ""
Write-Host "Build installer with:"
Write-Host "    `"C:\Program Files (x86)\Inno Setup 6\iscc.exe`" installer\tupitube_server.iss"
Write-Host ""
