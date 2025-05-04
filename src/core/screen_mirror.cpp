#include "screen_mirror.hpp"
#include "../utils/logger.hpp"
#include "../utils/error.hpp"
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <thread>
#include <atomic>
#include <array>
#include <cstdio>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

namespace mirrolink {

class ScreenMirror::Impl {
public:
    Impl() : active(false), recording(false) {
        inputHandler = std::make_unique<InputHandler>();
    }
    
    bool start(const ScreenConfig& config) {
        PERFORMANCE_SCOPE("ScreenMirror::Start");

        if (active) {
            utils::Logger::getInstance().warn("Screen mirror already active, stopping previous session");
            stop();
        }
        
        try {
            currentConfig = config;
            
            // Validate configuration
            if (config.width <= 0 || config.height <= 0) {
                utils::Logger::getInstance().error("Invalid resolution: ", config.width, "x", config.height);
                return false;
            }
            if (config.maxFps <= 0 || config.maxFps > 120) {
                utils::Logger::getInstance().error("Invalid FPS setting: ", config.maxFps);
                return false;
            }
            
            if (!setupAdbForward()) {
                utils::Logger::getInstance().error("Failed to set up ADB forwarding");
                return false;
            }
            
            utils::Logger::getInstance().debug("ADB forwarding set up successfully");
            
            if (!initializeEncoder()) {
                utils::Logger::getInstance().error("Failed to initialize video encoder");
                cleanupAdbForward();
                return false;
            }
            
            utils::Logger::getInstance().debug("Video encoder initialized successfully");
            
            active = true;
            captureThread = std::thread(&Impl::captureLoop, this);
            utils::Logger::getInstance().info("Screen mirroring started with config: ",
                config.width, "x", config.height, " @ ", config.maxFps, "fps");
            return true;
            
        } catch (const std::exception& e) {
            utils::Logger::getInstance().error("Unexpected error during screen mirror start: ", e.what());
            cleanup();
            return false;
        }
    }
    
    void stop() {
        if (!active) {
            return;
        }
        
        active = false;
        if (captureThread.joinable()) {
            captureThread.join();
        }
        
        cleanup();
    }
    
    void setFrameCallback(FrameCallback cb) {
        std::lock_guard<std::mutex> lock(callbackMutex);
        frameCallback = cb;
    }
    
    bool startRecording(const std::string& path) {
        if (recording || !active) {
            return false;
        }
        
        if (!initializeRecording(path)) {
            return false;
        }
        
        recording = true;
        return true;
    }
    
    void stopRecording() {
        if (!recording) {
            return;
        }
        
        recording = false;
        cleanupRecording();
    }

private:
    bool setupAdbForward() {
        // Start scrcpy server on device
        std::array<char, 128> buffer;
        std::string result;
        
        // Push scrcpy-server to device
        std::string cmd = "adb push scrcpy-server /data/local/tmp/";
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) {
            utils::Logger::getInstance().error("Failed to push scrcpy server");
            return false;
        }
        while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
            result += buffer.data();
        }
        pclose(pipe);
        
        // Start server
        cmd = "adb shell CLASSPATH=/data/local/tmp/scrcpy-server app_process / com.genymobile.scrcpy.Server " 
              + std::to_string(currentConfig.width) + " "
              + std::to_string(currentConfig.maxFps) + " "
              + std::to_string(currentConfig.videoBitrate);
              
        pipe = popen(cmd.c_str(), "r");
        if (!pipe) {
            utils::Logger::getInstance().error("Failed to start scrcpy server");
            return false;
        }
        pclose(pipe);
        
        // Forward local port
        cmd = "adb forward tcp:27183 localabstract:scrcpy";
        pipe = popen(cmd.c_str(), "r");
        if (!pipe) {
            utils::Logger::getInstance().error("Failed to set up port forwarding");
            return false;
        }
        pclose(pipe);
        
        return true;
    }
    
    void cleanupAdbForward() {
        system("adb forward --remove tcp:27183");
    }
    
    bool initializeEncoder() {
        // Initialize FFmpeg components for H.264 decoding
        codec = avcodec_find_decoder(AV_CODEC_ID_H264);
        if (!codec) {
            utils::Logger::getInstance().error("H.264 codec not found");
            return false;
        }
        
        codecContext = avcodec_alloc_context3(codec);
        if (!codecContext) {
            utils::Logger::getInstance().error("Could not allocate codec context");
            return false;
        }
        
        // Configure decoder
        codecContext->width = currentConfig.width;
        codecContext->height = currentConfig.height;
        codecContext->time_base = (AVRational){1, currentConfig.maxFps};
        codecContext->framerate = (AVRational){currentConfig.maxFps, 1};
        codecContext->pix_fmt = AV_PIX_FMT_YUV420P;
        
        if (avcodec_open2(codecContext, codec, nullptr) < 0) {
            utils::Logger::getInstance().error("Could not open codec");
            cleanupEncoder();
            return false;
        }
        
        // Initialize conversion context for YUV to RGB
        swsContext = sws_getContext(
            currentConfig.width, currentConfig.height, AV_PIX_FMT_YUV420P,
            currentConfig.width, currentConfig.height, AV_PIX_FMT_RGBA,
            SWS_BILINEAR, nullptr, nullptr, nullptr
        );
        
        if (!swsContext) {
            utils::Logger::getInstance().error("Could not initialize conversion context");
            cleanupEncoder();
            return false;
        }
        
        return true;
    }
    
    void cleanupEncoder() {
        if (swsContext) {
            sws_freeContext(swsContext);
            swsContext = nullptr;
        }
        if (codecContext) {
            avcodec_free_context(&codecContext);
        }
    }
    
    bool initializeRecording(const std::string& path) {
        if (recording) {
            return false;
        }

        // Initialize output format context
        avformat_alloc_output_context2(&formatContext, nullptr, nullptr, path.c_str());
        if (!formatContext) {
            utils::Logger::getInstance().error("Could not create output context");
            return false;
        }

        // Add video stream
        stream = avformat_new_stream(formatContext, codec);
        if (!stream) {
            utils::Logger::getInstance().error("Could not create video stream");
            cleanupRecording();
            return false;
        }

        // Configure stream
        stream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
        stream->codecpar->codec_id = AV_CODEC_ID_H264;
        stream->codecpar->width = currentConfig.width;
        stream->codecpar->height = currentConfig.height;
        stream->codecpar->format = AV_PIX_FMT_YUV420P;
        stream->time_base = (AVRational){1, currentConfig.maxFps};

        // Open output file
        if (avio_open(&formatContext->pb, path.c_str(), AVIO_FLAG_WRITE) < 0) {
            utils::Logger::getInstance().error("Could not open output file");
            cleanupRecording();
            return false;
        }

        // Write header
        if (avformat_write_header(formatContext, nullptr) < 0) {
            utils::Logger::getInstance().error("Could not write header");
            cleanupRecording();
            return false;
        }

        recordingStartTime = std::chrono::steady_clock::now();
        return true;
    }

    void cleanupRecording() {
        if (formatContext) {
            if (formatContext->pb) {
                av_write_trailer(formatContext);
                avio_closep(&formatContext->pb);
            }
            avformat_free_context(formatContext);
            formatContext = nullptr;
        }
        stream = nullptr;
    }
    
    void captureLoop() {
        PERFORMANCE_SCOPE("ScreenMirror::CaptureLoop");
        
        int sockfd = connectToServer();
        if (sockfd < 0) {
            utils::Logger::getInstance().error("Failed to connect to scrcpy server");
            return;
        }
        
        utils::Logger::getInstance().debug("Connected to scrcpy server successfully");
        
        AVPacket* packet = av_packet_alloc();
        AVFrame* frame = av_frame_alloc();
        
        // Track frame statistics for performance monitoring
        int frameCount = 0;
        auto lastStatsTime = std::chrono::steady_clock::now();
        
        while (active) {
            try {
                // Read video packet from socket
                if (!readVideoPacket(sockfd, packet)) {
                    utils::Logger::getInstance().warn("Failed to read video packet, retrying...");
                    continue;
                }
                
                // Decode video frame
                if (avcodec_send_packet(codecContext, packet) < 0) {
                    utils::Logger::getInstance().warn("Failed to send packet to decoder");
                    continue;
                }
                
                if (avcodec_receive_frame(codecContext, frame) < 0) {
                    utils::Logger::getInstance().warn("Failed to receive frame from decoder");
                    continue;
                }
                
                frameCount++;
                auto now = std::chrono::steady_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - lastStatsTime);
                
                // Log performance stats every 5 seconds
                if (duration.count() >= 5) {
                    float fps = frameCount / static_cast<float>(duration.count());
                    utils::Logger::getInstance().debug("Mirroring performance: ", 
                        fps, " FPS, Frame size: ", frame->width, "x", frame->height);
                    
                    frameCount = 0;
                    lastStatsTime = now;
                }
                
                // Convert YUV to RGBA
                FrameData frameData;
                frameData.width = currentConfig.width;
                frameData.height = currentConfig.height;
                frameData.format = AV_PIX_FMT_RGBA;
                frameData.timestamp = frame->pts;
                frameData.data.resize(currentConfig.width * currentConfig.height * 4);
                
                uint8_t* destSlice[] = { frameData.data.data() };
                int destStride[] = { currentConfig.width * 4 };
                
                sws_scale(swsContext, frame->data, frame->linesize, 0,
                         currentConfig.height, destSlice, destStride);
                
                // Notify callback
                std::lock_guard<std::mutex> lock(callbackMutex);
                if (frameCallback) {
                    frameCallback(frameData);
                }
                
            } catch (const std::exception& e) {
                utils::Logger::getInstance().error("Error in capture loop: ", e.what());
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
        
        av_frame_free(&frame);
        av_packet_free(&packet);
        close(sockfd);
        
        utils::Logger::getInstance().info("Screen mirroring stopped");
    }
    
    int connectToServer() {
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            utils::Logger::getInstance().error("Failed to create socket");
            return -1;
        }
        
        struct sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(27183);
        serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
        
        if (connect(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            utils::Logger::getInstance().error("Failed to connect to scrcpy server");
            close(sockfd);
            return -1;
        }
        
        return sockfd;
    }
    
    bool readVideoPacket(int sockfd, AVPacket* packet) {
        // Read packet header (12 bytes)
        uint8_t header[12];
        ssize_t n = read(sockfd, header, sizeof(header));
        if (n != sizeof(header)) {
            return false;
        }
        
        // Parse packet size from header (first 4 bytes, big-endian)
        uint32_t packetSize = (header[0] << 24) | (header[1] << 16) | 
                             (header[2] << 8) | header[3];
        
        // Allocate packet data
        if (av_new_packet(packet, packetSize) < 0) {
            return false;
        }
        
        // Read packet payload
        size_t totalRead = 0;
        while (totalRead < packetSize) {
            n = read(sockfd, packet->data + totalRead, packetSize - totalRead);
            if (n <= 0) {
                av_packet_unref(packet);
                return false;
            }
            totalRead += n;
        }
        
        // Set packet timestamp from header
        packet->pts = (header[4] << 56) | (header[5] << 48) | 
                     (header[6] << 40) | (header[7] << 32) |
                     (header[8] << 24) | (header[9] << 16) |
                     (header[10] << 8) | header[11];
        
        return true;
    }
    
    void cleanup() {
        cleanupEncoder();
        cleanupAdbForward();
        active = false;
    }
    
    std::atomic<bool> active;
    std::atomic<bool> recording;
    std::thread captureThread;
    std::mutex callbackMutex;
    FrameCallback frameCallback;
    ScreenConfig currentConfig;
    std::unique_ptr<InputHandler> inputHandler;
    
    // FFmpeg components
    const AVCodec* codec{nullptr};
    AVCodecContext* codecContext{nullptr};
    SwsContext* swsContext{nullptr};

    // Recording components
    AVFormatContext* formatContext{nullptr};
    AVStream* stream{nullptr};
    std::chrono::steady_clock::time_point recordingStartTime;
};

// Public interface implementation
ScreenMirror::ScreenMirror() : pimpl(std::make_unique<Impl>()) {
    inputHandler = std::make_unique<InputHandler>();
}
ScreenMirror::~ScreenMirror() = default;

bool ScreenMirror::start(const ScreenConfig& config) {
    return pimpl->start(config);
}

void ScreenMirror::stop() {
    pimpl->stop();
}

void ScreenMirror::setFrameCallback(FrameCallback callback) {
    pimpl->setFrameCallback(callback);
}

ScreenConfig ScreenMirror::getConfig() const {
    return pimpl->currentConfig;
}

bool ScreenMirror::updateConfig(const ScreenConfig& config) {
    stop();
    return start(config);
}

bool ScreenMirror::isActive() const {
    return pimpl->active;
}

bool ScreenMirror::startRecording(const std::string& path) {
    return pimpl->startRecording(path);
}

void ScreenMirror::stopRecording() {
    pimpl->stopRecording();
}

InputHandler& ScreenMirror::getInputHandler() {
    return *inputHandler;
}

} // namespace mirrolink