#pragma once
#include "Library/LiveActor/LiveActor.hpp"
namespace al { class MtxConnector; }
class DashPanelSeTriggerChecker;
class DashPanel : public al::LiveActor {
public:
    explicit DashPanel(const char*);
    void init(const al::ActorInitInfo&) override;
    void initNoPlacement(const al::ActorInitInfo&);
    void initAfterPlacement() override;
    void reappear() override;
    void attackSensor(al::HitSensor*, al::HitSensor*) override;
    void control() override;
private:
    al::MtxConnector* mConnector = nullptr;
    int mDashFrames = 180;
    int mActionCooldown = 0;
    bool mNoConnectCollision = false;
    bool mShowShadow = true;
    bool mHasAppearSwitch = false;
    DashPanelSeTriggerChecker* mSeTrigger = nullptr;
    sead::Matrix34f mEffectMtx = sead::Matrix34f::ident;
    bool mModifiedSpeed = false;
};
static_assert(sizeof(DashPanel) == 0x1a0);
