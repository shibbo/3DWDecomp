#pragma once
#include "Library/Layout/LayoutActor.hpp"
namespace al { class LayoutInitInfo; }
class GameDataHolder;

class LoadingLayout : public al::LayoutActor {
public:
    LoadingLayout(const al::LayoutInitInfo& rInfo, GameDataHolder* pGameData);
    void startDisplay(bool isDefaultIcon, bool isDelayAppear);
    void selectRandomIcon();
    void requestEnd();
    void exeAppear();
    void exeWaitAppear();
    void exeLoop();
    void exeEnd();
    s32 getNumIconsAvailable();
    bool isDisplay() const { return isAlive(); }
private:
    GameDataHolder* mpGameData;
    bool mEndRequested;
    s32 mIcon;
};

static_assert(sizeof(LoadingLayout) == 0x138);
