#pragma once
#include <basis/seadTypes.h>
#include <math/seadVector.h>
#include "Library/Nerve/NerveStateBase.hpp"
namespace al {
class ActorInitInfo;
class HitSensor;
class BlockRailRider;
class BlockRailRouteSelecter;
}
struct WalkerStateParam;
class RouteDokanInOutEffect;

/** @brief Speed, spin, and reentry settings for pipe travel. */
class WalkerStateRouteDokanMoveParam {
public:
    WalkerStateRouteDokanMoveParam();
    WalkerStateRouteDokanMoveParam(f32 speed, f32 ejectSpeed, f32 gravityLerp, f32 spinSpeed, s32 reentryDelay);
    f32 mSpeed;
    f32 mEjectSpeed;
    f32 mGravityLerp;
    f32 mSpinSpeed;
    s32 mReentryDelay;
};
static_assert(sizeof(WalkerStateRouteDokanMoveParam) == 0x14);

/** @brief Moves a walking enemy through route pipes. */
class WalkerStateRouteDokanMove : public al::ActorStateBase {
public:
    WalkerStateRouteDokanMove(al::LiveActor* pHost, const al::ActorInitInfo& rInfo,
                              const WalkerStateParam* pParam,
                              const WalkerStateRouteDokanMoveParam* pMoveParam);
    /** @brief Destroys the pipe movement state. */
    ~WalkerStateRouteDokanMove() override = default;
    void appear() override;
    bool tryStart(al::HitSensor* pSelf, al::HitSensor* pOther);
    void setRouteSelecter(al::BlockRailRouteSelecter* pSelecter);
    bool isMove() const;
    bool isEject() const;
    void exeStart();
    void exeMove();
    void exeEject();
    void exeLand();
private:
    al::BlockRailRider* mRailRider = nullptr;
    RouteDokanInOutEffect* mEffect = nullptr;
    sead::Vector3f mFront = sead::Vector3f::ez;
    sead::Vector3f mMoveDir = sead::Vector3f::ez;
    sead::Vector3f mGravity;
    f32 mSpinAngle;
    const WalkerStateParam* mParam;
    const WalkerStateRouteDokanMoveParam* mMoveParam;
};
static_assert(sizeof(WalkerStateRouteDokanMove) == 0x68);
