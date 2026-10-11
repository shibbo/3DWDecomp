#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"

namespace al {
class HitSensor;
}  // namespace al

/**
 * @brief The light that carries a bound player to a warp destination.
 */
class BindWarpEffect : public al::LiveActor {
public:
    BindWarpEffect();
    ~BindWarpEffect() override;

    void init(const al::ActorInitInfo& rInfo) override;

    void start(const al::HitSensor* pPlayerSensor, const sead::Vector3f& rEndPos, bool isFast);
    void start(const sead::Vector3f& startPos, const sead::Vector3f& endPos, bool noTrail);
    void start(const sead::Vector3f& startPos, const sead::Vector3f* endPos, bool noTrail);
    void start(const al::HitSensor*, const sead::Vector3f*, bool noTrail);
    const char* getEffectName() const;
    void setEndPos(const sead::Vector3f&);
    void calcEndPos(sead::Vector3f*) const;
    void exeWait();
    void exeMove();
    void exeDone();
    void cancel();
    bool isMoving() const;
    bool isEnd() const;

private:
    const al::HitSensor* mPlayerSensor = nullptr;
    sead::Vector3f mStartPos = sead::Vector3f::zero;
    sead::Vector3f mEndPos = sead::Vector3f::zero;
    const sead::Vector3f* mEndPosPtr = nullptr;
    bool mIsKoopaJr = false;
};

static_assert(sizeof(BindWarpEffect) == 0x178);
