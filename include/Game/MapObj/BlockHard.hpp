#pragma once
#include "Library/LiveActor/LiveActor.hpp"
namespace al { class BreakModel; class MtxConnector; }
class BlockHard : public al::LiveActor {
public:
    explicit BlockHard(const char*);
    ~BlockHard() override;
    void init(const al::ActorInitInfo&) override;
    void initAfterPlacement() override;
    void appear() override;
    void kill() override;
    void updateLinkedTrans(const sead::Vector3f&) override;
    void control() override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    void exeWait();
    void exeReaction();
    void exeBreakStart();
private:
    al::BreakModel* mBreakModel = nullptr;
    al::HitSensor* mAttacker = nullptr;
    al::MtxConnector* mConnector = nullptr;
    bool mSaveIfBroken = false;
    int mSaveId = 0;
};
static_assert(sizeof(BlockHard) == 0x168);
