#pragma once

#include "Library/Nerve/NerveStateBase.hpp"
#include "MapObj/BallStateThrowParam.hpp"

namespace al { class HitSensor; }

class BallStateThrow : public al::ActorStateBase {
public:
    BallStateThrow(al::LiveActor* pActor, const BallStateThrowParam* pParam);
    void appear() override;
    void setThrowParam(const al::HitSensor* pSensor, const BallStateThrowParam* pParam,
                        const al::LiveActor* pThrower);
    void exeThrow();
    void exeThrowAuto();

private:
    const BallStateThrowParam* mParam;
    const al::HitSensor* mThrowSensor = nullptr;
    const al::LiveActor* mThrower = nullptr;
};
