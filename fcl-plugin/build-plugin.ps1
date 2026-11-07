# build-plugin.ps1 -- build the FCL renderer plugin APK using SDK command-line tools
#
# This intentionally does NOT use Gradle or the Android Gradle Plugin. The plugin
# is a tiny "shell" APK whose only jobs are:
#   1. to be discoverable by FCL's PluginManager (needs an ACTION_MAIN activity
#      plus the fclPlugin meta-data), and
#   2. to ship libgl_gles.so under lib/<abi>/ so FCL can dlopen it.
# Driving aapt2/javac/d8/zipalign/apksigner directly keeps the build fast and
# avoids dependency downloads (several mavens are unreachable from this machine).
#
# Usage:
#   .\fcl-plugin\build-plugin.ps1                # rebuild the native libs too
#   .\fcl-plugin\build-plugin.ps1 -SkipNative    # reuse existing .so files
#   .\fcl-plugin\build-plugin.ps1 -Clean
#
# ASCII-only on purpose: PowerShell 5.1 decodes .ps1 with the system codepage.
#
# LGPL-3.0-or-later

param(
    [switch]$SkipNative,
    [switch]$Clean
)

$ErrorActionPreference = "Stop"

# ----------------------------------------------------------------------
# Native tool invocation helper
#
# Windows PowerShell 5.1 wraps ANY stderr output from a native executable in an
# ErrorRecord. Combined with $ErrorActionPreference = "Stop", that aborts the
# script even when the tool succeeded and output was redirected via 2>&1.
# keytool and apksigner both write ordinary progress text to stderr, so without
# this helper they would always appear to fail.
#
# The helper temporarily relaxes ErrorActionPreference, captures both streams to
# a log file, and relies solely on the process exit code.
# ----------------------------------------------------------------------
function Invoke-NativeTool {
    param(
        [Parameter(Mandatory)][string]   $Exe,
        [Parameter(Mandatory)][string[]] $Arguments,
        [Parameter(Mandatory)][string]   $LogPath,
        [string] $Label = ""
    )
    $prev = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    try {
        & $Exe @Arguments > $LogPath 2>&1
        $code = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $prev
    }
    if ($code -ne 0) {
        Write-Host ""
        Write-Host ("--- {0} failed (exit {1}) {2} ---" -f $Exe, $code, $(if ($Label) { "[$Label]" } else { "" }))
        Get-Content $LogPath -ErrorAction SilentlyContinue |
            Select-Object -Last 30 | ForEach-Object { Write-Host "  $_" }
        throw ("{0} failed with exit code {1}" -f $Exe, $code)
    }
    return $code
}

# ---- Toolchain --------------------------------------------------------
$sdkRoot    = if ($env:ANDROID_HOME) { $env:ANDROID_HOME } else { "C:\Android\Sdk" }
$buildTools = Join-Path $sdkRoot "build-tools\34.0.0"
$aapt2      = Join-Path $buildTools "aapt2.exe"
$zipalign   = Join-Path $buildTools "zipalign.exe"
$apksigner  = Join-Path $buildTools "apksigner.bat"
$d8         = Join-Path $buildTools "d8.bat"
$androidJar = Join-Path $sdkRoot "platforms\android-34\android.jar"

$javaHome = if ($env:JAVA_HOME) { $env:JAVA_HOME } else { "C:\Program Files\Java\jdk-21.0.12" }
$javac    = Join-Path $javaHome "bin\javac.exe"
$jar      = Join-Path $javaHome "bin\jar.exe"
$keytool  = Join-Path $javaHome "bin\keytool.exe"

foreach ($t in @($aapt2, $zipalign, $apksigner, $d8, $androidJar, $javac, $jar, $keytool)) {
    if (-not (Test-Path $t)) { throw "missing tool: $t" }
}

$pluginDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$root      = Split-Path -Parent $pluginDir
$work      = Join-Path $pluginDir "build"
$outDir    = Join-Path $root "build\plugin"

if ($Clean -and (Test-Path $work)) { Remove-Item $work -Recurse -Force }
foreach ($d in @("classes", "dex", "apk", "logs")) {
    New-Item -ItemType Directory -Force -Path (Join-Path $work $d) | Out-Null
}
New-Item -ItemType Directory -Force -Path $outDir | Out-Null
$logDir = Join-Path $work "logs"

Write-Host "=============================================="
Write-Host " FCL renderer plugin build"
Write-Host "=============================================="

$abis = @("arm64-v8a", "armeabi-v7a")

# ---- 1. Native libraries ---------------------------------------------
if (-not $SkipNative) {
    Write-Host ""
    Write-Host "=== [1/6] building native libraries ==="
    $nativeScript = Join-Path $root "native\build-native.ps1"
    foreach ($abi in $abis) {
        & $nativeScript -Abi $abi
        if ($LASTEXITCODE -ne 0) { throw "native build failed for $abi" }
    }
} else {
    Write-Host ""
    Write-Host "=== [1/6] native libraries (skipped) ==="
}

# FCL resolves the .so via ApplicationInfo.nativeLibraryDir, so it must live at
# lib/<abi>/ inside the APK.
foreach ($abi in $abis) {
    $src = Join-Path $root "build\native\$abi\libgl_gles.so"
    if (-not (Test-Path $src)) { throw "native lib not found: $src (run without -SkipNative)" }
    $dstDir = Join-Path $work "apk\lib\$abi"
    New-Item -ItemType Directory -Force -Path $dstDir | Out-Null
    Copy-Item $src (Join-Path $dstDir "libgl_gles.so") -Force
    Write-Host ("  staged lib/{0}/libgl_gles.so ({1} KB)" -f $abi, [math]::Round((Get-Item $src).Length/1KB,1))
}

# ---- 2. Compile resources --------------------------------------------
Write-Host ""
Write-Host "=== [2/6] aapt2 compile ==="
$resZip = Join-Path $work "res.zip"
Invoke-NativeTool -Exe $aapt2 `
    -Arguments @("compile", "--dir", (Join-Path $pluginDir "res"), "-o", $resZip) `
    -LogPath (Join-Path $logDir "aapt2-compile.log") -Label "aapt2 compile" | Out-Null
Write-Host "  ok"

# ---- 3. Link resources + manifest ------------------------------------
Write-Host ""
Write-Host "=== [3/6] aapt2 link ==="
$baseApk = Join-Path $work "base.apk"
Invoke-NativeTool -Exe $aapt2 `
    -Arguments @(
        "link",
        "-o", $baseApk,
        "-I", $androidJar,
        "--manifest", (Join-Path $pluginDir "AndroidManifest.xml"),
        $resZip,
        "--min-sdk-version", "26",
        "--target-sdk-version", "34",
        "--version-code", "2",
        "--version-name", "1.1.0",
        "--auto-add-overlay"
    ) `
    -LogPath (Join-Path $logDir "aapt2-link.log") -Label "aapt2 link" | Out-Null
Write-Host "  ok"

# ---- 4. Compile Java -------------------------------------------------
Write-Host ""
Write-Host "=== [4/6] javac + d8 ==="
$classesDir = Join-Path $work "classes"
$sources = @(Get-ChildItem (Join-Path $pluginDir "src") -Recurse -Filter *.java |
             Select-Object -ExpandProperty FullName)
if ($sources.Count -eq 0) { throw "no java sources found under fcl-plugin/src/" }

# javac is driven via an @argfile. Windows PowerShell mangles certain argument
# combinations (a stray ':' gets passed through as a flag), and argfiles avoid
# quoting entirely. Paths inside must use forward slashes.
$argFile = Join-Path $work "javac.args"
$argLines = @(
    "-source", "8",
    "-target", "8",
    "-nowarn",
    "-bootclasspath", ($androidJar -replace '\\', '/'),
    "-d", ($classesDir -replace '\\', '/')
) + ($sources | ForEach-Object { '"' + ($_ -replace '\\', '/') + '"' })
Set-Content -Path $argFile -Value $argLines -Encoding ASCII

$javacLog = Join-Path $logDir "javac.log"
$prev = $ErrorActionPreference
$ErrorActionPreference = "Continue"
try {
    & $javac "@$argFile" > $javacLog 2>&1
    $javacExit = $LASTEXITCODE
} finally {
    $ErrorActionPreference = $prev
}
if ($javacExit -ne 0) {
    Get-Content $javacLog | ForEach-Object { Write-Host "  $_" }
    throw "javac failed (exit $javacExit)"
}
Write-Host "  javac ok"

$classesJar = Join-Path $work "classes.jar"
Push-Location $classesDir
try {
    Invoke-NativeTool -Exe $jar -Arguments @("cf", $classesJar, ".") `
        -LogPath (Join-Path $logDir "jar.log") -Label "jar" | Out-Null
} finally {
    Pop-Location
}

$dexDir = Join-Path $work "dex"
Invoke-NativeTool -Exe $d8 `
    -Arguments @("--lib", $androidJar, "--min-api", "26", "--output", $dexDir, $classesJar) `
    -LogPath (Join-Path $logDir "d8.log") -Label "d8" | Out-Null
$dex = Join-Path $dexDir "classes.dex"
if (-not (Test-Path $dex)) { throw "classes.dex was not produced" }
Write-Host "  d8 ok"

# ---- 5. Assemble APK -------------------------------------------------
Write-Host ""
Write-Host "=== [5/6] assembling APK ==="
$unsigned = Join-Path $work "unsigned.apk"
Copy-Item $baseApk $unsigned -Force

Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem

$zip = [System.IO.Compression.ZipFile]::Open($unsigned, [System.IO.Compression.ZipArchiveMode]::Update)
try {
    [System.IO.Compression.ZipFileExtensions]::CreateEntryFromFile(
        $zip, $dex, "classes.dex",
        [System.IO.Compression.CompressionLevel]::Optimal) | Out-Null
    Write-Host "  added classes.dex"

    foreach ($abi in $abis) {
        $so = Join-Path $work "apk\lib\$abi\libgl_gles.so"
        $entryName = "lib/$abi/libgl_gles.so"
        # Store uncompressed: the loader maps .so files directly, and
        # extractNativeLibs + alignment require STORED (or page-aligned) entries.
        [System.IO.Compression.ZipFileExtensions]::CreateEntryFromFile(
            $zip, $so, $entryName,
            [System.IO.Compression.CompressionLevel]::NoCompression) | Out-Null
        Write-Host "  added $entryName"
    }
} finally {
    $zip.Dispose()
}

# ---- 6. Align + sign -------------------------------------------------
Write-Host ""
Write-Host "=== [6/6] zipalign + apksigner ==="
$aligned = Join-Path $work "aligned.apk"
Invoke-NativeTool -Exe $zipalign -Arguments @("-f", "-p", "4", $unsigned, $aligned) `
    -LogPath (Join-Path $logDir "zipalign.log") -Label "zipalign" | Out-Null
Write-Host "  aligned"

# A self-signed debug key is appropriate: this APK is side-loaded by the user,
# not distributed through a store.
$ks = Join-Path $pluginDir "debug.keystore"
if (-not (Test-Path $ks)) {
    Write-Host "  generating debug keystore"
    Invoke-NativeTool -Exe $keytool `
        -Arguments @(
            "-genkeypair",
            "-keystore", $ks,
            "-storepass", "android",
            "-keypass", "android",
            "-alias", "glesmod",
            "-keyalg", "RSA",
            "-keysize", "2048",
            "-validity", "10000",
            "-dname", "CN=GLES Mod Renderer, OU=dev, O=glesmod, L=dev, S=dev, C=CN"
        ) `
        -LogPath (Join-Path $logDir "keytool.log") -Label "keytool genkeypair" | Out-Null
    if (-not (Test-Path $ks)) { throw "keytool reported success but no keystore was created" }
    Write-Host "  keystore created"
} else {
    Write-Host "  reusing existing debug keystore"
}

$outApk = Join-Path $outDir "glesmod-renderer-plugin.apk"
if (Test-Path $outApk) { Remove-Item $outApk -Force }

Invoke-NativeTool -Exe $apksigner `
    -Arguments @(
        "sign",
        "--ks", $ks,
        "--ks-pass", "pass:android",
        "--key-pass", "pass:android",
        "--ks-key-alias", "glesmod",
        "--out", $outApk,
        $aligned
    ) `
    -LogPath (Join-Path $logDir "apksigner.log") -Label "apksigner sign" | Out-Null
Write-Host "  signed"

Invoke-NativeTool -Exe $apksigner `
    -Arguments @("verify", "--verbose", $outApk) `
    -LogPath (Join-Path $logDir "apksigner-verify.log") -Label "apksigner verify" | Out-Null
Get-Content (Join-Path $logDir "apksigner-verify.log") |
    Select-Object -First 6 | ForEach-Object { Write-Host "  $_" }

# ---- 7. Verify the manifest is loadable by FCL -----------------------
#
# This check exists because FCL fails SILENTLY: RendererPlugin.parseV1() does
#     metaData.getString("boatEnv") ?: return
# for each of four fields, so a single missing field makes the plugin vanish
# from the renderer list with no error anywhere. That is exactly what happened
# once already, so the build now refuses to declare success without checking.
Write-Host ""
Write-Host "=== [7/7] verifying FCL plugin metadata ==="

$aapt2DumpLog = Join-Path $logDir "manifest-dump.txt"
Invoke-NativeTool -Exe $aapt2 `
    -Arguments @("dump", "xmltree", "--file", "AndroidManifest.xml", $outApk) `
    -LogPath $aapt2DumpLog -Label "aapt2 dump" | Out-Null

$manifestText = Get-Content $aapt2DumpLog -Raw

# parseV1() requires all four; missing any one => plugin silently ignored.
$requiredFields = @("renderer", "des", "boatEnv", "pojavEnv")
# PluginManager.scan() gate.
$pluginFlag = "fclPlugin"

$missing = @()
foreach ($f in $requiredFields) {
    if ($manifestText -notmatch [regex]::Escape("`"$f`"") ) {
        $missing += $f
    }
}
if ($manifestText -notmatch [regex]::Escape("`"$pluginFlag`"")) {
    $missing += $pluginFlag
}

if ($missing.Count -gt 0) {
    Write-Host ""
    Write-Host "FAILED: manifest is missing metadata fields that FCL requires:" -ForegroundColor Red
    foreach ($m in $missing) { Write-Host "  - $m" -ForegroundColor Red }
    Write-Host ""
    Write-Host "FCL would ignore this plugin with NO error message. See the comment"
    Write-Host "block at the top of fcl-plugin/AndroidManifest.xml for the source"
    Write-Host "references in FCL that define these requirements."
    throw "plugin metadata incomplete: $($missing -join ', ')"
}

# Report the values that FCL will actually read.
Write-Host "  required fields present: $($requiredFields -join ', '), $pluginFlag"
foreach ($f in @("renderer", "des", "pojavEnv", "minMCVer", "maxMCVer")) {
    $m = [regex]::Match($manifestText, "name\(0x01010003\)=`"$f`"$([char]13)?""`n[^\n]*value[^=]*=\([^)]*\)`"([^`"]*)`"")
    if (-not $m.Success) {
        $m = [regex]::Match($manifestText, "`"$f`"\s*\r?\n\s*A:[^\n]*value[^=]*=[^`"]*`"([^`"]*)`"")
    }
    if ($m.Success) {
        Write-Host ("  {0,-10} = {1}" -f $f, $m.Groups[1].Value)
    } else {
        Write-Host ("  {0,-10} = (present)" -f $f)
    }
}

# LIBGL_ES must be injected, otherwise FCL creates an ES 2 context (O-09).
if ($manifestText -notmatch "LIBGL_ES=3") {
    throw "pojavEnv does not set LIBGL_ES=3 -- the EGL context would fall back to ES 2 (see docs/architecture.md 2.5)"
}
Write-Host "  LIBGL_ES=3 is injected (required, see O-09)"

# POJAV_RENDERER must be injected. FCL only sets it for v2 plugins; a v1 plugin
# that omits it makes FCL's pojavInitOpenGL() pass NULL to strncmp, which
# SIGSEGVs during GLFW.glfwInit() before Minecraft even starts.
# Verified against a real device crash log on 2026-09-25.
if ($manifestText -notmatch "POJAV_RENDERER=opengles") {
    throw "pojavEnv does not set POJAV_RENDERER=opengles* -- FCL will crash with SIGSEGV in strncmp during glfwInit"
}
Write-Host "  POJAV_RENDERER is injected (required; missing causes SIGSEGV)"

# v2 metadata: new FCL prefers fclPlugin_V2; v1 fields above are kept so old
# FCL versions still discover the plugin. The JSON itself is validated by
# native/tools/check_plugin_v2.py (run in CI and run_all_gates.py).
$hasV2 = $manifestText -match [regex]::Escape('"fclPlugin_V2"')
if ($hasV2) {
    if ($manifestText -notmatch [regex]::Escape('"gles_driver_angle"')) {
        throw "v2 plugin is missing gles_driver_angle title metadata"
    }
    Write-Host "  v2 metadata present: fclPlugin_V2, gles_driver_angle"
} else {
    Write-Host "  v2 metadata absent (v1-only build)"
}

# The native libraries must be present under lib/<abi>/ or dlopen fails.
foreach ($abi in $abis) {
    $entry = "lib/$abi/libgl_gles.so"
    $zipCheck = [System.IO.Compression.ZipFile]::OpenRead($outApk)
    try {
        $found = $zipCheck.Entries | Where-Object { $_.FullName -eq $entry }
        if (-not $found) { throw "APK is missing $entry -- FCL cannot dlopen it" }
        Write-Host ("  {0} present ({1} bytes)" -f $entry, $found.Length)
    } finally {
        $zipCheck.Dispose()
    }
}

$kb = [math]::Round((Get-Item $outApk).Length / 1KB, 1)
Write-Host ""
Write-Host "=============================================="
Write-Host (" plugin APK: {0}" -f $outApk)
Write-Host (" size      : {0} KB" -f $kb)
Write-Host " all FCL metadata requirements verified"
Write-Host "=============================================="
