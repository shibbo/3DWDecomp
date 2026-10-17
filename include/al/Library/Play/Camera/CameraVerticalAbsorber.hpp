#pragma once

#include <gfx/seadCamera.h>
#include <gfx/seadProjection.h>

#include "Library/Nerve/NerveExecutor.hpp"

namespace al {
class ByamlIter;
class CameraPoser_RS;
struct CameraStartInfo;

class CameraVerticalAbsorber : public NerveExecutor {
public:
    CameraVerticalAbsorber(const CameraPoser_RS* pPoser, bool isNoCameraPosAbsorb);

    void load(const ByamlIter& rIter);
    void start(const sead::Vector3f& rPos, const CameraStartInfo& rInfo);
    bool isValid() const;
    void update();
    void makeLookAtCamera(sead::LookAtCamera* pCamera) const;
    void liberateAbsorb();
    void exeAbsorb();
    void exeFollow();
    void exeFollowGround();
    void exeFollowClimbPole();
    void exeFollowAbsolute();
    void exeFollowWater();
    bool isAbsorbing() const;
    void invalidate();
    void tryResetAbsorbVecIfInCollision(const sead::Vector3f& rPos);

    f32 getAbsorbHeight() const { return mAbsorbVec.y; }

    f32 getAbsorbScreenPosUp() const { return mAbsorbScreenPosUp; }

    f32 getAbsorbScreenPosDown() const { return mAbsorbScreenPosDown; }

    void setIsStopUpdate(bool isStop) { mIsStopUpdate = isStop; }

    void setIsKeepInFrame(bool isKeep) { mIsKeepInFrame = isKeep; }

    void setKeepInFrameOffsetUp(f32 offset) { mKeepInFrameOffsetUp = offset; }

    void setKeepInFrameOffsetDown(f32 offset) { mKeepInFrameOffsetDown = offset; }

    const f32* getFollowRate() const { return mFollowRate; }

    void setFollowRate(const f32* pRate) { mFollowRate = pRate; }

private:
    void projectToScreen(sead::Vector2f* pOut, const sead::Vector3f& rPos) const;

    const CameraPoser_RS* mCameraPoser;
    sead::LookAtCamera mLookAtCamera;
    sead::PerspectiveProjection mProjection;
    sead::Vector3f mAbsorbVec = {0.0f, 0.0f, 0.0f};
    f32 mLerp1 = 0.0f;
    f32 mAbsorbScreenPosUp = 75.0f;
    f32 mAbsorbScreenPosDown = 550.0f;
    bool mIsAdvanceAbsorbUp = false;
    f32 mAdvanceAbsorbScreenPosUp = 0.0f;
    bool mIsExistCollisionUnderTarget = false;
    sead::Vector3f mUnderTargetCollisionPos = {0.0f, 0.0f, 0.0f};
    sead::Vector3f mUnderTargetCollisionNormal = {0.0f, 0.0f, 0.0f};
    f32 mLerp2 = 0.0f;
    f32 mKeepInFrameOffsetUp = 0.0f;
    f32 mKeepInFrameOffsetDown = 0.0f;
    f32 mHighJumpJudgeSpeedV = 35.0f;
    sead::Vector3f mPrevTargetTrans = {0.0f, 0.0f, 0.0f};
    sead::Vector3f mTargetFront = sead::Vector3f::ez;
    sead::Vector3f mPrevTargetFront = sead::Vector3f::ez;
    bool mIsNoCameraPosAbsorb = false;
    bool mIsInvalidated = false;
    bool _1aa = false;
    bool mIsStopUpdate = false;
    bool mIsKeepInFrame = false;
    const f32* mFollowRate = nullptr;
};

static_assert(sizeof(CameraVerticalAbsorber) == 0x1b8);

}  // namespace al
