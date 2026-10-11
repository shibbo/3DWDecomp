#pragma once

#include <math/seadMatrix.h>
#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"

/** @brief Boomerang thrown by a Boomerang Bro. */
class BoomerangBrosBoomerang : public al::LiveActor {
public:
    explicit BoomerangBrosBoomerang(const char* pName);

    /**
     * @brief Gets the name the Boomerang Bro gives its boomerangs.
     * @return Actor name of the boomerang.
     */
    static const char* getWeaponName() { return "ブーメラン"; }

    void init(const al::ActorInitInfo& rInfo) override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;
    void attach(const sead::Matrix34f* pMtx, const sead::Vector3f& rTrans,
                const sead::Vector3f& rRotate);
    void shoot(const sead::Vector3f& rVelocity);
    void caught(const sead::Matrix34f* pMtx, const sead::Vector3f& rTrans,
                const sead::Vector3f& rRotate);
    void exeWait();
    void exeMoveStart();
    void exeMoveBrake();
    void exeMoveStay();
    void exeMoveBack();
    void exeCatch();
    bool isMoveBack();

private:
    bool isFlying();
    void updateAttachPose();
    void stopForKill();
    void killWithHitReaction(const char* pReaction, bool isAppearItem);
    void killByHitReaction(const char* pReaction, bool isAppearItem);

    const sead::Matrix34f* mHostMtx = nullptr;     ///< Joint matrix of the holder.
    sead::Vector3f mTrans = sead::Vector3f::zero;  ///< Offset from the holder matrix.
    sead::Vector3f mRotate = sead::Vector3f::zero; ///< Rotation (degrees) relative to the holder.
    sead::Vector3f mFlyDir = {0.0f, 0.0f, 0.0f};   ///< Normalized throw direction.
    f32 mFlySpeed = 0.0f;                          ///< Throw speed.
    s32 mFlyStep = 0;                              ///< Steps flown before braking.
};

static_assert(sizeof(BoomerangBrosBoomerang) == 0x180);
