#pragma once

#include "Library/Message/IUseMessageSystem.hpp"
#include "Library/Nerve/NerveExecutor.hpp"
#include "Library/Scene/ISceneObj.hpp"

namespace al {
class CameraDirector;
class LayoutInitInfo;
}  // namespace al

class GuideGameWindow;

/**
 * @brief Scene object showing the camera mode change layout.
 * @note Minimal declaration: only what the reconstructed code uses is declared so far.
 */
class CameraChangeLayout : public al::NerveExecutor,
                           public al::IUseMessageSystem,
                           public al::ISceneObj {
public:
    CameraChangeLayout(const al::LayoutInitInfo& rInfo, al::CameraDirector* pCameraDirector,
                       GuideGameWindow* pGuideGameWindow, bool isKinopioBrigade);

    const al::MessageSystem* getMessageSystem() const override;
    const char* getSceneObjName() const override;
    void update();
    void disappearCameraChangeLayout();

private:
    u8 _20[0x50 - 0x20];
};

static_assert(sizeof(CameraChangeLayout) == 0x50);
