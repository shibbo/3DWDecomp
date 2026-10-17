#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class DemoSceneActorHolder;
class DemoActionList;
class DemoPlayerLocator;
class DemoScenePlayerModel;
class IDemoScenePlayerInfo;
class PlayerRetargettingSelector;

/** @brief Coordinates the active players and their models during a demo. */
class DemoScenePlayer : public al::LiveActor {
public:
    DemoScenePlayer(const char* pName, DemoSceneActorHolder* pHolder, PlayerRetargettingSelector* pSelector);
    void init(const al::ActorInitInfo& rInfo) override;
    void initAfterPlacement() override;
    void control() override;
    virtual void initDemoSceneActor(const al::ActorInitInfo& rInfo, const al::ActorInitInfo& rDemoInfo,
        const sead::Matrix34f* pMtx, bool onlySuper);
    void startDemo(const sead::Matrix34f& rMtx);
    void endDemo();
    void startAction(int index);
    void setDemoScenePlayerInfo(IDemoScenePlayerInfo* pInfo);
    void setPlacementBaseMtx(const sead::Matrix34f* pMtx);
    void killAllEffect();
    al::LiveActor* getPlayerModel(int index);
    al::LiveActor* tryGetPlayerModel(int index) const;
    void exeWait();
    void exePlay();

private:
    DemoSceneActorHolder* mHolder;
    DemoActionList* mActions = nullptr;
    sead::Matrix34f mPlacementMtx;
    sead::Matrix34f mLocalMtx;
    IDemoScenePlayerInfo* mPlayerInfo = nullptr;
    PlayerRetargettingSelector* mSelector;
    DemoPlayerLocator* mLocator = nullptr;
    DemoScenePlayerModel** mModels = nullptr;
    int mPlayerCount = 0;
    int* mDelayFrames;
    int mSeMaterialCode = 0;
    bool mOnlySuper = false;
};

static_assert(sizeof(DemoScenePlayer) == 0x1f0);
