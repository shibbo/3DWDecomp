#pragma once

#include <container/seadPtrArray.h>
#include <math/seadQuat.h>
#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"
#include "Library/Play/Placement/PlacementInfo.hpp"
#include "MapObj/DisasterModeController.hpp"

namespace al {
class JointLocalAxisRotator;
class MtxConnector;
}  // namespace al

class DummyCameraTarget;
class IUsePlayerPuppet;
class PlayerBindEndParam;

/**
 * @brief The springy pole the player climbs and bends to fling itself away (also used as the
 * flag pole raised next to a lighthouse once all of its Cat Shines are collected).
 */
class FlingPole : public al::LiveActor, public DisasterModeStateListener {
public:
    /// Placement parameters of a pole.
    struct Param {
        const char* mComment = nullptr;       // 0x0
        bool mIsDisabledPR = false;           // 0x8
        bool mIsDisablePlessieChase = false;  // 0x9
        f32 mFlingPower = 1.0f;               // 0xc
        s32 mFlingDashTime = 240;             // 0x10
        bool mIsConnectToCollision = false;   // 0x14
        f32 mCameraDist = 1.0f;               // 0x18
    };

    static_assert(sizeof(Param) == 0x20);

    /// A linked point the flung player can be pulled towards.
    struct SnapPoint {
        /**
         * @brief Read a snap point from its placement.
         * @param info Placement of the snap point.
         * @param index Index of the snap point in the pole's links.
         */
        SnapPoint(al::PlacementInfo info, s32 index);

        sead::Vector3f mTrans;                             // 0x0
        f32 mRadius;                                       // 0xc
        f32 mStrength;                                     // 0x10
        s32 mIndex;                                        // 0x14
        sead::Vector3f mHitNormal = sead::Vector3f::zero;  // 0x18
        sead::Vector3f mHitPos = sead::Vector3f::zero;     // 0x24
        sead::Vector3f mEnterPos = sead::Vector3f::zero;   // 0x30
        bool mIsHit = false;                               // 0x3c
    };

    static_assert(sizeof(SnapPoint) == 0x40);

    /// Which joycon guide message is shown while the player stands on top.
    enum class GuideType : s32 { None = 0, DualJoycons = 1, SingleJoycons = 2 };

    FlingPole(const char* pName, bool isLighthouseFlag = false);

    void init(const al::ActorInitInfo& rInfo) override;
    void initAfterPlacement() override;
    void updateLinkedTrans(const sead::Vector3f& rTrans) override;
    void startClipped() override;
    void endClipped() override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    void control() override;
    void onDisasterModeStateChange(DisasterModeController::State state) override;
    virtual void initJointKeeper();

    void tryShakePole(f32 angle);
    bool isBindPole() const;
    void setDisabled(bool isDisabled);
    bool isIdle() const;
    bool canBind() const;
    void calcNearestPos();
    sead::Vector3f calcTop() const;
    void setPuppetOnTop();
    void turnOffCameraTarget();
    bool isOnTop() const;
    bool canJump() const;
    bool calcPuppetTrans(sead::Vector3f* pTrans) const;
    void DisasterModeAnimUpdate();
    void showFlag(bool isShow);
    void startNrvWait();
    void exeWait();
    sead::Vector3f calcBottom() const;
    void exeBindPoleWait();
    void controlSpring(f32 damping);
    void exeBindPoleClimb();
    void exeBindPoleDown();
    bool isOnBottom() const;
    void exeBindTop();
    void turnOnCameraTarget();
    bool findBestInRangeSnapPoint();
    void exeShoot();
    void shoot(f32 speed, f32 upY);
    void exeJump();
    void exeDamping();
    sead::Vector3f calcJointAt(s32 index) const;

private:
    f32 calcWobbleSeRate() const;
    void holdWobbleSe();
    void validateBindSensors();

    sead::PtrArray<al::JointLocalAxisRotator> mStickRotators;  // 0x150
    f32 mStickXAngle = 0.0f;                                   // 0x160
    sead::Vector3f mRotateAxis = sead::Vector3f::ez;           // 0x164
    f32 mRotateAngle = 0.0f;                                   // 0x170
    void* _178 = nullptr;
    void* _180 = nullptr;
    Param* mParam = new Param();                                // 0x188
    IUsePlayerPuppet* mPuppet = nullptr;                        // 0x190
    PlayerBindEndParam* mBindEndParam;                          // 0x198
    f32 _1a0 = 1.0f;
    f32 mClimbSpeed = 0.0f;                                     // 0x1a4
    sead::Vector3f mCameraPos = {0.0f, 0.0f, 0.0f};             // 0x1a8
    sead::Vector3f mCameraAt = {0.0f, 0.0f, 0.0f};              // 0x1b4
    f32 mCameraOffsetY = 0.0f;                                  // 0x1c0
    sead::Quatf mBaseQuat = sead::Quatf::unit;                  // 0x1c4
    u8 _1d4[0x1e4 - 0x1d4];
    f32 mRotateAngleVel = 0.0f;                                 // 0x1e4
    bool mIsPulling = false;                                    // 0x1e8
    bool mIsFlipped = false;                                    // 0x1e9
    sead::Vector3f mPullDir = {0.0f, 0.0f, 0.0f};               // 0x1ec
    f32 mShootRate = 0.0f;                                      // 0x1f8
    sead::Vector3f mPlayerPos = sead::Vector3f::zero;           // 0x1fc
    f32 mTurnAngle = 0.0f;                                      // 0x208
    bool mIsRequestTop = false;                                 // 0x20c
    sead::PtrArray<SnapPoint> mSnapPoints;                      // 0x210
    SnapPoint* mBestSnapPoint = nullptr;                        // 0x220
    al::MtxConnector* mConnector = nullptr;                     // 0x228
    DummyCameraTarget* mCameraTarget = nullptr;                 // 0x230
    bool mIsCameraTargetOn = false;                             // 0x238
    bool mIsDisabled = false;                                   // 0x239
    bool mIsLighthouseFlag;                                     // 0x23a
    bool mIsFirstCatch = false;                                 // 0x23b
    s32 mWobbleSeTime = 0;                                      // 0x23c
    s32 mShakeCoolTime = 0;                                     // 0x240
    GuideType mGuideType = GuideType::None;                     // 0x244
};

static_assert(sizeof(FlingPole) == 0x248);
