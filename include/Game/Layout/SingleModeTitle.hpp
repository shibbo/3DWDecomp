#pragma once
#include "Library/Layout/LayoutActor.hpp"
namespace al { class LayoutInitInfo; }
class GameDataHolder;
class ButtonGroup;
/** @brief Bowser's Fury title menu for starting or resuming a save file. */
class SingleModeTitle : public al::LayoutActor {
public:
    SingleModeTitle(const al::LayoutInitInfo& rInfo, const GameDataHolder* pGameData);
    void appear() override;
    void returnFromMenu(const char* pButtonName);
    void exeAppear();
    void exeWait();
    void exeDecide();
    void exeEnd();
    bool isStartNewGame() const;
    bool isEnd();
    bool isReturnToTopMenu() const { return mIsBack; }
private:
    const GameDataHolder* mGameData;
    bool mIsBack = false;
    ButtonGroup* mButtons = nullptr;
    float mStartLoopFrame = 0.0f;
    int mNewFileId = 0;
};
static_assert(sizeof(SingleModeTitle) == 0x148);
