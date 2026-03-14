#include "OpenXRApp.hpp"
#include <Geode/Geode.hpp>

using namespace geode::prelude;

OpenXRApp& OpenXRApp::get() {
    static OpenXRApp instance;
    return instance;
}

bool OpenXRApp::initialize() {
    if (!createInstance()) return false;
    if (!initializeSystem()) return false;
    if (!initializeSession()) return false;
    if (!createSwapchain()) return false;
    if (!createActions()) return false;
    
    m_isRunning = true;
    log::info("OpenXR App initialized successfully.");
    return true;
}

void OpenXRApp::cleanup() {
    if (m_session) xrDestroySession(m_session);
    if (m_instance) xrDestroyInstance(m_instance);
    m_session = XR_NULL_HANDLE;
    m_instance = XR_NULL_HANDLE;
}

bool OpenXRApp::createInstance() {
    XrInstanceCreateInfo createInfo = {XR_TYPE_INSTANCE_CREATE_INFO};
    strncpy(createInfo.applicationInfo.applicationName, "GDVR", XR_MAX_APPLICATION_NAME_SIZE);
    createInfo.applicationInfo.applicationVersion = 1;
    strncpy(createInfo.applicationInfo.engineName, "Cocos2d", XR_MAX_ENGINE_NAME_SIZE);
    createInfo.applicationInfo.engineVersion = 1;
    createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;

    std::vector<const char*> extensions;
#ifdef GEODE_IS_WIN32
    extensions.push_back(XR_KHR_OPENGL_ENABLE_EXTENSION_NAME);
#elif defined(GEODE_IS_ANDROID)
    extensions.push_back(XR_KHR_OPENGL_ES_ENABLE_EXTENSION_NAME);
#endif

    createInfo.enabledExtensionCount = (uint32_t)extensions.size();
    createInfo.enabledExtensionNames = extensions.data();

    XrResult res = xrCreateInstance(&createInfo, &m_instance);
    if (XR_FAILED(res)) {
        log::error("Failed to create OpenXR instance: {}", (int)res);
        return false;
    }
    return true;
}

bool OpenXRApp::initializeSystem() {
    XrSystemGetInfo systemInfo = {XR_TYPE_SYSTEM_GET_INFO};
    systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    XrResult res = xrGetSystem(m_instance, &systemInfo, &m_systemId);
    if (XR_FAILED(res)) {
        log::error("Failed to get OpenXR system: {}", (int)res);
        return false;
    }
    return true;
}

bool OpenXRApp::initializeSession() {
#ifdef GEODE_IS_WIN32
    // Setup OpenGL Graphics Binding for Win32
    PFN_xrGetOpenGLGraphicsRequirementsKHR pfnGetOpenGLGraphicsRequirementsKHR = nullptr;
    xrGetInstanceProcAddr(m_instance, "xrGetOpenGLGraphicsRequirementsKHR", (PFN_xrVoidFunction*)&pfnGetOpenGLGraphicsRequirementsKHR);
    
    XrGraphicsRequirementsOpenGLKHR reqs = {XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR};
    pfnGetOpenGLGraphicsRequirementsKHR(m_instance, m_systemId, &reqs);

    XrGraphicsBindingOpenGLWin32KHR graphicsBinding = {XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR};
    graphicsBinding.hDC = wglGetCurrentDC();
    graphicsBinding.hGLRC = wglGetCurrentContext();
#elif defined(GEODE_IS_ANDROID)
    // Setup OpenGLES Graphics Binding for Android
    PFN_xrGetOpenGLESGraphicsRequirementsKHR pfnGetOpenGLESGraphicsRequirementsKHR = nullptr;
    xrGetInstanceProcAddr(m_instance, "xrGetOpenGLESGraphicsRequirementsKHR", (PFN_xrVoidFunction*)&pfnGetOpenGLESGraphicsRequirementsKHR);
    
    XrGraphicsRequirementsOpenGLESKHR reqs = {XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGLES_KHR};
    pfnGetOpenGLESGraphicsRequirementsKHR(m_instance, m_systemId, &reqs);

    XrGraphicsBindingOpenGLESAndroidKHR graphicsBinding = {XR_TYPE_GRAPHICS_BINDING_OPENGL_ES_ANDROID_KHR};
    graphicsBinding.display = eglGetCurrentDisplay();
    graphicsBinding.config = (EGLConfig)0; // Or retrieve specific config
    graphicsBinding.context = eglGetCurrentContext();
#endif

    XrSessionCreateInfo sessionInfo = {XR_TYPE_SESSION_CREATE_INFO};
    sessionInfo.next = &graphicsBinding;
    sessionInfo.systemId = m_systemId;

    XrResult res = xrCreateSession(m_instance, &sessionInfo, &m_session);
    if (XR_FAILED(res)) {
        log::error("Failed to create OpenXR session: {}", (int)res);
        return false;
    }

    XrReferenceSpaceCreateInfo spaceInfo = {XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
    spaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
    spaceInfo.poseInReferenceSpace.orientation.w = 1.0f;
    
    res = xrCreateReferenceSpace(m_session, &spaceInfo, &m_playSpace);
    if (XR_FAILED(res)) return false;

    return true;
}

bool OpenXRApp::createSwapchain() {
    // We only need one swapchain for a single 2D quad overlay
    int64_t format = 0;
#ifdef GEODE_IS_WIN32
    format = GL_SRGB8_ALPHA8; // Typical OpenGL SRGB format
#elif defined(GEODE_IS_ANDROID)
    format = GL_SRGB8_ALPHA8; // Typical GLES SRGB format
#endif

    XrSwapchainCreateInfo swapchainInfo = {XR_TYPE_SWAPCHAIN_CREATE_INFO};
    swapchainInfo.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT | XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT;
    swapchainInfo.format = format;
    swapchainInfo.sampleCount = 1;
    swapchainInfo.width = 1920;  // Resolution for the virtual screen
    swapchainInfo.height = 1080;
    swapchainInfo.faceCount = 1;
    swapchainInfo.arraySize = 1;
    swapchainInfo.mipCount = 1;

    XrResult res = xrCreateSwapchain(m_session, &swapchainInfo, &m_swapchain.handle);
    if (XR_FAILED(res)) return false;

    m_swapchain.width = swapchainInfo.width;
    m_swapchain.height = swapchainInfo.height;

    uint32_t imageCount = 0;
    xrEnumerateSwapchainImages(m_swapchain.handle, 0, &imageCount, nullptr);

#ifdef GEODE_IS_WIN32
    m_swapchain.images.resize(imageCount, {XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR});
    xrEnumerateSwapchainImages(m_swapchain.handle, imageCount, &imageCount, (XrSwapchainImageBaseHeader*)m_swapchain.images.data());
#elif defined(GEODE_IS_ANDROID)
    m_swapchain.images.resize(imageCount, {XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR});
    xrEnumerateSwapchainImages(m_swapchain.handle, imageCount, &imageCount, (XrSwapchainImageBaseHeader*)m_swapchain.images.data());
#endif

    return true;
}

bool OpenXRApp::createActions() {
    // Basic action setup
    XrActionSetCreateInfo actionSetInfo = {XR_TYPE_ACTION_SET_CREATE_INFO};
    strncpy(actionSetInfo.actionSetName, "gameplay", XR_MAX_ACTION_SET_NAME_SIZE);
    strncpy(actionSetInfo.localizedActionSetName, "Gameplay", XR_MAX_LOCALIZED_ACTION_SET_NAME_SIZE);
    
    xrCreateActionSet(m_instance, &actionSetInfo, &m_actionSet);
    
    xrStringToPath(m_instance, "/user/hand/left", &m_leftHandPath);
    xrStringToPath(m_instance, "/user/hand/right", &m_rightHandPath);
    
    XrActionCreateInfo jumpInfo = {XR_TYPE_ACTION_CREATE_INFO};
    jumpInfo.actionType = XR_ACTION_TYPE_BOOLEAN_INPUT;
    strncpy(jumpInfo.actionName, "jump", XR_MAX_ACTION_NAME_SIZE);
    strncpy(jumpInfo.localizedActionName, "Jump", XR_MAX_LOCALIZED_ACTION_NAME_SIZE);
    xrCreateAction(m_actionSet, &jumpInfo, &m_jumpAction);
    
    XrActionCreateInfo moveInfo = {XR_TYPE_ACTION_CREATE_INFO};
    moveInfo.actionType = XR_ACTION_TYPE_VECTOR2F_INPUT;
    strncpy(moveInfo.actionName, "move", XR_MAX_ACTION_NAME_SIZE);
    strncpy(moveInfo.localizedActionName, "Move", XR_MAX_LOCALIZED_ACTION_NAME_SIZE);
    xrCreateAction(m_actionSet, &moveInfo, &m_moveAction);

    // Simple bindings for Oculus Touch controllers (Quest 1/2/3/Pro)
    XrPath profilePath, jumpPathRightA, jumpPathRightTrigger, jumpPathLeftTrigger, movePathLeftStick;
    xrStringToPath(m_instance, "/interaction_profiles/oculus/touch_controller", &profilePath);
    xrStringToPath(m_instance, "/user/hand/right/input/a/click", &jumpPathRightA);
    xrStringToPath(m_instance, "/user/hand/right/input/trigger/value", &jumpPathRightTrigger);
    xrStringToPath(m_instance, "/user/hand/left/input/trigger/value", &jumpPathLeftTrigger);
    xrStringToPath(m_instance, "/user/hand/left/input/thumbstick", &movePathLeftStick);

    std::vector<XrActionSuggestedBinding> bindings = {
        {m_jumpAction, jumpPathRightA},
        {m_jumpAction, jumpPathRightTrigger},
        {m_jumpAction, jumpPathLeftTrigger},
        {m_moveAction, movePathLeftStick}
    };

    XrInteractionProfileSuggestedBinding suggestedBindings = {XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
    suggestedBindings.interactionProfile = profilePath;
    suggestedBindings.suggestedBindings = bindings.data();
    suggestedBindings.countSuggestedBindings = (uint32_t)bindings.size();

    xrSuggestInteractionProfileBindings(m_instance, &suggestedBindings);

    XrSessionActionSetsAttachInfo attachInfo = {XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO};
    attachInfo.countActionSets = 1;
    attachInfo.actionSets = &m_actionSet;
    xrAttachSessionActionSets(m_session, &attachInfo);

    return true;
}

void OpenXRApp::pollEvents() {
    if (!m_instance) return;
    
    XrEventDataBuffer event = {XR_TYPE_EVENT_DATA_BUFFER};
    while (xrPollEvent(m_instance, &event) == XR_SUCCESS) {
        if (event.type == XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED) {
            auto* stateChanged = reinterpret_cast<XrEventDataSessionStateChanged*>(&event);
            m_sessionState = stateChanged->state;
            if (m_sessionState == XR_SESSION_STATE_READY) {
                XrSessionBeginInfo beginInfo = {XR_TYPE_SESSION_BEGIN_INFO};
                beginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
                xrBeginSession(m_session, &beginInfo);
            } else if (m_sessionState == XR_SESSION_STATE_STOPPING) {
                xrEndSession(m_session);
            }
        }
        event.type = XR_TYPE_EVENT_DATA_BUFFER; // Reset for next poll
    }

    if (m_sessionState == XR_SESSION_STATE_FOCUSED) {
        XrActiveActionSet activeActionSet = {m_actionSet, XR_NULL_PATH};
        XrActionsSyncInfo syncInfo = {XR_TYPE_ACTIONS_SYNC_INFO};
        syncInfo.countActiveActionSets = 1;
        syncInfo.activeActionSets = &activeActionSet;
        xrSyncActions(m_session, &syncInfo);

        // Get Jump State
        XrActionStateGetInfo getInfo = {XR_TYPE_ACTION_STATE_GET_INFO};
        getInfo.action = m_jumpAction;
        XrActionStateBoolean jumpState = {XR_TYPE_ACTION_STATE_BOOLEAN};
        xrGetActionStateBoolean(m_session, &getInfo, &jumpState);
        m_jumpPressed = (jumpState.isActive && jumpState.currentState);

        // Get Move State
        getInfo.action = m_moveAction;
        XrActionStateVector2f moveState = {XR_TYPE_ACTION_STATE_VECTOR2F};
        xrGetActionStateVector2f(m_session, &getInfo, &moveState);
        if (moveState.isActive) {
            m_moveX = moveState.currentState.x;
        } else {
            m_moveX = 0.0f;
        }
    }
}

void OpenXRApp::renderFrame(unsigned int sourceTexture) {
    if (!m_isRunning || m_sessionState < XR_SESSION_STATE_READY) return;

    XrFrameWaitInfo waitInfo = {XR_TYPE_FRAME_WAIT_INFO};
    XrFrameState frameState = {XR_TYPE_FRAME_STATE};
    xrWaitFrame(m_session, &waitInfo, &frameState);

    XrFrameBeginInfo beginInfo = {XR_TYPE_FRAME_BEGIN_INFO};
    xrBeginFrame(m_session, &beginInfo);

    std::vector<XrCompositionLayerBaseHeader*> layers;

    if (frameState.shouldRender && m_sessionState >= XR_SESSION_STATE_SYNCHRONIZED) {
        // Copy Cocos Texture to OpenXR Swapchain
        uint32_t imageIndex;
        XrSwapchainImageAcquireInfo acquireInfo = {XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
        xrAcquireSwapchainImage(m_swapchain.handle, &acquireInfo, &imageIndex);

        XrSwapchainImageWaitInfo waitImageInfo = {XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
        waitImageInfo.timeout = XR_INFINITE_DURATION;
        xrWaitSwapchainImage(m_swapchain.handle, &waitImageInfo);

        // --- GL Blit Framebuffer ---
        // Save current FBO state just in case
        GLint restoreReadFBO, restoreDrawFBO;
        glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &restoreReadFBO);
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &restoreDrawFBO);

        // Setup FBOs
        GLuint readFbo, drawFbo;
        glGenFramebuffers(1, &readFbo);
        glGenFramebuffers(1, &drawFbo);

        glBindFramebuffer(GL_READ_FRAMEBUFFER, readFbo);
        glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sourceTexture, 0);

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, drawFbo);
        uint32_t destTex = m_swapchain.images[imageIndex].image;
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, destTex, 0);

        // Blit
        glBlitFramebuffer(
            0, 0, CCDirector::sharedDirector()->getWinSizeInPixels().width, CCDirector::sharedDirector()->getWinSizeInPixels().height,
            0, 0, m_swapchain.width, m_swapchain.height,
            GL_COLOR_BUFFER_BIT, GL_LINEAR
        );

        // Cleanup
        glDeleteFramebuffers(1, &readFbo);
        glDeleteFramebuffers(1, &drawFbo);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, restoreReadFBO);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, restoreDrawFBO);

        XrSwapchainImageReleaseInfo releaseInfo = {XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
        xrReleaseSwapchainImage(m_swapchain.handle, &releaseInfo);

        // Setup Quad Composition Layer
        static XrCompositionLayerQuad layer = {XR_TYPE_COMPOSITION_LAYER_QUAD};
        layer.space = m_playSpace;
        
        // Grab settings
        float distance = Mod::get()->getSettingValue<double>("screen-distance");
        float size = Mod::get()->getSettingValue<double>("screen-size");

        layer.pose.orientation.w = 1.0f;
        layer.pose.position.x = 0.0f;
        layer.pose.position.y = 1.5f; // eye height roughly
        layer.pose.position.z = -distance;
        layer.size.width = size;
        layer.size.height = size * (m_swapchain.height / (float)m_swapchain.width); // maintain aspect ratio
        layer.subImage.swapchain = m_swapchain.handle;
        layer.subImage.imageRect.offset.x = 0;
        layer.subImage.imageRect.offset.y = 0;
        layer.subImage.imageRect.extent.width = m_swapchain.width;
        layer.subImage.imageRect.extent.height = m_swapchain.height;

        layers.push_back((XrCompositionLayerBaseHeader*)&layer);
    }

    XrFrameEndInfo endInfo = {XR_TYPE_FRAME_END_INFO};
    endInfo.displayTime = frameState.predictedDisplayTime;
    endInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    endInfo.layerCount = (uint32_t)layers.size();
    endInfo.layers = layers.data();

    xrEndFrame(m_session, &endInfo);
}
