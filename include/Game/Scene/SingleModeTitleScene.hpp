#pragma once
#include "Library/Scene/Scene.hpp"
namespace sead {
class Viewport;
}
class StageWipeKeeper;
class GameDataHolder;
class SingleModeTitle;
class WindowConfirm;
class RCS_SaveDataLayout;
class SingleModeTitleScene : public al::Scene {
public:
    explicit SingleModeTitleScene(StageWipeKeeper* wipe);
    ~SingleModeTitleScene() override;
    void init(const al::SceneInitInfo& info) override;
    void appear() override;
    void kill() override;
    void control() override;
    void drawMain_() const override;
    bool isReadyShow() const;
    bool isReturnToTopMenu() const;
    void exeLoad();
    void exeAppearWait();
    void exeTitle();
    void exeNewGameWarning();
    void exeFileSelect();
    void exeDeleteFile();
    void exeWipeClose();
    void exeKill();

private:
    GameDataHolder* mGameDataHolder = nullptr;
    StageWipeKeeper* mWipe;
    sead::Viewport* mViewport = nullptr;
    SingleModeTitle* mTitle = nullptr;
    WindowConfirm* mConfirm = nullptr;
    RCS_SaveDataLayout* mFileSelect = nullptr;
};
static_assert(sizeof(SingleModeTitleScene) == 0x118);
