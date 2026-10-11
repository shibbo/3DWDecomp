#pragma once

#include "Library/Nerve/NerveStateBase.hpp"
#include <math/seadVector.h>

namespace al {
class HitSensor;
}

struct EnemyStateBlowDownParam {
    EnemyStateBlowDownParam(bool isSwim);
    EnemyStateBlowDownParam(const char* pAction);
    EnemyStateBlowDownParam(const char* pAction, float speed, float jumpSpeed,
                           float friction, float gravity, int step);

    const char* mAction;
    float mSpeed;
    float mJumpSpeed;
    float mFriction;
    float mGravity;
    int mStep;
};

class EnemyStateBlowDown : public al::ActorStateBase {
public:
    EnemyStateBlowDown(const char* pName, al::LiveActor* pHost,
                       const EnemyStateBlowDownParam* pParam);
    EnemyStateBlowDown(al::LiveActor* pHost, const EnemyStateBlowDownParam* pParam);
    /** @brief Destroys the knockback state. */
    ~EnemyStateBlowDown() override = default;
    void appear() override;
    void kill() override;
    void setBlowDir(const al::LiveActor* pActor);
    void setBlowDir(const sead::Vector3f& rDir);
    void setBlowDir(const al::HitSensor* pOther, const al::HitSensor* pSelf);
    void setBlowDirScale(const sead::Vector3f& rDir);
    void setBlowDownParam(const EnemyStateBlowDownParam* pParam);
    void exeDown();

protected:
    const EnemyStateBlowDownParam* mParam;
    sead::Vector3f mBlowDir = sead::Vector3f::ez;
    bool mIsKeepClippingInvalid = false;
};

static_assert(sizeof(EnemyStateBlowDownParam) == 0x20);
static_assert(sizeof(EnemyStateBlowDown) == 0x38);
