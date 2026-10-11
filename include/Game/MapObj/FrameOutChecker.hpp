#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include <container/seadObjArray.h>
class FrameOutChecker : public al::LiveActor {
public:
    explicit FrameOutChecker(const char*);
    ~FrameOutChecker() override;
    void init(const al::ActorInitInfo&) override;
    void appear() override;
    void kill() override;
    void control() override;
private:
    sead::ObjArray<int> mFrameOutCounts;
    int mScrollDirection = 0;
    float mWidthMargin = 100.0f;
    float mHeightMargin = 100.0f;
    float mDepthMargin = 100.0f;
    bool mIsInvalidBindKill = false;
};
static_assert(sizeof(FrameOutChecker) == 0x180);
