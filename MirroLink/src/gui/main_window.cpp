#include "main_window.hpp"
#include "../utils/logger.hpp"
#include "../utils/error.hpp"
#include <SDL2/SDL_image.h>

namespace mirrolink {
namespace gui {

MainWindow::MainWindow()
    : window(nullptr)
    , renderer(nullptr)
    , frameTexture(nullptr)
    , windowWidth(1280)
    , windowHeight(720)
    , isRunning(false)
    , fullscreenMode(false)
{
    deviceManager = std::make_unique<DeviceManager>();
    screenMirror = std::make_unique<ScreenMirror>();
}

MainWindow::~MainWindow() {
    if (frameTexture) {
        SDL_DestroyTexture(frameTexture);
    }
    if (renderer) {
        SDL_DestroyRenderer(renderer);
    }
    if (window) {
        SDL_DestroyWindow(window);
    }
    SDL_Quit();
}

bool MainWindow::initialize(const char* title, int width, int height) {
    windowWidth = width;
    windowHeight = height;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) {
        utils::Logger::getInstance().error("SDL initialization failed: ", SDL_GetError());
        return false;
    }

    window = SDL_CreateWindow(
        title,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        windowWidth, windowHeight,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );

    if (!window) {
        utils::Logger::getInstance().error("Window creation failed: ", SDL_GetError());
        return false;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        utils::Logger::getInstance().error("Renderer creation failed: ", SDL_GetError());
        return false;
    }

    // Initialize device manager
    if (!deviceManager->initialize()) {
        utils::Logger::getInstance().error("Device manager initialization failed");
        return false;
    }

    // Set up screen mirror callback
    screenMirror->setFrameCallback([this](const FrameData& frame) {
        this->onFrameReceived(frame);
    });

    isRunning = true;
    return true;
}

void MainWindow::run() {
    SDL_Event event;
    while (isRunning) {
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    isRunning = false;
                    break;

                case SDL_KEYDOWN:
                case SDL_KEYUP:
                    handleKeyboard(event.key);
                    break;

                case SDL_MOUSEBUTTONDOWN:
                case SDL_MOUSEBUTTONUP:
                    handleMouse(event.button);
                    break;

                case SDL_MOUSEMOTION:
                    handleMouseMotion(event.motion);
                    break;

                case SDL_WINDOWEVENT:
                    if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                        windowWidth = event.window.data1;
                        windowHeight = event.window.data2;
                        resize(windowWidth, windowHeight);
                    }
                    break;
            }
        }

        // Render current frame
        SDL_RenderClear(renderer);
        if (frameTexture) {
            SDL_RenderCopy(renderer, frameTexture, nullptr, nullptr);
        }
        SDL_RenderPresent(renderer);

        SDL_Delay(1); // Prevent high CPU usage
    }
}

void MainWindow::setFullscreen(bool fullscreen) {
    if (fullscreen == fullscreenMode) return;

    Uint32 flags = fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0;
    if (SDL_SetWindowFullscreen(window, flags) < 0) {
        utils::Logger::getInstance().error("Failed to toggle fullscreen: ", SDL_GetError());
        return;
    }

    fullscreenMode = fullscreen;
}

void MainWindow::handleKeyboard(const SDL_KeyboardEvent& event) {
    KeyboardEvent keyEvent{
        .keycode = event.keysym.scancode,
        .pressed = event.type == SDL_KEYDOWN,
        .ctrl = (event.keysym.mod & KMOD_CTRL) != 0,
        .alt = (event.keysym.mod & KMOD_ALT) != 0,
        .shift = (event.keysym.mod & KMOD_SHIFT) != 0
    };

    // Handle special keys
    if (event.type == SDL_KEYDOWN) {
        if (event.keysym.scancode == SDL_SCANCODE_F11) {
            setFullscreen(!fullscreenMode);
            return;
        }
    }

    // Forward other keys to input handler
    screenMirror->getInputHandler().sendKeyEvent(keyEvent);
}

void MainWindow::handleMouse(const SDL_MouseButtonEvent& event) {
    TouchEvent touchEvent{
        .id = event.which,
        .x = static_cast<float>(event.x) / windowWidth,
        .y = static_cast<float>(event.y) / windowHeight,
        .pressed = event.type == SDL_MOUSEBUTTONDOWN
    };

    screenMirror->getInputHandler().sendTouchEvent(touchEvent);
}

void MainWindow::handleMouseMotion(const SDL_MouseMotionEvent& event) {
    if (event.state & SDL_BUTTON_LMASK) {
        TouchEvent touchEvent{
            .id = event.which,
            .x = static_cast<float>(event.x) / windowWidth,
            .y = static_cast<float>(event.y) / windowHeight,
            .pressed = true
        };

        screenMirror->getInputHandler().sendTouchEvent(touchEvent);
    }
}

void MainWindow::onDeviceConnected(const DeviceInfo& device) {
    utils::Logger::getInstance().info("Device connected: ", device.model);
    
    ScreenConfig config{
        .width = windowWidth,
        .height = windowHeight,
        .maxFps = 60
    };

    if (!screenMirror->start(config)) {
        utils::Logger::getInstance().error("Failed to start screen mirroring");
    }
}

void MainWindow::onDeviceDisconnected(const DeviceInfo& device) {
    utils::Logger::getInstance().info("Device disconnected: ", device.model);
    screenMirror->stop();
}

void MainWindow::onFrameReceived(const FrameData& frame) {
    if (!frameTexture) {
        frameTexture = SDL_CreateTexture(
            renderer,
            SDL_PIXELFORMAT_RGBA8888,
            SDL_TEXTUREACCESS_STREAMING,
            frame.width,
            frame.height
        );
    }

    if (frameTexture) {
        SDL_UpdateTexture(frameTexture, nullptr, frame.data.data(), frame.width * 4);
    }
}

void MainWindow::resize(int width, int height) {
    windowWidth = width;
    windowHeight = height;
    
    // Update screen mirror configuration if active
    if (screenMirror->isActive()) {
        ScreenConfig config{
            .width = width,
            .height = height,
            .maxFps = 60
        };
        screenMirror->updateConfig(config);
    }
}

}} // namespace mirrolink::gui