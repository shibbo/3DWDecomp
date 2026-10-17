#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class BlockStateItem;
class BlockQuestionChameleon;
class BlockTransparent : public al::LiveActor {
public:
    explicit BlockTransparent(const char*);
    void init(const al::ActorInitInfo&) override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    void respawn() override;
    void onConnectRailBlock();
    bool isLong() const;
    void exeHide();
    void exeAppearItem();
    void exeEmpty();
private:
    bool mConnectedRail = false;
    BlockStateItem* mState = nullptr;
    BlockQuestionChameleon* mChameleon = nullptr;
    int mPunchCooldown = 0;
    sead::Vector3f mClippingCenter = sead::Vector3f::zero;
};
static_assert(sizeof(BlockTransparent) == 0x168);
