#!/bin/zsh

# Assign command line arguments to named variables
TargetPlatform=$1
PluginDir=$2

# Check if PluginDir is empty or null
if [[ -z $PluginDir ]]; then
    echo "Error: PluginDir is empty or not provided"
    exit 1
fi

if [[ -z $TargetPlatform ]]; then
    echo "Error: TargetPlatform is empty or not provided"
    exit 1
fi

if [[ $TargetPlatform == "Android" && -z "$NDKROOT" ]]; then
    echo "Error: NDKROOT environment variable is not set, but it's required for Android builds"
    exit 1
fi

if [[ $TargetPlatform == "Android" && -z "$ANDROID_HOME" ]]; then
    echo "Error: ANDROID_HOME environment variable is not set, but it's required for Android builds"
    exit 1
fi

# Define directories
SourceDir="$PluginDir/Source/ThirdParty/ZXing/zxing-cpp"
BuildDir="$PluginDir/Intermediate/ThirdParty/ZXing/$TargetPlatform"
BinariesDir="$PluginDir/Binaries/$TargetPlatform"

# This stuff is useful to see in the build logs
echo "Building ZXing for Win64..."
echo "Source Directory: $SourceDir"
echo "Build Directory: $BuildDir"
echo "Binaries Directory: $BuildDir"

args=(
    "-S" "$SourceDir"
    "-B" "$BuildDir"
    "-DCMAKE_BUILD_TYPE=Release"
    "-DBUILD_READERS=ON"
    "-DBUILD_WRITERS=OFF"
    "-DBUILD_UNIT_TESTS=OFF"
    "-DBUILD_BLACKBOX_TESTS=OFF"
    "-DBUILD_EXAMPLES=OFF"
)

# Check the target platform
case $TargetPlatform in
    "Mac")
        if [[ -f "$BinariesDir/libZXing.dylib" && -f "$BuildDir/core/libZXing.a" ]]; then
          echo "Both libZXing.dylib and libZXing.a already exist. Exiting early with success."
          exit 0
        fi
        
        args+=(
          "-DCMAKE_OSX_DEPLOYMENT_TARGET=11.0"
        )

        cmake=$(which cmake)

        echo "$cmake ${args[@]} -DBUILD_SHARED_LIBS=OFF"
        "$cmake" "${args[@]}" -DBUILD_SHARED_LIBS=OFF
        echo "$cmake --build \"$BuildDir\" -j8 --config Release"
        "$cmake" --build "$BuildDir" -j8 --config Release

        echo "$cmake ${args[@]} -DBUILD_SHARED_LIBS=ON"
        "$cmake" "${args[@]}" -DBUILD_SHARED_LIBS=ON
        echo "$cmake --build \"$BuildDir\" -j8 --config Release"
        "$cmake" --build "$BuildDir" -j8 --config Release
        
        ArtifactPath="$BuildDir/core/libZXing.dylib"
        if [[ ! -f "$ArtifactPath" ]]; then
            echo "Error: File $ArtifactPath does not exist."
            exit 1
        fi
        mkdir -p "$BinariesDir"
        cp "$ArtifactPath" "$BinariesDir/libZXing.dylib"
        ;;

    "Android")
        if [[ -f "$BinariesDir/libZXing.so" && -f "$BuildDir/core/libZXing.a" ]]; then
          echo "Both libZXing.so and libZXing.a already exist. Exiting early with success."
          exit 0
        fi

        echo "Building ZXing for Android..."
        echo "NDKROOT: $NDKROOT"
        echo "ANDROID_HOME: $ANDROID_HOME"
        
        # There can be multiple cmake versions in the SDK path, let's find the most recent one
        cmake=$(find $ANDROID_HOME/cmake -name "cmake" | sort -r | head -1)
        binPath=$(dirname "$cmake")
        ninja="$binPath/ninja"
        echo "cmake: $cmake"
        echo "ninja: $ninja"
        
        # Prepend Android specific build arguments
        android_args=(
            "-G" "Ninja Multi-Config"
            "-DCMAKE_SYSTEM_NAME=Android"
            "-DCMAKE_SYSTEM_VERSION=32"
            "-DCMAKE_ANDROID_ARCH_ABI=arm64-v8a"
            "-DCMAKE_TOOLCHAIN_FILE='$NDKROOT/build/cmake/android.toolchain.cmake'"
            "-DCMAKE_MAKE_PROGRAM='$ninja'"
            "-DANDROID_PLATFORM=android-32"
            "-DANDROID_CPP_FEATURES='exceptions'"
            "-DANDROID_ARM_NEON=TRUE"
            "-DANDROID_ABI=arm64-v8a"
        )
        args=("${android_args[@]}" "${args[@]}")

        echo "$cmake ${args[@]} -DBUILD_SHARED_LIBS=OFF"
        "$cmake" "${args[@]}" -DBUILD_SHARED_LIBS=OFF
        echo "$cmake $BuildDir -j8 --config Release"
        "$cmake" --build "$BuildDir" -j8 --config Release

        echo "$cmake ${args[@]} -DBUILD_SHARED_LIBS=ON"
        "$cmake" "${args[@]}" -DBUILD_SHARED_LIBS=ON
        echo "$cmake $BuildDir -j8 --config Release"
        "$cmake" --build "$BuildDir" -j8 --config Release

        ArtifactPath="$BuildDir/core/Release/libZXing.so"
        if [[ ! -f "$ArtifactPath" ]]; then
            echo "Error: File $ArtifactPath does not exist."
            exit 1
        fi
        mkdir -p "$BinariesDir"
        cp "$ArtifactPath" "$BinariesDir/libZXing.so"
        ;;

    *)
        echo "Target platform not supported: $TargetPlatform"
        exit 1
        ;;
esac
