#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"

namespace al {
class KeyPoseKeeper;
class RumbleCalculatorCosMultLinear;
}  // namespace al

class CrawlerWalkState;
class EnemyAttachItem;
class EnemyStateBlowDown;

/**
 * @brief Caterpillar enemy (Crawler) that walks along walls and floors. Comes in a normal, a
 * needle (CrawlerNeedle) and a big (CrawlerBig) variant and can be launched by a generator.
 */
class Crawler : public al::LiveActor {
public:
    explicit Crawler(const char* pName);

    void init(const al::ActorInitInfo& rInfo) override;
    void killBySwitch();
    void appear() override;
    void control() override;
    void kill() override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    void startTrampleReaction();
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;
    void startClipped() override;
    void endClipped() override;
    void resetParam(const sead::Vector3f& rFront, const sead::Vector3f& rUp,
                    const sead::Vector3f& rTrans);
    void setWalkSpeed(f32 speed);
    void startLaunch();
    void startWalk();
    bool isDamaged() const;

    void exeStop();
    void exeWalkStandby();
    void exeLaunchStandby();
    void exeLaunch();
    void exeWalk();
    void exePressDown();
    void exeBlowDown();

    /**
     * @brief Sets the step after which a launched crawler dies while walking.
     * @param step Kill step.
     */
    void setKillStep(s32 step) { mKillStep = step; }

    /**
     * @brief Makes the crawler always carry a coin.
     * @param isCoinItem Whether the crawler carries a coin.
     */
    void setIsCoinItem(bool isCoinItem) { mIsCoinItem = isCoinItem; }

private:
    bool mIsLauncher = false;
    bool mIsNeedle = false;
    bool mIsBig = false;
    bool mIsCoinItem = false;
    bool mIsTouchItemAppeared = false;
    bool mIsEnableHitInLaunch = true;
    s32 mWalkDelay = 0;
    s32 mKillStep = 0;
    s32 mDamageTime = 0;
    s32 mTouchCount = 0;
    sead::Vector3f mDisplayOffset = {0.0f, 0.0f, 0.0f};
    EnemyStateBlowDown* mStateBlowDown = nullptr;
    CrawlerWalkState* mStateWalk = nullptr;
    al::RumbleCalculatorCosMultLinear* mRumble = nullptr;
    al::KeyPoseKeeper* mKeyPoseKeeper = nullptr;
    EnemyAttachItem* mAttachItem = nullptr;
    f32 mShadowLength = 0.0f;
};

static_assert(sizeof(Crawler) == 0x198);
