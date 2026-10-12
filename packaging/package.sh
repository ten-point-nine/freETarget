#!/usr/bin/env bash
#
# Builds the downloadable packages for one platform: the freETarget desktop application
# and the target simulator.
#
#   packaging/package.sh <runtime> [app|simulator]
#
# runtime is one of: win-x64 win-arm64 linux-x64 linux-arm64 osx-x64 osx-arm64
# Without a second argument both packages are built. The result is written to dist/ :
#   Windows  freETarget-<version>-<runtime>.zip      a folder with freETarget.Desktop.exe
#   Linux    freETarget-<version>-<runtime>.tar.gz   a folder with the executable freETarget.Desktop
#   macOS    freETarget-<version>-<runtime>.zip      freETarget.app
#   all      freETarget-simulator-<version>-<runtime>.zip (.tar.gz on Linux)
#            a folder with the single program TargetSimulator.Tcp
#
# The version is the <Version> of the project file of the application. The environment variable
# FET_VERSION replaces it (same major number only), that is how a release gets the version of its tag.
#
# The packages contain the .NET runtime, nothing has to be installed to run them.
# Any platform can be built on any platform, with one exception: the macOS application is only
# complete (icon, ad-hoc signature) when it is built on macOS.

set -euo pipefail

RID="${1:-}"
case "$RID" in
    win-x64|win-arm64|linux-x64|linux-arm64|osx-x64|osx-arm64) ;;
    *) echo "usage: $0 <win-x64|win-arm64|linux-x64|linux-arm64|osx-x64|osx-arm64> [app|simulator]" >&2; exit 1 ;;
esac

WHAT="${2:-all}"
case "$WHAT" in
    all|app|simulator) ;;
    *) echo "the second argument is 'app' or 'simulator'" >&2; exit 1 ;;
esac

cd "$(dirname "$0")/.."

PROJECT="freETarget.Desktop/freETarget.Desktop.csproj"
# the version of the project file
VERSION="$(sed -n 's/.*<Version>\(.*\)<\/Version>.*/\1/p' "$PROJECT" | head -1)"
if [ -z "$VERSION" ]; then
    echo "no <Version> found in $PROJECT" >&2
    exit 1
fi

# a release gives its own version in FET_VERSION (the build workflow takes it from the tag pc-v<version>).
# the major number is the version of the database the application needs, a different one would make
# the application ask every user to overwrite their database. it can only be changed in the project file
if [ -n "${FET_VERSION:-}" ]; then
    if ! [[ "$FET_VERSION" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
        echo "FET_VERSION '$FET_VERSION' is not a version like 4.12.2" >&2
        exit 1
    fi
    if [ "${FET_VERSION%%.*}" != "${VERSION%%.*}" ]; then
        echo "FET_VERSION '$FET_VERSION' has another major number than the <Version> $VERSION in $PROJECT." >&2
        echo "the major number is the version of the database, change it in the project file if that is intended" >&2
        exit 1
    fi
    VERSION="$FET_VERSION"
fi

DIST="$PWD/dist"
STAGE="$DIST/stage/$RID"
rm -rf "$STAGE"
mkdir -p "$STAGE"

# zip a folder of the stage directory. the runners of the build service do not all have the same tools
make_zip() {
    local folder="$1" target="$2"
    if command -v zip > /dev/null; then
        (cd "$STAGE" && zip -q -r -y "$target" "$folder")
    elif command -v 7z > /dev/null; then
        (cd "$STAGE" && 7z a -bso0 -bsp0 "$target" "$folder")
    else
        (cd "$STAGE" && powershell -NoProfile -Command "Compress-Archive -Path '$folder' -DestinationPath '$(cygpath -w "$target")'")
    fi
}


package_app() {
    local NAME="freETarget-$VERSION-$RID"
    rm -f "$DIST/$NAME.zip" "$DIST/$NAME.tar.gz"

    echo "== publishing $NAME"
    dotnet publish "$PROJECT" --configuration Release --runtime "$RID" --self-contained true \
        -p:Version="$VERSION" -p:DebugType=none -p:DebugSymbols=false --output "$STAGE/publish" --nologo

    # debug files of native libraries are not needed to run the application
    find "$STAGE/publish" -name '*.pdb' -delete

    case "$RID" in
        win-*)
            mv "$STAGE/publish" "$STAGE/freETarget"
            make_zip freETarget "$DIST/$NAME.zip"
            ;;

        linux-*)
            mv "$STAGE/publish" "$STAGE/freETarget"
            chmod +x "$STAGE/freETarget/freETarget.Desktop"
            tar -czf "$DIST/$NAME.tar.gz" -C "$STAGE" freETarget
            ;;

        osx-*)
            APP="$STAGE/freETarget.app"
            mkdir -p "$APP/Contents/Resources"
            mv "$STAGE/publish" "$APP/Contents/MacOS"
            chmod +x "$APP/Contents/MacOS/freETarget.Desktop"
            sed "s/@VERSION@/$VERSION/g" packaging/macos/Info.plist > "$APP/Contents/Info.plist"

            if [ "$(uname)" = "Darwin" ]; then
                # icon in the sizes macOS wants, from the icon of the application
                ICONSET="$STAGE/freETarget.iconset"
                mkdir -p "$ICONSET"
                sips -s format png freETarget.Desktop/Assets/app.ico --out "$STAGE/icon.png" > /dev/null
                for size in 16 32 128 256 512; do
                    sips -z $size $size "$STAGE/icon.png" --out "$ICONSET/icon_${size}x${size}.png" > /dev/null
                    sips -z $((size * 2)) $((size * 2)) "$STAGE/icon.png" --out "$ICONSET/icon_${size}x${size}@2x.png" > /dev/null
                done
                iconutil -c icns "$ICONSET" -o "$APP/Contents/Resources/freETarget.icns"

                # ad-hoc signature: no certificate and no Apple account involved. without any signature
                # macOS refuses to start programs for Apple Silicon at all
                codesign --force --deep --sign - "$APP"

                # ditto keeps the signature and the attributes of the bundle intact
                ditto -c -k --keepParent "$APP" "$DIST/$NAME.zip"
            else
                echo "warning: not on macOS. the application has no icon and no signature and will not start on Apple Silicon" >&2
                make_zip freETarget.app "$DIST/$NAME.zip"
            fi
            ;;
    esac


    echo "== done: $(ls "$DIST/$NAME".*)"
}

# the simulator is one program without any other files. it pretends to be a target with a network connection
package_simulator() {
    local NAME="freETarget-simulator-$VERSION-$RID"
    local FOLDER="freETarget-simulator"
    rm -f "$DIST/$NAME.zip" "$DIST/$NAME.tar.gz"

    echo "== publishing $NAME"
    dotnet publish TargetSimulator.Tcp/TargetSimulator.Tcp.csproj --configuration Release --runtime "$RID" --self-contained true \
        -p:PublishSingleFile=true -p:PublishTrimmed=true -p:InvariantGlobalization=true \
        -p:Version="$VERSION" -p:DebugType=none -p:DebugSymbols=false --output "$STAGE/$FOLDER" --nologo
    find "$STAGE/$FOLDER" -name '*.pdb' -delete

    case "$RID" in
        win-*)
            make_zip "$FOLDER" "$DIST/$NAME.zip"
            ;;
        linux-*)
            chmod +x "$STAGE/$FOLDER/TargetSimulator.Tcp"
            tar -czf "$DIST/$NAME.tar.gz" -C "$STAGE" "$FOLDER"
            ;;
        osx-*)
            chmod +x "$STAGE/$FOLDER/TargetSimulator.Tcp"
            if [ "$(uname)" = "Darwin" ]; then
                # ad-hoc signature, see the application above
                codesign --force --sign - "$STAGE/$FOLDER/TargetSimulator.Tcp"
                ditto -c -k --keepParent "$STAGE/$FOLDER" "$DIST/$NAME.zip"
            else
                echo "warning: not on macOS. the simulator has no signature and will not start on Apple Silicon" >&2
                make_zip "$FOLDER" "$DIST/$NAME.zip"
            fi
            ;;
    esac
    rm -rf "$STAGE/$FOLDER"
    echo "== done: $(ls "$DIST/$NAME".*)"
}

if [ "$WHAT" = "all" ] || [ "$WHAT" = "app" ]; then
    package_app
fi
if [ "$WHAT" = "all" ] || [ "$WHAT" = "simulator" ]; then
    package_simulator
fi

rm -rf "$STAGE"
rmdir "$DIST/stage" 2> /dev/null || true
