# Contributing to MirroLink

Thank you for your interest in contributing to MirroLink! This guide will help you get started with development.

## Development Setup

### Prerequisites
- Xcode 14.0 or later
- macOS 12.0 or later
- Android Platform Tools (ADB)
- Homebrew
- Git

### Required Dependencies
```bash
brew install ffmpeg libusb sdl2 meson ninja
brew install --cask android-platform-tools
```

### Building from Source
1. Clone the repository:
   ```bash
   git clone https://github.com/innovatesagor/MirroLink.git
   cd MirroLink
   ```

2. Install dependencies:
   ```bash
   ./scripts/setup.sh
   ```

3. Build the project:
   ```bash
   meson setup build
   ninja -C build
   ```

## Project Structure

```
MirroLink/
├── assets/            # Icons, images, and other static resources
├── client/
│   └── macos/         # macOS-specific client implementation
├── docs/
│   ├── developer/     # Developer documentation
│   └── user/          # End-user documentation
├── scripts/           # Build and development scripts
├── server/            # ADB connection and device management
├── src/
│   ├── core/          # Core functionality (screen mirroring, input handling)
│   ├── gui/           # GUI implementation
│   └── utils/         # Utility functions and helpers
└── tests/
    ├── integration/   # Integration tests
    └── unit/          # Unit tests
```

## Code Style
- Follow the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
- Use clang-format for code formatting
- Write meaningful commit messages following [Conventional Commits](https://www.conventionalcommits.org/)

## Testing
- Write unit tests for new features
- Ensure all tests pass before submitting PR
- Test on multiple Android versions (5.0+)
- Test on both Intel and Apple Silicon Macs

## Pull Request Process
1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Run tests and ensure they pass
5. Submit PR with clear description
6. Wait for review and address feedback

## License
By contributing, you agree that your contributions will be licensed under the Apache License 2.0.