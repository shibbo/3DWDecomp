#pragma once
#include "Library/LiveActor/LiveActor.hpp"

class RedBlueBlock : public al::LiveActor {
public:
    RedBlueBlock(const char* pName);
    ~RedBlueBlock() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void exeWait();
    void exeFlip();

private:
    sead::Vector3f mBasePosition = sead::Vector3f::zero;
    int mMoveAxis = 0;
    float mMoveDistance = 100.0f;
    int mMoveStep = 0;
    bool mReverseAxis = false;
    bool mMovingOut = true;
};
