#pragma once

#include <basis/seadTypes.h>
#include <math/seadBoundBox.h>
#include <math/seadMatrix.h>
#include <math/seadQuat.h>
#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"

namespace al {
class CameraInfo;
class EffectMtxSetter;
}  // namespace al

class BossDemoStartInfo;
class BossStateDemoStart;
class BossWackunBody;
class BossWackunFrame;
class BossWackunHand;

/** @brief Whack (Wackun) boss: a giant block that tumbles over its faces towards the player. */
class BossWackun : public al::LiveActor {
public:
    /** @brief How the body tumbles onto its next face. */
    enum class RotateType : s32 {
        FallFront = 0,
        FallBack = 1,
        Side = 2,
        Rise = 3,
    };

    explicit BossWackun(const char* pName);

    void kill() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void start();
    void setRotatePose(f32 degree);
    void control() override;
    void exeWaitStart();
    void exeWaitDemoStart();
    void exeDemoStart();
    void exeBattleStart();
    void exeWait();
    bool setRotateAxisAndPos(const sead::Vector3f& rTargetPos, bool isReverse);
    void exeRotateSign();
    void exeRotate();
    void exeLand();
    void resetPoseError();
    void exeDamageRotate();
    void exeDamage();
    void exeRecoverStandUp();
    void exeRecoverJump();
    void exeRecoverFrame();
    void exeRecoverRotate();
    void exeRecoverFallSign();
    void exeRecoverFall();
    void exeRecoverLand();
    void exeDown();
    ~BossWackun() override;

    /** @return Axis the body tumbles around during the current rotation. */
    const sead::Vector3f& getRotateAxis() const { return mRotateAxis; }

private:
    BossWackunBody* mBody = nullptr;                             // 0x148
    BossWackunFrame* mFrame = nullptr;                           // 0x150
    BossWackunHand* mHand;                                       // 0x158
    BossDemoStartInfo* mDemoStartInfo = nullptr;                 // 0x160
    BossStateDemoStart* mStateDemoStart = nullptr;               // 0x168
    al::EffectMtxSetter* mEffectMtxSetter = nullptr;             // 0x170
    al::CameraInfo* mCameraInfo = nullptr;                       // 0x178
    sead::BoundBox3f mBoxInfo;                                   // 0x180
    sead::Matrix34f mLandEffectMtx = sead::Matrix34f::ident;     // 0x198
    sead::Vector3f mCameraLookAtPos = sead::Vector3f::zero;      // 0x1C8
    sead::Quatf mInitQuat = sead::Quatf::unit;                   // 0x1D4
    sead::Vector3f mInitTrans = {0.0f, 0.0f, 0.0f};              // 0x1E4
    sead::Vector3f mPoseTrans = {0.0f, 0.0f, 0.0f};              // 0x1F0
    sead::Quatf mRotateBaseQuat = sead::Quatf::unit;             // 0x1FC
    sead::Vector3f mRotateBaseTrans = {0.0f, 0.0f, 0.0f};        // 0x20C
    sead::Vector3f mRotateCenter = {0.0f, 0.0f, 0.0f};           // 0x218
    sead::Vector3f mRotateAxis = sead::Vector3f::ex;             // 0x224
    f32 _230;
    sead::Vector3f mRotateNormal = sead::Vector3f::ez;           // 0x234
    f32 mRotateDegree = 0.0f;                                    // 0x240
    f32 mDamageDegree = 0.0f;                                    // 0x244
    RotateType mRotateType = RotateType::FallFront;              // 0x248
    s32 _24C = 0;
    bool mIsRotateAxisValid = false;                             // 0x250
    bool mIsReverseNextRotate = false;                           // 0x251
};
static_assert(sizeof(BossWackun) == 0x258);
