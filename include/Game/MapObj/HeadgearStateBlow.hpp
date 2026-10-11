#pragma once
#include "Library/Nerve/NerveStateBase.hpp"
#include <math/seadVector.h>

namespace al { class SensorMsg; class HitSensor; }

struct HeadgearStateBlowParam {
    HeadgearStateBlowParam();
    constexpr HeadgearStateBlowParam(const char* damageAction, const char* goalAction,
        const char* silentAction, float goalUp, float goalBack, float damageUp,
        float damageBack, float gravity, float velocityScale, int collideStep)
        : mDamageAction(damageAction), mGoalAction(goalAction), mSilentAction(silentAction),
          mGoalUpSpeed(goalUp), mGoalBackSpeed(goalBack), mDamageUpSpeed(damageUp),
          mDamageBackSpeed(damageBack), mGravity(gravity), mVelocityScale(velocityScale),
          mCollideStep(collideStep) {}

    const char* mDamageAction;
    const char* mGoalAction;
    const char* mSilentAction;
    float mGoalUpSpeed;
    float mGoalBackSpeed;
    float mDamageUpSpeed;
    float mDamageBackSpeed;
    float mGravity;
    float mVelocityScale;
    int mCollideStep;
};

class HeadgearStateBlow : public al::ActorStateBase {
public:
    HeadgearStateBlow(al::LiveActor* pActor, const HeadgearStateBlowParam* pParam);
    bool tryStart(const al::SensorMsg* pMsg, al::HitSensor* pSensor);
    void exeBlow();

private:
    const HeadgearStateBlowParam* mParam;
    al::HitSensor* mReleaseSensor;
    sead::Vector3f mLaunchVelocity;
};
