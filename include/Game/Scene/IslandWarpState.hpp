#pragma once
#include "Demo/StageStartEventBase.hpp"
class SingleModeScene;
class BindPuppeteerGroup;
class IslandWarpState : public StageStartEventBase {
public:
    explicit IslandWarpState(SingleModeScene* scene);
    void init(const al::ActorInitInfo& info) override;
    void appear() override;
    void kill() override;
    void setIslandWarpDest(s32 islandId);
    bool receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender,
                    al::HitSensor* receiver) override;
    s64 getEventType() const override { return 4; }
    void startDemo() override { appear(); }
    void endDemo() override { kill(); }
    bool isEndDemo() const override;
    bool isEnableOpenStartWipe() const override;
    void exeBindWait();
    void exeDoWarp();
    void exeFadeWait();

private:
    BindPuppeteerGroup* mPuppeteers;
    SingleModeScene* mScene;
    s32 mIslandId = -1;
    sead::Vector3f mDestination = sead::Vector3f::zero;
    sead::Vector3f mFront = sead::Vector3f::ez;
};
static_assert(sizeof(IslandWarpState) == 0x178);
