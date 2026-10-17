#pragma once

#include "Library/Scene/Scene.hpp"

namespace sead {
class Viewport;
}

class DrcCameraScene : public al::Scene {
public:
    DrcCameraScene();
    ~DrcCameraScene() override;

    void init(const al::SceneInitInfo& rInfo) override;
    void appear() override;
    void kill() override;
    void control() override;
    void drawMain_() const override;
    void drawSub_() const override;

    void initDrcCamera();
    void termDrcCamera();
    void openDrcCamera();
    void closeDrcCamera();
    void updateDrcCamera();

private:
    sead::Viewport* mMainViewport = nullptr;
    sead::Viewport* mSubViewport = nullptr;
    void* mDrcCamera = nullptr;
    s32 mCameraState = 0;
};

static_assert(sizeof(DrcCameraScene) == 0x108);
