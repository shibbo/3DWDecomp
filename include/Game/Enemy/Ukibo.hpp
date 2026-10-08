#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"

namespace al {
class AreaObj;
class Nerve;
}  // namespace al

class ActorMicRumbler;
class ActorStateSupportFreeze;
class EnemyStateBlowDown;
class TargetFinder;

/**
 * @brief Goomba riding a swim ring: floats around on the surface of a water area and chases
 * players that come near. The swim ring is a separate actor that follows the Goomba.
 */
class Ukibo : public al::LiveActor {
public:
    explicit Ukibo(const char* pName);

    ~Ukibo() override = default;
    void init(const al::ActorInitInfo& rInfo) override;
    void startActionWithFloat(const char* pActionName);
    void initAfterPlacement() override;
    void control() override;
    void appear() override;
    void reappear() override;
    void updateMoveSurface();
    void makeActorDead() override;
    void killComplete(bool isNoReaction) override;
    void killBySwitch();
    void kill() override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;

    void exeSleepStart();
    void exeWait();
    void exeMove();
    void exeFind();
    void exeChase();
    void exeChaseImmediate();
    void exeChaseEnd();
    void exePressDown();
    void exeBlowDown();
    void exePressDownHipDrop();
    void exeSupportFreeze();

private:
    /** @brief Center of the move area (middle of its bounding box). */
    sead::Vector3f getMoveAreaCenter() const { return (mMoveAreaMin + mMoveAreaMax) * 0.5f; }

    void decideDestination();
    bool isInMoveArea() const;
    void addChaseVelocity();

    al::LiveActor* mFloatActor = nullptr;                 // 0x148
    al::AreaObj* mWaterArea = nullptr;                    // 0x150
    al::LiveActor* mTarget = nullptr;                     // 0x158
    TargetFinder* mTargetFinder = nullptr;                // 0x160
    EnemyStateBlowDown* mStateBlowDown = nullptr;         // 0x168
    ActorStateSupportFreeze* mStateSupportFreeze = nullptr;  // 0x170
    sead::Vector3f mMoveAreaMin = {0.0f, 0.0f, 0.0f};     // 0x178
    sead::Vector3f mMoveAreaMax = {0.0f, 0.0f, 0.0f};     // 0x184
    sead::Vector3f mDestination = {0.0f, 0.0f, 0.0f};     // 0x190
    f32 mOffsetY = 0.0f;                                  // 0x19C
    s32 mLostTargetTime = 0;                              // 0x1A0
    s32 mTargetAboveTime = 0;                             // 0x1A4
    f32 mMoveAngle = 0.0f;                                // 0x1A8
    ActorMicRumbler* mMicRumbler = nullptr;               // 0x1B0
    const al::Nerve* mNerveBeforeFreeze = nullptr;        // 0x1B8
    f32 mFloatSklAnimFrameRate = 0.0f;                    // 0x1C0
    sead::Vector3f mInitFront = sead::Vector3f::ez;       // 0x1C4
};

static_assert(sizeof(Ukibo) == 0x1D0);
