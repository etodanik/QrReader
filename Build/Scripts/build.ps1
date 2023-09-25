param (
    [string]$TargetPlatform,
    [string]$PluginDir
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

# Check if PluginDir is empty or null
if (-not $PluginDir) {
    Write-Host "Error: PluginDir is empty or not provided"
    exit 1
}

if (-not $TargetPlatform) {
    Write-Host "Error: TargetPlatform is empty or not provided"
    exit 1
}

if ($TargetPlatform -eq "Android" -and (-not $env:NDKROOT)) {
    Write-Host "Error: NDKROOT environment variable is not set, but it's required for Android builds"
    exit 1
}

if ($TargetPlatform -eq "Android" -and (-not $env:ANDROID_HOME)) {
    Write-Host "Error: ANDROID_HOME environment variable is not set, but it's required for Android builds"
    exit 1
}

# Define directories
$SourceDir = "$PluginDir\Source\ThirdParty\ZXing\zxing-cpp"
$BuildDir = "$PluginDir\Intermediate\ThirdParty\ZXing\$TargetPlatform"
$BinariesDir = "$PluginDir\Binaries\$TargetPlatform"

# This stuff is useful to see in the build logs
Write-Host "Building ZXing for Win64..."
Write-Host "Source Directory: $SourceDir"
Write-Host "Build Directory: $BuildDir"
Write-Host "Binaries Directory: $BuildDir"

$args = @(
    "-S", "$SourceDir",
    "-B", "$BuildDir",
    "-DCMAKE_BUILD_TYPE=Release",
    "-DBUILD_READERS=ON",
    "-DBUILD_WRITERS=OFF",
    "-DBUILD_UNIT_TESTS=OFF",
    "-DBUILD_BLACKBOX_TESTS=OFF",
    "-DBUILD_EXAMPLES=OFF"
)

# Check the target platform
switch ($TargetPlatform) {
    "Win64" {
        # Exit early if the files are already built
        if ((Test-Path "$BinariesDir\ZXing.dll") -and (Test-Path "$BuildDir\core\Release\ZXing.lib")) {
            Write-Host "Both ZXing.dll and ZXing.lib already exist. Exiting early with success."
            exit 0
        }

        $args = $args + @(
            "-DBUILD_SHARED_LIBS=ON"
        );
        
        & cmake.exe @args
        & cmake.exe --build "$BuildDir" -j8 --config Release
        $ArtifactPath = "$BuildDir\core\Release\ZXing.dll"
        if (-not (Test-Path $ArtifactPath)) {
            Write-Host "Error: File $ArtifactPath does not exist."
            exit 1
        }
        New-Item -Path "$BinariesDir" -ItemType Directory -Force
        Copy-Item "$ArtifactPath" "$BinariesDir\ZXing.dll"
    }
    
    "Android" {
        # Exit early if the files are already built
        if ((Test-Path "$BinariesDir\libZXing.so") -and (Test-Path "$BuildDir\core\Release\libZXing.a")) {
            Write-Host "Both libZXing.so and libZXing.a already exist. Exiting early with success."
            exit 0
        }

        Write-Host "Building ZXing for Android..."
        Write-Host "NDKROOT: $env:NDKROOT"
        Write-Host "ANDROID_HOME: $env:ANDROID_HOME"
        
        # There can be multiple cmake versions in the SDK path, let's find the most recent one
        $cmake = (Get-ChildItem -Path $env:ANDROID_HOME/cmake -Recurse -File cmake.exe | Sort-Object LastWriteTime -Descending | Select-Object -First 1 -ExpandProperty FullName)
        $binPath = Split-Path $cmake
        # Let's reuse the path to make things faster
        $ninja = Join-Path $binPath "ninja.exe"
        Write-Host "cmake.exe: $cmake"
        Write-Host "ninja.exe: $ninja"
        
        # Prepend Android specific build arguments
        $args = @(
            "-G", "Ninja Multi-Config",
            # "-A", "arm64",
            "-DCMAKE_SYSTEM_NAME=Android",
            "-DCMAKE_SYSTEM_VERSION=32",
            "-DCMAKE_ANDROID_ARCH_ABI=arm64-v8a",
            "-DCMAKE_TOOLCHAIN_FILE='$env:NDKROOT/build/cmake/android.toolchain.cmake'",            
            "-DCMAKE_MAKE_PROGRAM='$ninja'",
            "-DANDROID_PLATFORM=android-32",
            "-DANDROID_CPP_FEATURES='exceptions'",
            "-DANDROID_ARM_NEON=TRUE",
            "-DANDROID_ABI=arm64-v8a"
        ) + $args;

        Write-Host "$cmake " @args " -DBUILD_SHARED_LIBS=OFF"
        & $cmake @args -DBUILD_SHARED_LIBS=OFF
        Write-Host "$cmake $BuildDir -j8 --config Release"
        & $cmake --build "$BuildDir" -j8 --config Release

        Write-Host "$cmake " @args "  -DBUILD_SHARED_LIBS=ON"
        & $cmake @args -DBUILD_SHARED_LIBS=ON
        Write-Host "$cmake $BuildDir -j8 --config Release"
        & $cmake --build "$BuildDir" -j8 --config Release

        $ArtifactPath = "$BuildDir\core\Release\libZXing.so"
        if (-not (Test-Path $ArtifactPath)) {
            Write-Host "Error: File $ArtifactPath does not exist."
            exit 1
        }
        New-Item -Path "$BinariesDir" -ItemType Directory -Force
        Copy-Item "$ArtifactPath" "$BinariesDir\libZXing.so"
    }

    default {
        Write-Host "Target platform not supported: $TargetPlatform"
        exit 1
    }
}