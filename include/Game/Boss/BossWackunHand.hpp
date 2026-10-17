#pragma once

#include <basis/seadTypes.h>

#include "Library/LiveActor/LiveActor.hpp"

class BossWackun;

/** @brief The hand that pushes BossWackun's body . */
class BossWackunHand : public al::LiveActor {
public:
    explicit BossWackunHand(BossWackun* pBoss);

    ~BossWackunHand() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void exeControled();
    void exeWait();
    void exeMoveLeft();
    void exeMoveRight();
    void startWait();
    void startMove(s32 targetIndex);
    bool isEndMove() const;
    void startControled();

private:
    void updateLocalRotation();
    void updateHostPose();
    BossWackun* mBoss;
    sead::Quatf mLocalQuat = sead::Quatf::unit;
    s32 mTargetIndex = 0;
    s32 mCurrentIndex = 0;
};
static_assert(sizeof(BossWackunHand) == 0x168);
