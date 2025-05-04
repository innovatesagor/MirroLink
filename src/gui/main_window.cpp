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
    cleanup();
}

bool MainWindow::initialize(const char* title, int width, int height) {
    PERFORMANCE_SCOPE("MainWindow::Initialize");
    
    windowWidth = width;
    windowHeight = height;

    try {
        // Initialize SDL with error recovery
        int retryCount = 0;
        const int maxRetries = 3;
        while (retryCount < maxRetries) {
            if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_AUDIO) >= 0) {
                break;
            }
            utils::Logger::getInstance().warn("SDL initialization failed, attempt ", 
                retryCount + 1, " of ", maxRetries, ": ", SDL_GetError());
            retryCount++;
            SDL_Delay(1000);  // Wait before retry
        }
        
        if (retryCount == maxRetries) {
            throw utils::Error("SDL initialization failed after multiple attempts: " + 
                std::string(SDL_GetError()));
        }
        utils::Logger::getInstance().debug("SDL initialized successfully");

        // Create window with proper error checking
        window = SDL_CreateWindow(
            title,
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            windowWidth, windowHeight,
            SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
        );

        if (!window) {
            throw utils::Error("Window creation failed: " + std::string(SDL_GetError()));
        }
        utils::Logger::getInstance().debug("Window created successfully");

        // Create renderer with preferred settings or fallback
        SDL_RendererFlags renderFlags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC;
        renderer = SDL_CreateRenderer(window, -1, renderFlags);
            
        if (!renderer) {
            utils::Logger::getInstance().warn("Failed to create accelerated renderer, falling back to software renderer");
            renderFlags = SDL_RENDERER_SOFTWARE;
            renderer = SDL_CreateRenderer(window, -1, renderFlags);
            
            if (!renderer) {
                throw utils::Error("Renderer creation failed: " + std::string(SDL_GetError()));
            }
        }
        utils::Logger::getInstance().debug("Renderer created successfully");

        // Initialize device manager with error recovery
        int deviceRetryCount = 0;
        const int maxDeviceRetries = 3;
        while (deviceRetryCount < maxDeviceRetries) {
            try {
                deviceManager = std::make_unique<DeviceManager>();
                if (deviceManager->initialize()) {
                    break;
                }
            } catch (const std::exception& e) {
                utils::Logger::getInstance().warn("Device manager initialization failed, attempt ", 
                    deviceRetryCount + 1, " of ", maxDeviceRetries, ": ", e.what());
            }
            deviceRetryCount++;
            SDL_Delay(1000);  // Wait before retry
        }
        
        if (deviceRetryCount == maxDeviceRetries) {
            throw utils::Error("Device manager initialization failed after multiple attempts");
        }
        utils::Logger::getInstance().info("Device manager initialized successfully");

        // Set up device callbacks with exception handling
        deviceManager->onDeviceConnected([this](const DeviceInfo& device) {
            try {
                this->onDeviceConnected(device);
            } catch (const std::exception& e) {
                utils::Logger::getInstance().error("Error in device connected callback: ", e.what());
            }
        });
        
        deviceManager->onDeviceDisconnected([this](const DeviceInfo& device) {
            try {
                this->onDeviceDisconnected(device);
            } catch (const std::exception& e) {
                utils::Logger::getInstance().error("Error in device disconnected callback: ", e.what());
            }
        });

        // Initialize screen mirror with error recovery
        screenMirror = std::make_unique<ScreenMirror>();
        screenMirror->setFrameCallback([this](const FrameData& frame) {
            try {
                PERFORMANCE_SCOPE("Frame Processing");
                this->onFrameReceived(frame);
            } catch (const std::exception& e) {
                utils::Logger::getInstance().error("Error processing frame: ", e.what());
            }
        });

        isRunning = true;
        utils::Logger::getInstance().info("Main window initialized successfully");
        return true;
        
    } catch (const std::exception& e) {
        utils::Logger::getInstance().error("Initialization failed with exception: ", e.what());
        cleanup();
        return false;
    }
}

void MainWindow::run() {
    PERFORMANCE_SCOPE("MainWindow::Run");
    
    SDL_Event event;
    Uint32 frameStart;
    int frameCount = 0;
    Uint32 fpsTimer = SDL_GetTicks();
    const int frameDelay = 1000 / 60;  // Target 60 FPS
    
    while (isRunning) {
        frameStart = SDL_GetTicks();
        
        try {
            // Event handling with error recovery
            while (SDL_PollEvent(&event)) {
                try {
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
                            } else if (event.window.event == SDL_WINDOWEVENT_RESTORED) {
                                // Handle window restore after minimize
                                SDL_RaiseWindow(window);
                            }
                            break;

                        case SDL_RENDER_DEVICE_RESET:
                            // Handle graphics device reset (e.g., display change)
                            handleRendererReset();
                            break;
                    }
                } catch (const std::exception& e) {
                    utils::Logger::getInstance().error("Error handling event: ", e.what());
                }
            }

            // Render frame with error handling
            if (SDL_RenderClear(renderer) < 0) {
                throw utils::Error("Failed to clear renderer: " + std::string(SDL_GetError()));
            }
            
            if (frameTexture) {
                if (SDL_RenderCopy(renderer, frameTexture, nullptr, nullptr) < 0) {
                    throw utils::Error("Failed to copy texture: " + std::string(SDL_GetError()));
                }
            }
            
            SDL_RenderPresent(renderer);

            // Frame rate control and monitoring
            frameCount++;
            if (SDL_GetTicks() - fpsTimer >= 1000) {
                float fps = frameCount / ((SDL_GetTicks() - fpsTimer) / 1000.0f);
                utils::Logger::getInstance().debug("Current FPS: ", fps);
                frameCount = 0;
                fpsTimer = SDL_GetTicks();
            }

            // Frame timing
            int frameTime = SDL_GetTicks() - frameStart;
            if (frameDelay > frameTime) {
                SDL_Delay(frameDelay - frameTime);
            }
            
        } catch (const std::exception& e) {
            utils::Logger::getInstance().error("Error in main loop: ", e.what());
            if (!recoverFromError()) {
                isRunning = false;
                break;
            }
            SDL_Delay(100);  // Prevent rapid error logging
        }
    }
    
    utils::Logger::getInstance().info("Application shutting down");
    cleanup();
}

void MainWindow::cleanup() {
    if (frameTexture) {
        SDL_DestroyTexture(frameTexture);
        frameTexture = nullptr;
    }
    if (renderer) {
        SDL_DestroyRenderer(renderer);
        renderer = nullptr;
    }
    if (window) {
        SDL_DestroyWindow(window);
        window = nullptr;
    }
    SDL_Quit();
    utils::Logger::getInstance().debug("Cleanup completed");
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

void MainWindow::handleRendererReset() {
    utils::Logger::getInstance().warn("Graphics device reset detected, attempting recovery");
    
    try {
        if (frameTexture) {
            SDL_DestroyTexture(frameTexture);
            frameTexture = nullptr;
        }
        
        if (renderer) {
            SDL_DestroyRenderer(renderer);
            renderer = nullptr;
        }
        
        renderer = SDL_CreateRenderer(window, -1, 
            SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
            
        if (!renderer) {
            throw utils::Error("Failed to recreate renderer: " + std::string(SDL_GetError()));
        }
        
        utils::Logger::getInstance().info("Graphics device recovery successful");
    } catch (const std::exception& e) {
        utils::Logger::getInstance().error("Failed to recover from graphics device reset: ", e.what());
        isRunning = false;
    }
}

bool MainWindow::recoverFromError() {
    static int errorCount = 0;
    static Uint32 lastErrorTime = 0;
    Uint32 currentTime = SDL_GetTicks();
    
    // Reset error count if enough time has passed
    if (currentTime - lastErrorTime > 5000) {
        errorCount = 0;
    }
    
    errorCount++;
    lastErrorTime = currentTime;
    
    // If too many errors occur in a short time, stop the application
    if (errorCount > 5) {
        utils::Logger::getInstance().error("Too many errors occurred, shutting down");
        return false;
    }
    
    // Attempt basic recovery
    try {
        if (screenMirror && screenMirror->isActive()) {
            screenMirror->stop();
            SDL_Delay(100);
            screenMirror->start(screenMirror->getConfig());
        }
        return true;
    } catch (const std::exception& e) {
        utils::Logger::getInstance().error("Recovery attempt failed: ", e.what());
        return false;
    }
}

}} // namespace mirrolink::gui