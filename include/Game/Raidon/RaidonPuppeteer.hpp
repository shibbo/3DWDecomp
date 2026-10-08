#pragma once

#include <basis/seadTypes.h>
#include <math/seadMatrix.h>
#include <math/seadVector.h>

#include "Library/Nerve/NerveExecutor.hpp"

namespace al {
class ActorInitInfo;
}  // namespace al

class IUsePlayerPuppet;

/// Drives one player riding Plessie: getting on and off and reading the rider's input.
class RaidonPuppeteer : public al::NerveExecutor {
public:
    RaidonPuppeteer();

    void init(const al::ActorInitInfo& rInfo);
    void cancelBind();
    void startGetOn(IUsePlayerPuppet* pPuppet, const sead::Matrix34f* pMtx, bool isWarp);
    void startBindWarp(IUsePlayerPuppet* pPuppet, const sead::Matrix34f* pMtx);
    void startGetOff(const sead::Vector3f& rDir, s32 delay, bool, bool);
    void updateInput(bool isOnGround, bool isEnableInput);

    IUsePlayerPuppet* mPuppet = nullptr;  // 0x10

    /** @return Horizontal stick input used for the blend animation. */
    f32 getStickX() const { return mStickX; }

    /** @return Vertical stick input used for the blend animation. */
    f32 getStickY() const { return mStickY; }

    /** @return Steering input of this rider. */
    f32 getHandle() const { return mHandle; }

    /** @return Acceleration input of this rider. */
    f32 getAccel() const { return mAccel; }

    /** @return Whether this rider requested a jump. */
    bool isRequestJump() const { return mJumpRequest > 0; }

    /** @return Whether this rider is jumping. */
    bool isJump() const { return mIsJump; }

private:
    u8 _18[0x38 - 0x18];
    f32 mStickX;       // 0x38
    f32 mStickY;       // 0x3c
    f32 mHandle;       // 0x40
    f32 mAccel;        // 0x44
    u8 _48[0x4c - 0x48];
    s32 mJumpRequest;  // 0x4c
    bool mIsJump;      // 0x50
    u8 _51[0x58 - 0x51];
};

static_assert(sizeof(RaidonPuppeteer) == 0x58);
