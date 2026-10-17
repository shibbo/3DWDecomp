#pragma once

#include "Library/LiveActor/LiveActor.hpp"
#include "MapObj/IUseRouteDokan.hpp"
#include <container/seadPtrArray.h>
#include <math/seadQuat.h>
#include <math/seadVector.h>

namespace al {
class BlockRailPartsGroup;
class CameraInfo;
class CameraTicket;
}  // namespace al

class GuideBalloon;
class RouteDokanBazookaRider;
class RouteDokanEntrance;
class RouteDokanEntranceGroup;
class RouteDokanRider;

/**
 * @brief A network of pipes that carries players between its entrances.
 */
class RouteDokan : public al::LiveActor, public IUseRouteDokan {
public:
    RouteDokan(const char* pName);

    void init(const al::ActorInitInfo& rInfo) override;
    void active();
    void deactive();
    bool isBindStart(al::HitSensor* pSender) override;
    bool startBind(RouteDokanEntrance* pEntrance, const al::SensorMsg* pMsg,
                   al::HitSensor* pSender, al::HitSensor* pReceiver) override;
    bool cancelBind(al::HitSensor* pSender) override;
    bool damagePuppet(al::HitSensor* pSender) override;
    void updateLinkedTrans(const sead::Vector3f& rTrans) override;

private:
    al::BlockRailPartsGroup* mPartsGroup = nullptr;
    RouteDokanEntranceGroup* mEntranceGroup = nullptr;
    RouteDokanRider** mRiders = nullptr;
    s32 mRiderNum = 8;
    bool mIsUseSpecialAppear = false;
};

static_assert(sizeof(RouteDokan) == 0x170);

/**
 * @brief A pipe network ending in a cannon that launches every bound player at once.
 */
class RouteDokanBazooka : public al::LiveActor, public IUseRouteDokan {
public:
    using RiderArray = sead::FixedPtrArray<RouteDokanBazookaRider, 8>;

    /// How the riders are launched out of the cannon.
    enum ShootType : s32 {
        ShootType_Parabola = 0,  ///< Arc towards the linked target position.
        ShootType_Straight = 1,  ///< Fly in a straight line towards the target.
    };

    RouteDokanBazooka(const char* pName);

    void init(const al::ActorInitInfo& rInfo) override;
    void active();
    void deactive();
    void initAfterPlacement() override;
    bool isBindStart(al::HitSensor* pSender) override;
    bool startBind(RouteDokanEntrance* pEntrance, const al::SensorMsg* pMsg,
                   al::HitSensor* pSender, al::HitSensor* pReceiver) override;
    bool cancelBind(al::HitSensor* pSender) override;
    bool damagePuppet(al::HitSensor* pSender) override;
    bool isReadyAll() const;
    bool isDisplayGuideBalloon() const;
    bool tryAdjustActiveRiders();
    void invalidatePlayerDamage();
    void validatePlayerDamage();

    void exeAppear();
    void exeWait();
    void exeMove();
    void exeReady();
    void exeTryAllBind();
    void exeTurn();
    void exeWaitForShoot();
    void exeShoot();
    void exeShootEnd();
    void exeReset();

    bool isEnableActorRouteDokanMove() const override { return false; }

private:
    al::BlockRailPartsGroup* mPartsGroup = nullptr;
    RouteDokanEntranceGroup* mEntranceGroup = nullptr;
    RouteDokanEntrance* mStartEntrance = nullptr;
    RiderArray mRiders;
    RiderArray mBindRiders;
    al::CameraTicket* mCameraTicket = nullptr;
    al::CameraInfo* mCameraInfo = nullptr;
    GuideBalloon* mGuideBalloon = nullptr;
    bool mIsDisplayGuideIcon = true;
    bool mIsValidTarget = false;
    sead::Vector3f mTargetTrans = sead::Vector3f::zero;
    f32 mShootAngle = 45.0f;
    f32 mGravity = 0.8f;
    sead::Quatf mInitQuat = sead::Quatf::unit;
    sead::Quatf mTargetQuat = sead::Quatf::unit;
    s32 mShootType = ShootType_Parabola;
    bool mIsInvalidClipping = false;
    u32 mAppearIndex = 0;
    bool mIsUseSpecialAppear = false;
    s32 mWaitFrameCount = 30;
    bool mIsSingleMode = false;
    bool mIsUseDelayedCamera = false;
};

static_assert(sizeof(RouteDokanBazooka) == 0x270);
