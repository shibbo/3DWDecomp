#pragma once

#include "Library/LiveActor/LiveActor.hpp"
#include <math/seadMatrix.h>

class SwitchRotateParts : public al::LiveActor {
public:
    SwitchRotateParts(const char* pName);
    virtual ~SwitchRotateParts();
    virtual void init(const al::ActorInitInfo& rInfo);
    virtual void control();

    void initFromWatcher(const sead::Matrix34f* pMtx, const sead::Matrix34f& rInverseMtx);
    void requestStop();
    void requestRotate();
    void exeWait();
    void exeStop();
    void exeRotate();

    const sead::Matrix34f* mWatcherMtx = nullptr; // 0x148
    sead::Matrix34f mLocalMtx = sead::Matrix34f::ident; // 0x150
};
