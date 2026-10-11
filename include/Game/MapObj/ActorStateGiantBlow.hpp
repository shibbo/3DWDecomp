#pragma once

#include "Library/Nerve/NerveStateBase.hpp"
#include <math/seadVector.h>

namespace al {
class HitSensor;
class LiveActor;
class SensorMsg;
}  // namespace al

struct ActorStateGiantBlowParam {
    ActorStateGiantBlowParam();
    constexpr ActorStateGiantBlowParam(int duration, float speed, float upSpeed,
        float velocityScale, float gravity, float rotateSpeed, int rotateAxis)
        : mDuration(duration), mSpeed(speed), mUpSpeed(upSpeed), mVelocityScale(velocityScale),
          mGravity(gravity), mRotateSpeed(rotateSpeed), mRotateAxis(rotateAxis) {}

    int mDuration;
    float mSpeed;
    float mUpSpeed;
    float mVelocityScale;
    float mGravity;
    float mRotateSpeed;
    int mRotateAxis;
};

/**
 * @brief Actor state that blows an object away when a giant player rams it, swapping the actor
 * for a broken model and a trace model.
 */
class ActorStateGiantBlow : public al::ActorStateBase {
public:
    ActorStateGiantBlow(al::LiveActor* pActor, const ActorStateGiantBlowParam* pParam,
                        al::LiveActor* pBreakModel, al::LiveActor* pTraceModel);

    void appear() override;

    bool tryStartBlow(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf);

    void exeBlow();

private:
    const ActorStateGiantBlowParam* mParam;
    al::LiveActor* mBreakModel;
    al::LiveActor* mTraceModel;
    sead::Vector3f mBlowDirection = sead::Vector3f::ez;
};

static_assert(sizeof(ActorStateGiantBlow) == 0x48);
