#pragma once

#include "Library/Nerve/NerveExecutor.hpp"
#include <math/seadVector.h>

namespace al { class LiveActor; }
class RabbitRoute;
class IUseRabbitRoutePoint;

class RabbitRouteRider : public al::NerveExecutor {
public:
    /** @brief Gets the current route point. @return the current route point. */
    const IUseRabbitRoutePoint* getCurrentPoint() const { return mCurrentPoint; }
    /** @brief Gets the next route point. @return the next route point. */
    const IUseRabbitRoutePoint* getNextPoint() const { return mNextPoint; }
    /** @brief Gets the current route position. @return the current route position. */
    const sead::Vector3f& getPos() const { return mPosition; }
    /** @brief Gets the route movement direction. @return the route movement direction. */
    const sead::Vector3f& getFront() const { return mDirection; }
    RabbitRouteRider(const al::LiveActor* pActor, const RabbitRoute* pRoute, bool isFastJump);
    /** @brief Releases the rider's nerve executor. */
    ~RabbitRouteRider() override = default;
    void updatePoint(const IUseRabbitRoutePoint* pPoint, const IUseRabbitRoutePoint* pNextPoint);
    void move();
    bool isJumping() const;
    bool isJumpStart() const;
    const sead::Vector3f& getCurrentPointPos() const;
    bool isNearNextPoint() const;
    const sead::Vector3f& getNextPointPos() const;
    void reverse();
    void updateNextPointReversePlayer(const IUseRabbitRoutePoint* pPreviousPoint);
    void updateRotate();
    /** @brief Gets the route direction. @return Current movement direction. */
    const sead::Vector3f& getDirection() const { return mDirection; }
    void setSpeed(float speed);
    bool tryStartJump();
    void exeMove();
    void exeJump();

private:
    const al::LiveActor* mActor;
    bool mIsFastJump;
    const IUseRabbitRoutePoint* mCurrentPoint;
    const IUseRabbitRoutePoint* mNextPoint;
    sead::Vector3f mPosition;
    sead::Vector3f mDirection;
    float mSpeed;
    int mJumpSteps;
    int mJumpHeight;
};
