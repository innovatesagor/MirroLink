# MirroLink: Android-to-Mac Mirroring & Control

**MirroLink** is a cutting-edge, open-source tool that enables seamless Android screen mirroring and control on macOS via a USB-C cable. With MirroLink, you can view and interact with your Android device directly from your Mac, even if the Android screen is off, locked, or broken. Designed for simplicity, privacy, and high performance, MirroLink is the ultimate plug-and-play solution for Android-to-Mac integration.

---

## 🔑 Key Features

- **Full-HD Display Mirroring**: Supports resolutions of **1920×1080** (or higher) at **30–120 FPS**, with hardware-accelerated (H.264) video encoding for smooth visuals.
- **Ultra-Low Latency**: Achieves latency as low as **35–70 ms**, ensuring a responsive and fluid experience for video playback, gaming, and more.
- **Complete Remote Control**: Use your Mac’s **keyboard**, **mouse**, or **gamepad** to simulate Android touchscreen and button inputs. Navigate effortlessly with keyboard shortcuts like right-click for BACK and middle-click for HOME.
- **Clipboard Sync**: Copy and paste text seamlessly between your Mac and Android device.
- **Works with Screen Off/Broken**: Control your Android device even if the screen is off, locked, or damaged, making it ideal for troubleshooting or recovering data.
- **Plug-and-Play USB-C**: No app installation or root access required. Simply enable USB Debugging on Android, connect via USB-C, and start mirroring instantly.
- **Audio Forwarding**: For Android 11+, forward audio output to your Mac via USB for a complete multimedia experience.
- **Recording & Snapshots**: Capture screenshots or record the mirrored screen directly on your Mac for easy sharing or documentation.
- **Ad-Free & Private**: No ads, no accounts, and no internet connection required. Everything runs locally on your machine, ensuring complete privacy.
- **Cross-Platform Core**: While optimized for macOS, MirroLink’s core (based on scrcpy) also works on Linux and Windows, offering flexibility across platforms.

---

## 🚀 Getting Started

### Prerequisites

1. **Android Device**:
   - Android 5.0 (API 21) or newer.
   - Enable USB Debugging:
     - Go to `Settings > About Phone > Tap Build Number 7 times` to unlock Developer Options.
     - Navigate to `Settings > Developer Options > Enable USB Debugging`.
2. **Mac Computer**:
   - Install Android Platform Tools (ADB) and MirroLink.
3. **USB-C Cable**:
   - Use a high-quality USB-C cable for the best performance.

---

### Installation

#### Step 1: Install Android Platform Tools
Install ADB on macOS using Homebrew:
```bash
brew install --cask android-platform-tools
```

#### Step 2: Install MirroLink
Install MirroLink (based on scrcpy) via Homebrew:
```bash
brew install scrcpy
```

Alternatively, download the latest MirroLink release from the [GitHub repository](https://github.com/innovatesagor/MirroLink).

#### Step 3: Launch MirroLink
Run MirroLink from the terminal:
```bash
mirrolink
```