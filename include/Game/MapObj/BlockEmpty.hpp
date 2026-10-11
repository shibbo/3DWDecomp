#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class BlockEmpty : public al::LiveActor {
public:
    BlockEmpty(const char*, const char* pOther = "BlockEmpty");

    virtual ~BlockEmpty();
    virtual void init(const al::ActorInitInfo&);
    virtual void updateLinkedTrans(const sead::Vector3f&);
    virtual bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*);

    void exeWait();
    void onConnectRailBlock();

    bool mIsConnectedRailBlock = false;
    bool mPreventGiantBreak = false;
    s32 mHitCooldown = 0;
    sead::Vector3f mClippingOffset = sead::Vector3f::zero;
    const char* mArchiveName = nullptr;
};