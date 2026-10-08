#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>

#include "Library/Nerve/NerveStateBase.hpp"

namespace al {
class ActorInitInfo;
}  // namespace al

class DarkBowser;

/**
 * @brief Fury Bowser's jump to another spot of the arena, done before some attacks.
 * @note Only what reconstructed code needs is declared so far.
 */
class DarkBowserJump : public al::NerveStateBase {
public:
    /**
     * @brief Scores a candidate landing spot; the best scoring spot is jumped to.
     * @param rHostTrans Position of Fury Bowser.
     * @param rTargetTrans Position of the target (the player).
     * @param rSpotTrans Position of the candidate spot.
     * @return Score of the spot.
     */
    using SpotScoreFunc = f32 (*)(const sead::Vector3f& rHostTrans,
                                  const sead::Vector3f& rTargetTrans,
                                  const sead::Vector3f& rSpotTrans);

    DarkBowserJump(DarkBowser* pHost, const al::ActorInitInfo& rInfo);

    void appear() override;
    void kill() override;
    void exeJumpStart();
    void calcJumpPosition();
    void exeJump();
    void exeLand();
    void setCenterJump();

    /**
     * @brief Sets the function used to score the candidate landing spots.
     * @param pFunc Scoring function.
     */
    void setSpotScoreFunc(SpotScoreFunc pFunc) { mSpotScoreFunc = pFunc; }

    /** @brief Makes the jump pick its spot without looking at the target. */
    void setIgnoreTarget() { mIsIgnoreTarget = true; }

private:
    u8 _11[0x60 - 0x11];  // starts in the tail padding of al::NerveStateBase
    SpotScoreFunc mSpotScoreFunc;  // 0x60
    bool mIsCenterJump;            // 0x68
    bool mIsIgnoreTarget;          // 0x69
};
static_assert(sizeof(DarkBowserJump) == 0x70);
