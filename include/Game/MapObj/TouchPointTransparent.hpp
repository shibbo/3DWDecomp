#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class TouchPointTransparent : public al::LiveActor {
public:
    TouchPointTransparent(const char* pName);
    virtual ~TouchPointTransparent();
    virtual void init(const al::ActorInitInfo& rInfo);
    virtual void appear();

    void startTransparentMode();
    void setGyroDisappearAlpha(float rate);
    void exeTransparent();
    void exeDisappear();

    void setSlowDisappear(bool isSlow) { mIsSlowDisappear = isSlow; }

    bool mIsSlowDisappear = false; // 0x144
    float mAlpha = 0.75f; // 0x148
};
