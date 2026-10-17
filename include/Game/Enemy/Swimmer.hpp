#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"

namespace al {
class AreaObj;
class Nerve;
}  // namespace al

class ActorJointLookController;
class ActorStateSupportFreeze;
class EnemyStateBlowDown;
class TargetFinder;

/**
 * @brief Fish enemy (Cheep Cheep style) that swims around inside a water area and chases the
 * player when they come close.
 */
class Swimmer : public al::LiveActor {
public:
    explicit Swimmer(const char* pName);

    void init(const al::ActorInitInfo& rInfo) override;
    void initAfterPlacement() override;
    void kill() override;
    void control() override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;

    void exeMove();
    void updateMoveSurface();
    bool isInTerritory() const;
    void setSwimmerNerve(const al::Nerve* pNerve);
    bool isGreaterEqualStep(s32 step);
    void exeLockOn();
    void exeFind();
    void exeChase();
    void exeChaseEnd();
    void exeTurn();
    void exeSupportFreeze();
    void exePressDown();
    void exeBlowDown();

private:
    /** @brief Gets the center of the territory box. */
    sead::Vector3f getTerritoryCenter() const {
        return (mTerritoryMin + mTerritoryMax) * 0.5f;
    }

    /** @brief Gets half the size of the territory box. */
    sead::Vector3f getTerritoryHalfSize() const {
        return (mTerritoryMax - mTerritoryMin) * 0.5f;
    }

    inline void resetTargetPos();

    al::AreaObj* mWaterArea = nullptr;
    al::LiveActor* mChaseTarget = nullptr;
    TargetFinder* mTargetFinder = nullptr;
    EnemyStateBlowDown* mStateBlowDown = nullptr;
    EnemyStateBlowDown* mStateBlowDownBobsled = nullptr;
    ActorStateSupportFreeze* mStateSupportFreeze;
    ActorJointLookController* mJointLookController;
    sead::Vector3f mTerritoryMin = {0.0f, 0.0f, 0.0f};
    sead::Vector3f mTerritoryMax = {0.0f, 0.0f, 0.0f};
    sead::Vector3f mTargetPos = {0.0f, 0.0f, 0.0f};
    f32 mSurfaceOffsetY = 0.0f;
    s32 mLostTargetStep = 0;
    s32 mCloseBelowStep = 0;
    s32 mWallHitStep = 0;
    bool mIsValidSlowdownArea = false;
    const al::Nerve* mResumeNerve = nullptr;
    s32 mResumeStep = 0;
};

static_assert(sizeof(Swimmer) == 0x1C8);
