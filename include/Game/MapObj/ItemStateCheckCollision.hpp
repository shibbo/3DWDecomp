#pragma once

#include "Library/Nerve/NerveStateBase.hpp"
#include <math/seadVector.h>

namespace al {
class CollisionParts;
class CollisionPartsConnector;
class SensorMsg;
class HitSensor;
}

class ItemStateCheckCollision : public al::ActorStateBase {
public:
    ItemStateCheckCollision(al::LiveActor* pActor);
    void init() override;
    void appear() override;
    void kill() override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*);
    void connectToCollisionParts();
    void exeLand();
    void exeWaitConnect();
    void exeFall();

    void enableWaterReaction() { mWaterReaction = true; }
private:
    al::CollisionPartsConnector* mConnector;
    const al::CollisionParts* mCollisionParts = nullptr;
    sead::Vector3f mBaseTrans = {0.0f, 0.0f, 0.0f};
    sead::Vector3f mPreviousTrans = {0.0f, 0.0f, 0.0f};
    bool mPreserveInvalidClipping = false;
    bool mWaterReaction = false;
};
