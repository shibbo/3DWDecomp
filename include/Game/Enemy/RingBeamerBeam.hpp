#pragma once

#include <basis/seadTypes.h>
#include <math/seadQuat.h>
#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"

/** @brief Kind of owner that fires a ring beam. */
enum RingBeamerType : s32 {
    RingBeamerType_BossBunretsu = 4,
};

/** @brief Speed, lifetime and effect of a ring beam. */
class RingBeamerParam {
public:
    RingBeamerParam();
    RingBeamerParam(f32 speed, s32 lifeTime, const char* pEmitEffectName);

    f32 mSpeed;
    s32 mLifeTime;
    const char* mEmitEffectName;
};
static_assert(sizeof(RingBeamerParam) == 0x10);

/** @brief Expanding ring shock wave fired by RingBeamer and some bosses. */
class RingBeamerBeam : public al::LiveActor {
public:
    RingBeamerBeam(const char* pName, al::LiveActor* pHost, RingBeamerType type, bool flag);

    void setRingBeamerParam(const RingBeamerParam* pParam);
    void setActiveWithPosture(const sead::Vector3f& rTrans, const sead::Quatf& rQuat);

    /**
     * @brief Sets the frame of the color animation.
     * @param frame Color animation frame.
     */
    void setColorFrame(s32 frame) { mColorFrame = frame; }

private:
    u8 _144[0x164 - 0x144];
    s32 mColorFrame;  // 0x164 color animation frame
    u8 _168[0x170 - 0x168];
};
static_assert(sizeof(RingBeamerBeam) == 0x170);
