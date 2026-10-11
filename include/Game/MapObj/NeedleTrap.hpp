#pragma once
#include "Library/LiveActor/LiveActor.hpp"
namespace al { class CollisionObj; class MtxConnector; }
class NeedleTrap : public al::LiveActor {
public:
    explicit NeedleTrap(const char*);
    ~NeedleTrap() override;
    void init(const al::ActorInitInfo&) override;
    void initAfterPlacement() override;
    void control() override;
    void exeStop();
    void exeWait();
    void exeSign();
    void exeAttack();
    void exeEnd();
private:
    bool mHasKeepOnSwitch = false;
    bool mHasStartSwitch = false;
    al::CollisionObj* mNeedleCollision = nullptr;
    al::MtxConnector* mConnector = nullptr;
    int mWaitTime = 120;
    int mAttackTime = 120;
    int mStandbyTime = 120;
};
static_assert(sizeof(NeedleTrap) == 0x168);
