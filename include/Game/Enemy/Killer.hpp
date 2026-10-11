#pragma once

#include "Library/LiveActor/LiveActor.hpp"

namespace al {
class CollisionPartsFilterBase;
}
class KillerGenerator;
class ActorStateSupportFreeze;
class EnemyStateBlowDown;
class KillerStateFly;

class Killer : public al::LiveActor {
public:
    Killer(const char* pName, int type, KillerGenerator* pGenerator,
           const al::CollisionPartsFilterBase* pFilter);
    /** @brief Releases the Bullet Bill actor. */
    ~Killer() override = default;
    void init(const al::ActorInitInfo& rInfo) override;
    void appear() override;
    void kill() override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;
    void invalidateAttackSensors();
    void validateAttackSensors();
    void killBySwitch();
    bool isNerveFlyWait();
    void tryOnCollide();
    void exeAppear();
    void exeStandBy();
    void exeFlyWait();
    void exeFallDown();
    void exeHipDropDown();
    void exeReflect();
    void exeExplode();
    void exeBlowDown();
    void exeSupportFreeze();
    void forceStandBy();
    void startStandByAppear();
    void startFlyWait();
    int getStepDisappear() const;
    float getAccel() const;
    float getAccelRate() const;
    static bool isMagnum(int type);
    static bool isSearch(int type);
    bool isStandBy() const;

private:
    friend class KillerStateFly;

    KillerGenerator* mGenerator;
    const al::CollisionPartsFilterBase* mFilter;
    al::LiveActor* mHipDropActor = nullptr;
    ActorStateSupportFreeze* mStateSupportFreeze = nullptr;
    EnemyStateBlowDown* mStateBlowDown = nullptr;
    KillerStateFly* mStateFly = nullptr;
    int mType;
    float mJointRotation = 0.0f;
    bool mIsCollide = false;
    bool mIsSingleMode = false;
};
