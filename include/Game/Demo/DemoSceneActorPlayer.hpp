#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class DemoSceneActorHolder;
class DemoActionList;
class DemoScenePlayerModel;
class PlayerRetargettingSelector;

/** @brief Connects a placed demo player to a shared or private player model. */
class DemoSceneActorPlayer : public al::LiveActor {
public:
    DemoSceneActorPlayer(const char* pName, DemoSceneActorHolder* pHolder,
                         PlayerRetargettingSelector* pSelector);
    void init(const al::ActorInitInfo& rInfo) override;
    void initAfterPlacement() override;
    virtual void initDemoSceneActor(const al::ActorInitInfo& rInfo,
        const al::ActorInitInfo& rDemoInfo, const sead::Matrix34f* pMtx);
    void setPlacementBaseMtx(const sead::Matrix34f* pMtx);
    void startDemo(const sead::Matrix34f& rMtx);
    void startAction(int index);
    bool tryStartActionByName(const char* pName);
    bool isModelActionEnd() const;
    void endDemo();
    void killAllEffect();
    void exePlay();
    void exeWait();

private:
    friend class DemoSceneActorHolder;
    DemoSceneActorHolder* mHolder;
    DemoActionList* mActions = nullptr;
    sead::Matrix34f mPlacementMtx;
    sead::Matrix34f mLocalMtx;
    PlayerRetargettingSelector* mSelector;
    DemoScenePlayerModel* mModel;
    int mCharacter = -1;
    int mFigure = 0;
    bool mSingleMode = false;
    bool mIgnoreSingleModeCharID = false;
    bool mUniqueModel = false;
    bool mShowShadow = false;
};

static_assert(sizeof(DemoSceneActorPlayer) == 0x1d8);
