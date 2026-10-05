#pragma once

#include "Library/LiveActor/LiveActor.hpp"

namespace al { class KeyPoseKeeper; }

class SwitchBlock : public al::LiveActor {
public:
    SwitchBlock(const char* pName);
    virtual ~SwitchBlock();
    virtual void init(const al::ActorInitInfo& rInfo);

    void exeWait();
    void exeMove();
    void startMove();
    bool isMove() const;

    al::KeyPoseKeeper* mKeyPoseKeeper = nullptr; // 0x148
    sead::Vector3f mClippingCenter = {0.0f, 0.0f, 0.0f}; // 0x150
    bool _15c = false;
};
