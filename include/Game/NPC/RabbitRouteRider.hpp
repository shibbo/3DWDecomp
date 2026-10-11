#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>

#include "Library/Nerve/NerveExecutor.hpp"

namespace al {
class LiveActor;
}

class IUseRabbitRoutePoint;
class RabbitRoute;

/**
 * @brief Moves a position along a RabbitRoute, jumping between points where needed.
 * @note Only what reconstructed code needs is declared so far.
 */
class RabbitRouteRider : public al::NerveExecutor {
public:
    RabbitRouteRider(const al::LiveActor* pActor, const RabbitRoute* pRoute, bool isBig);

    void updatePoint(const IUseRabbitRoutePoint* pCurrent, const IUseRabbitRoutePoint* pNext);
    void move();
    bool isJumping() const;
    bool isJumpStart() const;
    const sead::Vector3f& getCurrentPointPos() const;
    bool isNearNextPoint() const;
    const sead::Vector3f& getNextPointPos() const;
    void reverse();
    void updateNextPointReversePlayer(const IUseRabbitRoutePoint* pPlayerPoint);
    void updateRotate();
    void setSpeed(f32 speed);
    bool tryStartJump();
    void exeMove();
    void exeJump();

    /** @return The point the rider last passed. */
    const IUseRabbitRoutePoint* getCurrentPoint() const { return mCurrentPoint; }

    /** @return The point the rider is heading to. */
    const IUseRabbitRoutePoint* getNextPoint() const { return mNextPoint; }

    /** @return The current position on the route. */
    const sead::Vector3f& getPos() const { return mPos; }

    /** @return The current moving direction. */
    const sead::Vector3f& getFront() const { return mFront; }

private:
    const al::LiveActor* mActor;
    const RabbitRoute* mRoute;
    const IUseRabbitRoutePoint* mCurrentPoint;
    const IUseRabbitRoutePoint* mNextPoint;
    sead::Vector3f mPos;
    sead::Vector3f mFront;
    u8 _48[0x58 - 0x48];
};

static_assert(sizeof(RabbitRouteRider) == 0x58);
