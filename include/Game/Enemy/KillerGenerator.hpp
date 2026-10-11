#pragma once

#include "Library/LiveActor/LiveActor.hpp"
#include <math/seadQuat.h>
#include <math/seadVector.h>

namespace al {
class CollisionPartsFilterBase;
template <typename T> class DeriveActorGroup;
}
class Killer;

class KillerGenerator : public al::LiveActor {
public:
    KillerGenerator(const char* pName, al::LiveActor* pHost, int type,
                    const al::CollisionPartsFilterBase* pFilter);
    /** @brief Releases the projectile generator. */
    ~KillerGenerator() override = default;
    void init(const al::ActorInitInfo& rInfo) override;
    bool hideActor() override;
    bool showActor() override;
    void kill() override;
    void startShoot();
    /** @brief Pauses automatic projectile firing. */
    void stopShoot() { mIsShoot = false; }
    void killBySwitch();
    void updateQT(const sead::Vector3f& rTrans, const sead::Quatf& rQuat);
    void shoot();
    bool tryStartHostAction(const char* pAction);
    float getAccelRate() const;
    void exeDeactive();
    void exeDelay();
    void exeStandByAppear();
    void exeStandBy();
    void exeWait();
    void exeShootKeepAppear();
    void exeShootKeepShoot();

private:
    friend class Killer;

    al::LiveActor* mHost;
    al::DeriveActorGroup<Killer>* mKillers = nullptr;
    Killer* mStandByKiller = nullptr;
    const al::CollisionPartsFilterBase* mFilter;
    int mType;
    int mStepWait = 240;
    int mStepDelay = 0;
    int mStepDisappear = 500;
    int mStepAppear = 0;
    float mAccelFly = 8.0f;
    bool mIsValidateCollision = true;
    bool mIsShoot = true;
    bool mIsShootImmediatelySwitchOn = false;
};
