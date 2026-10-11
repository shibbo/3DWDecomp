#pragma once

#include "Library/LiveActor/LiveActor.hpp"
#include <math/seadMatrix.h>
#include <math/seadQuat.h>
#include <math/seadVector.h>

namespace al {
class BlockRailRider;
class ComboCounter;
}  // namespace al

class BindWarpEffect;
class IUsePlayerPuppet;
class PuppetStickRouteSelecter;
class RouteDokanBazooka;
class RouteDokanEntrance;
class RouteDokanInOutEffect;

/**
 * @brief Carries one player through a RouteDokanBazooka and launches them out of it.
 */
class RouteDokanBazookaRider : public al::LiveActor {
public:
    RouteDokanBazookaRider(RouteDokanBazooka* pHost, const char* pName, s32 selecterCapacity,
                           f32 gravity);

    void init(const al::ActorInitInfo& rInfo) override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool damage(al::HitSensor* pSender);
    void setMoveSpeed(f32 speed);
    void setOutVelocity(const sead::Vector3f& rVelocity);
    void shoot();
    bool isActive(s32 playerIndex) const;
    bool isEndBind() const;
    bool isStateReady() const;
    bool isStateFlying() const;
    void startBind(RouteDokanEntrance* pEntrance, const al::SensorMsg* pMsg,
                   al::HitSensor* pSender, al::HitSensor* pReceiver, bool isContinue);
    void startBindForce(al::HitSensor* pSender, RouteDokanEntrance* pEntrance,
                        IUsePlayerPuppet* pPuppet);
    void startBindContinue(s32 playerIndex, RouteDokanEntrance* pEntrance,
                           IUsePlayerPuppet* pPuppet);
    void startBindNormal(s32 playerIndex, RouteDokanEntrance* pEntrance,
                         IUsePlayerPuppet* pPuppet);
    void addRouteSelectPuppet(IUsePlayerPuppet* pPuppet);
    bool tryCancelBind(al::HitSensor* pSender);
    void forceCancelBind();
    void emitEffectLaunch();
    void updatePuppetPose();
    void doLanding();

    void exeStart();
    void exeMove();
    void exeBindWarp();
    void exeReady();
    void exeParabolaFly();
    void exeParabolaFlyLandStart();
    void exeParabolaFlyLand();
    void exeShoot();
    void exeEnd();
    void exeCancel();
    void exeInvalid();

    IUsePlayerPuppet* getPuppet() const { return mPuppet; }

    s32 getPlayerIndex() const { return mPlayerIndex; }

    void setShootType(s32 shootType) { mShootType = shootType; }

    void setUpDir(const sead::Vector3f& rUpDir) { mUpDir = rUpDir; }

    void setShootFrame(s32 frame) { mShootFrame = frame; }

private:
    RouteDokanBazooka* mHost;
    IUsePlayerPuppet* mPuppet = nullptr;
    al::BlockRailRider* mRailRider = nullptr;
    PuppetStickRouteSelecter* mRouteSelecter;
    RouteDokanInOutEffect* mInOutEffect = nullptr;
    s32 mPlayerIndex = -1;
    sead::Quatf mQuat = sead::Quatf::unit;
    sead::Vector3f mTrans = sead::Vector3f::zero;
    sead::Quatf mStartQuat = sead::Quatf::unit;
    sead::Vector3f mStartTrans = sead::Vector3f::zero;
    f32 mMoveSpeed = 20.0f;
    sead::Vector3f mOutVelocity = sead::Vector3f::zero;
    s32 mShootType = 0;
    sead::Vector3f mVelocity = sead::Vector3f::zero;
    al::ComboCounter* mComboCounter;
    u32 mHitCount = 0;
    BindWarpEffect* mBindWarpEffect = nullptr;
    sead::Matrix34f mLaunchEffectMtx = sead::Matrix34f::ident;
    f32 mGravity;
    sead::Vector3f mUpDir = sead::Vector3f::ey;
    u8 _228[0x14];
    s32 mShootFrame = -1;
};

static_assert(sizeof(RouteDokanBazookaRider) == 0x240);
