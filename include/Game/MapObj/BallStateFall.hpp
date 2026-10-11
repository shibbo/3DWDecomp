#pragma once

#include "Library/Nerve/NerveStateBase.hpp"

class BallStateFallParam;

class BallStateFall : public al::ActorStateBase {
public:
    BallStateFall(al::LiveActor* pActor, const BallStateFallParam* pParam);
    void appear() override;
    void exeFall();
    void setIsRotate(bool isRotate) { mIsRotate = isRotate; }

private:
    const BallStateFallParam* mParam;
    bool mIsRotate = true;
};

static_assert(sizeof(BallStateFall) == 0x30);
