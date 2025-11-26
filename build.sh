#!/bin/bash

# build.sh - Helper script to build MoonAvoidance plugin on Linux/macOS

set -e

# Default values
QT_PATH=""
STEL_ROOT=""
BUILD_DIR="build"
CLEAN_BUILD=false
INTERACTIVE=true

# Help message
print_help() {
    echo "Usage: ./build.sh [options]"
    echo ""
    echo "Options:"
    echo "  --qt-path <path>      Path to Qt installation (e.g., ~/Qt/6.5.3/macos)"
    echo "  --stel-root <path>    Path to Stellarium source root"
    echo "  --clean               Clean build directory before building"
    echo "  --no-interactive      Don't prompt for input (fail if required args missing)"
    echo "  --help                Show this help message"
    echo ""
}

# Parse arguments
while [[ "$#" -gt 0 ]]; do
    case $1 in
        --qt-path) QT_PATH="$2"; shift ;;
        --stel-root) STEL_ROOT="$2"; shift ;;
        --clean) CLEAN_BUILD=true ;;
        --no-interactive) INTERACTIVE=false ;;
        --help) print_help; exit 0 ;;
        *) echo "Unknown parameter passed: $1"; print_help; exit 1 ;;
    esac
    shift
done

echo "=== MoonAvoidance Build Helper ==="

# Check for required tools
if ! command -v cmake &> /dev/null; then
    echo "Error: cmake is not installed or not in PATH."
    exit 1
fi

if ! command -v make &> /dev/null; then
    echo "Error: make is not installed or not in PATH."
    exit 1
fi

# Prompt for Qt path if not provided and interactive
if [ -z "$QT_PATH" ] && [ "$INTERACTIVE" = true ]; then
    echo ""
    echo "Please enter the path to your Qt installation (e.g., /Users/name/Qt/6.5.3/macos):"
    read -r QT_PATH
fi

# Validate Qt path
if [ -z "$QT_PATH" ]; then
    echo "Error: Qt path is required. Use --qt-path or run interactively."
    exit 1
fi

if [ ! -d "$QT_PATH" ]; then
    echo "Error: Directory '$QT_PATH' does not exist."
    exit 1
fi

# Prompt for Stellarium root if not provided and interactive
if [ -z "$STEL_ROOT" ] && [ "$INTERACTIVE" = true ]; then
    echo ""
    echo "Please enter the path to Stellarium source root (leave empty if using installed Stellarium):"
    read -r STEL_ROOT
fi

# Clean build directory if requested
if [ "$CLEAN_BUILD" = true ]; then
    if [ -d "$BUILD_DIR" ]; then
        echo "Cleaning build directory..."
        rm -rf "$BUILD_DIR"
    fi
fi

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Construct CMake command
CMAKE_CMD="cmake .."
CMAKE_CMD="$CMAKE_CMD -DCMAKE_PREFIX_PATH=\"$QT_PATH\""

if [ -n "$STEL_ROOT" ]; then
    CMAKE_CMD="$CMAKE_CMD -DSTELROOT=\"$STEL_ROOT\""
fi

echo ""
echo "Configuring with: $CMAKE_CMD"
eval $CMAKE_CMD

echo ""
echo "Building..."
make

echo ""
echo "Installing..."
make install

echo ""
echo "Build complete!"

