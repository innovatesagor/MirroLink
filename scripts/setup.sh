#!/bin/bash

# Exit on error
set -e

echo "Setting up MirroLink development environment..."

# Check if running on macOS
if [[ "$OSTYPE" != "darwin"* ]]; then
    echo "Error: This script is designed for macOS"
    exit 1
fi

# Check for Homebrew
if ! command -v brew &> /dev/null; then
    echo "Installing Homebrew..."
    /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
fi

# Install dependencies
echo "Installing dependencies..."
brew install \
    ffmpeg \
    libusb \
    sdl2 \
    sdl2_image \
    meson \
    ninja \
    pkg-config \
    googletest \
    jsoncpp

# Install Android Platform Tools if not present
if ! command -v adb &> /dev/null; then
    echo "Installing Android Platform Tools..."
    brew install --cask android-platform-tools
fi

# Create build directory
echo "Setting up build directory..."
if [ ! -d "build" ]; then
    meson setup build
else
    echo "Build directory already exists, reconfiguring..."
    meson setup --reconfigure build
fi

# Build the project
echo "Building MirroLink..."
ninja -C build

# Run tests
echo "Running tests..."
ninja -C build test

echo "Setup complete! You can now run MirroLink with './build/src/mirrolink'"