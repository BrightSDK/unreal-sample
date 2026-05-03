#!/usr/bin/env bash
# install_bright_sdk.sh — Download and extract BrightSDK binaries into the
# plugin's ThirdParty directory.  Run this once after cloning and whenever you
# want to update to the latest (or a specific) SDK version.
#
# Usage:
#   ./install_bright_sdk.sh                  # latest versions for all platforms
#   ./install_bright_sdk.sh --ios-ver 1.617.813 --win-ver 1.617.770
#   ./install_bright_sdk.sh --platforms ios  # iOS only
#   ./install_bright_sdk.sh --platforms win  # Win64 only
#
# Environment overrides (alternative to flags):
#   BRIGHT_SDK_IOS_VER=1.617.813 ./install_bright_sdk.sh
#   BRIGHT_SDK_WIN_VER=1.617.770 ./install_bright_sdk.sh

set -euo pipefail

VERSIONS_URL="https://bright-sdk.com/sdk_api/sdk/versions"
CDN_BASE="https://cdn.bright-sdk.com/static"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PLUGIN_DIR="$SCRIPT_DIR/BrightSDK"
THIRD_PARTY_DIR="$PLUGIN_DIR/ThirdParty"
CACHE_DIR="$SCRIPT_DIR/.sdk_cache"

# ── defaults ──────────────────────────────────────────────────────────────────
IOS_VER="${BRIGHT_SDK_IOS_VER:-}"
WIN_VER="${BRIGHT_SDK_WIN_VER:-}"
PLATFORMS="ios win"   # both by default

# ── argument parsing ──────────────────────────────────────────────────────────
while [[ $# -gt 0 ]]; do
    case "$1" in
        --ios-ver)   IOS_VER="$2";    shift 2 ;;
        --win-ver)   WIN_VER="$2";    shift 2 ;;
        --platforms) PLATFORMS="$2";  shift 2 ;;
        *) echo "Unknown argument: $1"; exit 1 ;;
    esac
done

# ── helper functions ──────────────────────────────────────────────────────────
fetch_latest_version() {
    local platform="$1"
    curl -fsSL "$VERSIONS_URL" | grep -o "\"${platform}\":\"[^\"]*\"" | cut -d'"' -f4
}

download_and_cache() {
    local url="$1"
    local dest="$2"
    if [[ -f "$dest" ]]; then
        echo "  [cache] $dest"
    else
        echo "  [download] $url"
        mkdir -p "$(dirname "$dest")"
        curl -fsSL --progress-bar "$url" -o "$dest"
    fi
}

# ── iOS xcframework ───────────────────────────────────────────────────────────
install_ios() {
    local ver="$1"
    local zip_name="bright_sdk_ios-${ver}.zip"
    local zip_path="$CACHE_DIR/ios/${zip_name}"
    local url="${CDN_BASE}/${zip_name}"
    local target_dir="$THIRD_PARTY_DIR/iOS"

    echo "→ iOS SDK ${ver}"
    download_and_cache "$url" "$zip_path"

    rm -rf "$target_dir"
    mkdir -p "$target_dir"

    # The zip contains brdsdk.xcframework at the top level or one level deep
    local tmp_dir
    tmp_dir=$(mktemp -d)
    unzip -q "$zip_path" -d "$tmp_dir"
    local found_xcfw
    found_xcfw=$(find "$tmp_dir" -maxdepth 3 -name "brdsdk.xcframework" -type d | head -1)
    if [[ -z "$found_xcfw" ]]; then
        echo "  ERROR: brdsdk.xcframework not found inside $zip_name" >&2
        rm -rf "$tmp_dir"
        return 1
    fi
    cp -r "$found_xcfw" "$target_dir/brdsdk.xcframework"
    rm -rf "$tmp_dir"
    echo "  ✔ Extracted brdsdk.xcframework → BrightSDK/ThirdParty/iOS/"
}

# ── Win64 DLL + exe ───────────────────────────────────────────────────────────
install_win() {
    local ver="$1"
    local zip_name="bright_sdk_win-${ver}.zip"
    local zip_path="$CACHE_DIR/win/${zip_name}"
    local url="${CDN_BASE}/${zip_name}"
    local target_dir="$THIRD_PARTY_DIR/Win64"

    echo "→ Win64 SDK ${ver}"
    download_and_cache "$url" "$zip_path"

    rm -rf "$target_dir"
    mkdir -p "$target_dir"

    local tmp_dir
    tmp_dir=$(mktemp -d)
    unzip -q "$zip_path" -d "$tmp_dir"

    # Copy lum_sdk64.dll, lum_sdk64.lib (if present), net_updater64.exe
    local found=0
    for fname in lum_sdk64.dll lum_sdk64.lib net_updater64.exe; do
        local src
        src=$(find "$tmp_dir" -name "$fname" | head -1)
        if [[ -n "$src" ]]; then
            cp "$src" "$target_dir/$fname"
            echo "  ✔ $fname"
            found=1
        fi
    done

    rm -rf "$tmp_dir"

    if [[ $found -eq 0 ]]; then
        echo "  ERROR: no expected files found inside $zip_name" >&2
        return 1
    fi
}

# ── resolve versions ──────────────────────────────────────────────────────────
if [[ "$PLATFORMS" == *ios* ]] && [[ -z "$IOS_VER" ]]; then
    echo "Fetching latest iOS SDK version..."
    IOS_VER=$(fetch_latest_version ios)
    echo "  Latest iOS: $IOS_VER"
fi

if [[ "$PLATFORMS" == *win* ]] && [[ -z "$WIN_VER" ]]; then
    echo "Fetching latest Win SDK version..."
    WIN_VER=$(fetch_latest_version win)
    echo "  Latest Win: $WIN_VER"
fi

# ── install ───────────────────────────────────────────────────────────────────
mkdir -p "$CACHE_DIR"

[[ "$PLATFORMS" == *ios* ]] && install_ios "$IOS_VER"
[[ "$PLATFORMS" == *win* ]] && install_win "$WIN_VER"

echo ""
echo "Done. BrightSDK/ThirdParty is ready:"
find "$THIRD_PARTY_DIR" -type f | sed 's|.*/BrightSDK/ThirdParty/|  ThirdParty/|'
