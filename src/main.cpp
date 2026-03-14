#include <Geode/Geode.hpp>
#include <Geode/modify/CCEGLView.hpp>
#include <Geode/modify/CCDirector.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include "OpenXRApp.hpp"

using namespace geode::prelude;

bool g_openXrInitialized = false;
bool g_wasJumpPressed = false;
bool g_wasLeftPressed = false;
bool g_wasRightPressed = false;

class $modify(CCEGLView) {
    void swapBuffers() {
        if (g_openXrInitialized) {
            OpenXRApp::get().pollEvents();
            OpenXRApp::get().renderFrame(0);
        }
        CCEGLView::swapBuffers();
    }
};

class $modify(CCDirector) {
    void drawScene() {
        if (!g_openXrInitialized) {
            OpenXRApp::get().initialize();
            g_openXrInitialized = true;
        }

        auto fps = Mod::get()->getSettingValue<int64_t>("fps-overclock");
        if (fps > 0) {
            CCDirector::sharedDirector()->setAnimationInterval(1.0 / fps);
        }

        // Handle VR Input in PlayLayer
        if (g_openXrInitialized) {
            auto playLayer = PlayLayer::get();
            if (playLayer) {
                bool isJumpPressed = OpenXRApp::get().getJumpState();
                if (isJumpPressed && !g_wasJumpPressed) {
                    playLayer->pushButton((int)PlayerButton::Jump, true);
                } else if (!isJumpPressed && g_wasJumpPressed) {
                    playLayer->releaseButton((int)PlayerButton::Jump, true);
                }
                g_wasJumpPressed = isJumpPressed;

                float moveX = OpenXRApp::get().getMoveX();
                bool isLeftPressed = moveX < -0.5f;
                bool isRightPressed = moveX > 0.5f;

                if (isLeftPressed && !g_wasLeftPressed) {
                    playLayer->pushButton((int)PlayerButton::Left, true);
                } else if (!isLeftPressed && g_wasLeftPressed) {
                    playLayer->releaseButton((int)PlayerButton::Left, true);
                }
                g_wasLeftPressed = isLeftPressed;

                if (isRightPressed && !g_wasRightPressed) {
                    playLayer->pushButton((int)PlayerButton::Right, true);
                } else if (!isRightPressed && g_wasRightPressed) {
                    playLayer->releaseButton((int)PlayerButton::Right, true);
                }
                g_wasRightPressed = isRightPressed;
            }
        }
        
        CCDirector::drawScene();
    }
};

$on_mod(Loaded) {
    log::info("GDVR Loaded - Waiting for CCDirector to initialize OpenXR.");
}
