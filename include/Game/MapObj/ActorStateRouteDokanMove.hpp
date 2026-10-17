#pragma once
#include <math/seadVector.h>
#include "Library/Nerve/NerveStateBase.hpp"

namespace al {
class ActorInitInfo;
class HitSensor;
class BlockRailRider;
class BlockRailRouteSelecter;
}
class RouteDokanInOutEffect;

class ActorStateRouteDokanMove : public al::ActorStateBase {
public:
    ActorStateRouteDokanMove(al::LiveActor* pActor, const al::ActorInitInfo& rInfo);
    ~ActorStateRouteDokanMove() override;
    void appear() override;
    void exeStart();
    void exeMove();
    bool tryStart(al::HitSensor* pSelf, al::HitSensor* pOther);
    void setMoveSpeed(float speed);
    void setEndSpeed(float speed);
    void setRouteSelecter(al::BlockRailRouteSelecter* pSelecter);
    void forceCalcMoveDirection();
    const sead::Vector3f& getMoveDirection() const { return mMoveDirection; }
    const sead::Vector3f& getFrontDirection() const { return mFrontDirection; }
private:
    al::BlockRailRider* mRailRider = nullptr;
    RouteDokanInOutEffect* mEffect = nullptr;
    sead::Vector3f mFrontDirection = sead::Vector3f::ez;
    sead::Vector3f mMoveDirection = sead::Vector3f::ez;
    float mMoveSpeed = 25.0f;
    float mEndSpeed = 25.0f;
};
