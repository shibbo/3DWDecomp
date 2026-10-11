#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>

#include "Library/Nerve/NerveStateBase.hpp"

namespace al {
class KeyPoseKeeper;
class LiveActor;
}  // namespace al

/**
 * @brief Movement parameters of a CrawlerWalkState.
 * @note Minimal declaration: the field meanings are not reconstructed yet.
 */
struct CrawlerWalkStateParam {
    CrawlerWalkStateParam();
    CrawlerWalkStateParam(f32 param0, f32 param1, f32 param2, f32 param3, f32 param4, f32 param5,
                          f32 param6, f32 param7, f32 param8, f32 param9, s32 param10);

    u8 _0[0x2c];
};

static_assert(sizeof(CrawlerWalkStateParam) == 0x2c);

/**
 * @brief Walking state of a Crawler: walks along the ground or along key poses.
 * @note Minimal declaration: only what the reconstructed code uses is declared so far.
 */
class CrawlerWalkState : public al::ActorStateBase {
public:
    CrawlerWalkState(al::LiveActor* pHost, al::KeyPoseKeeper* pKeyPoseKeeper,
                     const sead::Vector3f& rDisplayOffset, const CrawlerWalkStateParam* pParam);

    /**
     * @brief Sets the step the first key pose move starts at.
     * @param step Start step.
     */
    void setKeyMoveStartStep(s32 step) { mKeyMoveStartStep = step; }

    /**
     * @brief Sets the walk speed.
     * @param speed Walk speed.
     */
    void setWalkSpeed(f32 speed) { mWalkSpeed = speed; }

private:
    u8 _20[0x14];
    s32 mKeyMoveStartStep;
    f32 mWalkSpeed;
    u8 _3c[0x34];
};

static_assert(sizeof(CrawlerWalkState) == 0x70);
