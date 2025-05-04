# MirroLink Installation Guide

## System Requirements
- macOS 12.0 or later
- Android device running Android 5.0 (API 21) or newer
- USB-C cable (or appropriate adapter)
- At least 2GB of free RAM
- 100MB free disk space

## Installation Steps

### 1. Using Homebrew (Recommended)
```bash
# Install Android Platform Tools first
brew install --cask android-platform-tools

# Install MirroLink
brew install mirrolink
```

### 2. Manual Installation
1. Download the latest release from [GitHub Releases](https://github.com/innovatesagor/MirroLink/releases)
2. Move the MirroLink.app to your Applications folder
3. Install Android Platform Tools manually if needed

## Android Device Setup

1. Enable Developer Options:
   - Go to Settings > About phone
   - Find "Build number"
   - Tap it 7 times until you see "You are now a developer!"

2. Enable USB Debugging:
   - Go to Settings > System > Developer options
   - Enable "USB debugging"
   - Optional: Enable "USB debugging (Security settings)" if needed for input control

3. Connect Your Device:
   - Use a high-quality USB-C cable
   - When prompted on your Android device, check "Always allow from this computer"
   - Accept the USB debugging authorization prompt

## Troubleshooting

### Common Issues

1. Device Not Detected
   - Ensure USB debugging is enabled
   - Try a different USB cable
   - Restart both devices
   - Run `adb devices` in Terminal to verify connection

2. Performance Issues
   - Lower the resolution: `mirrolink --max-res 1280x720`
   - Reduce frame rate: `mirrolink --max-fps 30`
   - Check CPU usage and available RAM

3. Input Not Working
   - Enable "USB debugging (Security settings)"
   - Try reconnecting the device
   - Check keyboard layout settings

### Getting Help
- Visit our [GitHub Issues](https://github.com/innovatesagor/MirroLink/issues)
- Join our [Discord community](https://discord.gg/mirrolink)
- Check the [FAQ section](FAQ.md)

## Uninstallation
```bash
# If installed via Homebrew
brew uninstall mirrolink

# If installed manually
rm -rf /Applications/MirroLink.app
```