#pragma once

#include <math/seadMatrix.h>
#include <math/seadQuat.h>
#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"

class IUsePlayerPuppet;

/**
 * @brief Test object: a long "?" box up to three players can grab (center, right and left) and
 * carry around together. The box behaves like a simple rigid body: every player pushes it with the
 * stick, it tips over around its center of mass and spits coins while it travels.
 */
class TestBoxQuestionLongRot : public al::LiveActor {
public:
    TestBoxQuestionLongRot(const char* pName);

    void init(const al::ActorInitInfo& rInfo) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    void control() override;

    u32 findBindIndex(const al::HitSensor* pSensor) const;
    void initRigidBody();
    void exeWait();
    void exeBindWait();
    void invalidateBindSensor(u32 index);
    bool isNoBinded() const;
    bool isAllBinded() const;
    void exeBindMove();
    bool checkBindEnd();
    void resetJumpPow();
    void updateRigidBody();
    void applyPose();
    void applyCollision();
    void updateCoin();
    u32 calcJumpPow();
    void exeBindJump();
    void constrainVelocity();
    void exeKill();
    bool updatePuppetCollider();
    bool isAllJump() const;

private:
    void releasePuppet(u32 index);
    void integrate();

    IUsePlayerPuppet* mPuppets[3];                        // 0x148
    bool mIsFirstBind[3];                                 // 0x160
    s32 mInvalidSensorTimer[3];                           // 0x164
    s32 mJumpTimer[3];                                    // 0x170
    bool mIsOnGround[3];                                  // 0x17c
    s32 mBindNum = 0;                                     // 0x180
    sead::Vector3f mCoinTrans = {0.0f, 0.0f, 0.0f};       // 0x184
    sead::Vector3f mCenter = {0.0f, 0.0f, 0.0f};          // 0x190
    sead::Vector3f mVelocity = {0.0f, 0.0f, 0.0f};        // 0x19c
    sead::Quatf mQuat = sead::Quatf::unit;                // 0x1a8
    sead::Matrix33f mInertia = sead::Matrix33f::ident;    // 0x1b8
    sead::Vector3f mAngularVelocity;                      // 0x1dc
    sead::Vector3f mCenterOffset;                         // 0x1e8
    bool mIsCollided;                                     // 0x1f4
};

static_assert(sizeof(TestBoxQuestionLongRot) == 0x1f8);
