# MirroLink

MirroLink is an open-source Android screen mirroring and control tool for macOS. It enables high-performance, low-latency screen mirroring and full device control over USB-C, with no app installation required on the Android device.

## Features

- **Full-HD Display Mirroring**: 1080p+ resolution at 30-120 FPS with hardware acceleration
- **Low Latency**: 35-70ms latency for responsive control
- **Complete Remote Control**: Use your Mac's keyboard and mouse to control Android
- **No Root Required**: Works with standard Android USB debugging
- **Audio Forwarding**: Stream Android audio to Mac (Android 11+)
- **Recording & Screenshots**: Capture screen recordings and snapshots
- **Plug-and-Play**: Just connect USB-C and enable debugging

## Quick Start

### Prerequisites

- macOS 12.0 or later
- Android 5.0+ device with USB debugging enabled
- USB-C cable (or appropriate adapter)

### Installation

```bash
# Using Homebrew (Recommended)
brew tap innovatesagor/mirrolink
brew install mirrolink

# Or build from source
git clone https://github.com/innovatesagor/MirroLink.git
cd MirroLink
./scripts/setup.sh
```

### Basic Usage

1. Connect your Android device via USB-C
2. Enable USB debugging on your Android device
3. Launch MirroLink
4. Start controlling your device from your Mac

## Key Bindings

- **Mouse**: Click and drag to control touch input
- **Right-click**: Back button
- **Middle-click**: Home button
- **F11**: Toggle fullscreen
- **Ctrl+V**: Paste text from Mac clipboard
- **Volume keys**: Control Android volume

## Building from Source

### Dependencies

- Xcode Command Line Tools
- Homebrew
- FFmpeg
- SDL2
- libusb
- Meson & Ninja

### Build Steps

1. Clone the repository:
   ```bash
   git clone https://github.com/innovatesagor/MirroLink.git
   cd MirroLink
   ```

2. Run the setup script:
   ```bash
   ./scripts/setup.sh
   ```

3. Build the project:
   ```bash
   ninja -C build
   ```

4. Run tests:
   ```bash
   ninja -C build test
   ```

### Development

Check out our [Contributing Guide](docs/developer/CONTRIBUTING.md) for:
- Code style guidelines
- Build system details
- Testing procedures
- Pull request process

## Troubleshooting

See our [Installation Guide](docs/user/INSTALLATION.md) for detailed setup instructions and troubleshooting steps.

Common issues:
- Device not detected: Check USB debugging is enabled
- Poor performance: Try lowering resolution/framerate
- Input not working: Enable USB debugging (Security settings)

## License

MirroLink is licensed under the Apache License 2.0. See [LICENSE](LICENSE) for details.

## Contributing

We welcome contributions! Please see our [Contributing Guide](docs/developer/CONTRIBUTING.md) for details on:
- Setting up the development environment
- Code style guidelines
- Pull request process
- Testing requirements

## Acknowledgments

MirroLink builds upon several open-source projects:
- [scrcpy](https://github.com/Genymobile/scrcpy)
- FFmpeg
- SDL2
- libusb

## Support

- GitHub Issues: Bug reports and feature requests
- [Discord Community](https://discord.gg/mirrolink)
- [Documentation](docs/)

## Security

For security issues, please email security@mirrolink.dev instead of using the public issue tracker.