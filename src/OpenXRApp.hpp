#pragma once

#include <Geode/Geode.hpp>

// OpenXR & OpenGL Platform headers
#ifdef GEODE_IS_WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define XR_USE_PLATFORM_WIN32 1
#define XR_USE_GRAPHICS_API_OPENGL 1
#include <GL/gl.h>
#endif

#ifdef GEODE_IS_ANDROID
#define XR_USE_PLATFORM_ANDROID 1
#define XR_USE_GRAPHICS_API_OPENGL_ES 1
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#endif

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#include <vector>

struct Swapchain {
    XrSwapchain handle = XR_NULL_HANDLE;
    int32_t width = 0;
    int32_t height = 0;
    std::vector<XrSwapchainImageOpenGLKHR> images; // Also used for GLES
    // For GLES, XrSwapchainImageOpenGLESKHR is identical in layout to XrSwapchainImageOpenGLKHR
};

class OpenXRApp {
public:
    static OpenXRApp& get();

    bool initialize();
    void renderFrame(unsigned int sourceTexture);
    void pollEvents();
    void cleanup();

    // Input States
    bool getJumpState() const { return m_jumpPressed; }
    float getMoveX() const { return m_moveX; }

private:
    bool createInstance();
    bool initializeSystem();
    bool initializeSession();
    bool createSwapchain();
    bool createActions();

    XrInstance m_instance = XR_NULL_HANDLE;
    XrSystemId m_systemId = XR_NULL_SYSTEM_ID;
    XrSession m_session = XR_NULL_HANDLE;
    XrSpace m_playSpace = XR_NULL_HANDLE;
    XrSessionState m_sessionState = XR_SESSION_STATE_UNKNOWN;
    
    Swapchain m_swapchain;
    
    // Actions
    XrActionSet m_actionSet = XR_NULL_HANDLE;
    XrAction m_jumpAction = XR_NULL_HANDLE;
    XrAction m_moveAction = XR_NULL_HANDLE;
    XrPath m_leftHandPath;
    XrPath m_rightHandPath;

    // Input values
    bool m_jumpPressed = false;
    float m_moveX = 0.0f;

    bool m_isRunning = false;
};
