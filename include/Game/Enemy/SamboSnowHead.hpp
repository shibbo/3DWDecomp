#pragma once

#include <basis/seadTypes.h>
#include <math/seadMatrix.h>
#include <math/seadQuat.h>
#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"

namespace al {
class AreaObjGroup;
class HitSensor;
class SensorMsg;
class ScreenPointer;
class ScreenPointTarget;
}  // namespace al
class ActorStateSupportFreeze;
class BallSnow;
class SamboSnowBody;
class SamboSnowHat;

/** @brief Snow Pokey head: owns and steers the stack of snow bodies below it. */
class SamboSnowHead : public al::LiveActor {
public:
    explicit SamboSnowHead(const char* pName);
    /** @brief Releases the snow Pokey head actor. */
    ~SamboSnowHead() override = default;
    void init(const al::ActorInitInfo& rInfo) override;
    void control() override;
    void requestBodyBlowDown();
    s32 calcAliveBodyNum();
    bool isBodyDeadOrAttacked(s32 index);
    bool isEnableSupportFreeze() const;
    void appear() override;
    void reappear() override;
    void killComplete(bool isForce) override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool isDown() const;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    void tryBlowHat(const sead::Vector3f& rDir, bool isRotate);
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;
    void exeWait();
    void setVelocityZeroHeadAndBody();
    void startBodyAction(const char* pActionName, s32 frameInterval);
    bool isTargetInChaseArea() const;
    void exeSearch();
    void exeFind();
    void exeChase();
    bool checkForwardObstacle();
    void calcEffectMtx();
    void endChase();
    void exeGiveUpChase();
    void exeAttack();
    void exeBodyAttacked();
    void exeLand();
    void exeTrample();
    void exeHipDrop();
    void exeBlowDown();
    void exeFireDown();
    void exeTouchDown();
    void exeSupportFreeze();
    void requestBodySupportFreeze(const al::LiveActor* pActor);
    void endSupportFreeze();
    void requestBodyEndSupportFreeze();

private:
    BallSnow* mBallSnow = nullptr;                                   // 0x148
    al::LiveActor* mTarget = nullptr;                                // 0x150
    SamboSnowBody** mBodies = nullptr;                               // 0x158
    SamboSnowHat* mHat = nullptr;                                    // 0x160
    s32 mBodyNum = 4;                                                // 0x168
    f32 mGroundY = 0.0f;                                             // 0x16c
    sead::Matrix34f mEffectMtx = sead::Matrix34f::ident;             // 0x170
    bool mIsStacked = true;                                          // 0x1a0
    bool mIsRequestAttack = false;                                   // 0x1a1
    bool mIsRequestBlowDown = false;                                 // 0x1a2
    al::LiveActor* mSupportFreezeActor = nullptr;                    // 0x1a8
    s32 mAttackedBodyIndex = 0;                                      // 0x1b0
    f32 mLandY = 0.0f;                                               // 0x1b4
    bool mIsGroundSnow = true;                                       // 0x1b8
    sead::Vector3f mPushSensorPos = sead::Vector3f::zero;            // 0x1bc
    bool mIsHatBlown = false;                                        // 0x1c8
    bool mIsStopSePlayed = false;                                    // 0x1c9
    al::AreaObjGroup* mChaseArea = nullptr;                          // 0x1d0
    ActorStateSupportFreeze* mStateSupportFreeze = nullptr;          // 0x1d8
    sead::Vector3f mBlowDownVelocity = sead::Vector3f::zero;         // 0x1e0
    sead::Quatf mInitQuat;                                           // 0x1ec
    bool mIsSingleMode = false;                                      // 0x1fc
};
static_assert(sizeof(SamboSnowHead) == 0x200);
