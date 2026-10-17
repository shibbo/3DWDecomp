#pragma once
#include "MapObj/BindPuppeteer.hpp"
#include <math/seadMatrix.h>
namespace al { struct ActorInitInfo; class ParabolicPath; }
class BindWarpEffect;
class CloudBonusLauncherBindPuppeteer : public BindPuppeteer {
public:
    CloudBonusLauncherBindPuppeteer(const char*, const al::ActorInitInfo&);
    void startBindForce(al::HitSensor*, al::HitSensor*, const sead::Vector3f&);
    void startBindInStart(al::HitSensor*, al::HitSensor*);
    void launchStart(float);
    void launchEnd();
    void startBonusStartWarp(const sead::Matrix34f&, int, int);
    void startBonusEndBind(al::HitSensor*, al::HitSensor*);
    bool tryTemporaryWarp();
    bool isBonusStartWarp() const;
    /** @return Whether the bound player reached the launcher and waits inside it. */
    bool isInWait() const { return mInWait; }
    void exeNothing();
    void exeBindForce();
    void exeInWait();
    void exeLaunch();
    void exeBonusStartWarp();
private:
    BindWarpEffect* mWarpEffect;
    al::HitSensor* mBinderSensor = nullptr;
    bool mInWait = false;
    float mLaunchSpeed = 0.0f;
    al::ParabolicPath* mPath = nullptr;
    sead::Vector3f mPreviousTrans = {0.0f, 0.0f, 0.0f};
    bool mTemporaryWarp = false;
};
static_assert(sizeof(CloudBonusLauncherBindPuppeteer) == 0x50);
