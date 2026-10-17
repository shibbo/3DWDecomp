#pragma once

#include "Library/Scene/Scene.hpp"

namespace sead { class Viewport; }
namespace al { class WipeSimple; class SimpleLayoutAppearWaitEnd; }
class GameDataHolder;

class BootScene : public al::Scene {
public:
    BootScene();
    ~BootScene() override;
    void init(const al::SceneInitInfo& rInfo) override;
    void appear() override;
    void control() override;
    void drawMain_() const override;
    void drawSub_() const override;
    bool tryEnd();
    void updateGuide();
    void exeInit();
    void exeInitError();
    void exeLoadSaveData();
    void exeWaitLoadDoneResource();
    void exeLoadEnd();

private:
    GameDataHolder* mGameData = nullptr;
    sead::Viewport* mMainViewport = nullptr;
    sead::Viewport* mSubViewport = nullptr;
    bool mIsSaveInit = true;
    bool mIsSaveDataLoaded = false;
    al::WipeSimple* mWipe = nullptr;
    al::SimpleLayoutAppearWaitEnd* mWindowBoot = nullptr;
    al::SimpleLayoutAppearWaitEnd* mControllerGuide = nullptr;
    s32 mGuideFrames = 0;
    s32 mFrames = 0;
};
static_assert(sizeof(BootScene) == 0x128);
