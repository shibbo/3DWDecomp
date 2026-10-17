#pragma once
#include <math/seadVector.h>
#include "Library/LiveActor/LiveActor.hpp"

class TentackTentacle;
namespace al { class CollisionObj; }

/** @brief Scaffold block a Tentack tentacle carries up and throws off. */
class TentackTentacleStep : public al::LiveActor {
public:
    TentackTentacleStep(const char* pName, TentackTentacle* pTentacle,
                        const sead::Vector3f* pBasePos);
    ~TentackTentacleStep() override;
    void init(const al::ActorInitInfo&) override;
    void appear() override;
    void kill() override;
    void attackSensor(al::HitSensor*, al::HitSensor*) override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    void exeThrow();
    void exeWait();
    void exeBreak();
    void exeFall();
    void exeBreakLand();
    void exeBreakAir();
    void invalidateCollisionPartsLocal();
    void appearBreakable();
    void setBreak();
    void switchCollisionParts(bool isTentacleStiff);
    void startThrow(f32 speed);
    void startFall(f32 speed);
    bool isExistStep() const;
    bool isThrow() const;

private:
    TentackTentacle* mTentacle;
    const sead::Vector3f* mBasePos;
    bool mBreakable = false;
    al::CollisionObj* mClimbWall = nullptr;
};
static_assert(sizeof(TentackTentacleStep) == 0x168);
