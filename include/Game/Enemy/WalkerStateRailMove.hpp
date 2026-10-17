#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>
#include <prim/seadSafeString.h>
#include "Library/Nerve/NerveStateBase.hpp"

struct WalkerStateParam;

/** @brief Movement and animation settings for rail walking. */
class WalkerStateRailMoveParam {
public:
    WalkerStateRailMoveParam();
    WalkerStateRailMoveParam(f32 speed);
    WalkerStateRailMoveParam(f32 speed, f32 returnDistance, f32 railTurnDegree,
                            f32 turnDegree, f32 returnAccel, f32 returnTurnDegree,
                            const char* pWalkAction, const char* pTurnAction, bool turnAlongGround);
    f32 mSpeed;
    f32 mReturnDistance;
    f32 mRailTurnDegree;
    f32 mTurnDegree;
    f32 mReturnAccel;
    f32 mReturnTurnDegree;
    bool mTurnAlongGround;
    sead::FixedSafeString<32> mWalkAction;
    sead::FixedSafeString<32> mTurnAction;
};
static_assert(sizeof(WalkerStateRailMoveParam) == 0x90);

/** @brief Returns to a rail and walks back and forth along it. */
class WalkerStateRailMove : public al::ActorStateBase {
public:
    WalkerStateRailMove(al::LiveActor* pHost, const WalkerStateParam* pParam,
                        const WalkerStateRailMoveParam* pRailMoveParam);
    /** @brief Destroys the rail walking state. */
    ~WalkerStateRailMove() override = default;
    void appear() override;
    void kill() override;
    void setPositionToStart();
    void setClippingInfoOrigin();
    void setClippingInfoRail();
    void exeReturnToRail();
    void exeRailMove();
    void exeTurn();
private:
    const WalkerStateParam* mParam;
    const WalkerStateRailMoveParam* mRailMoveParam;
    sead::Vector3f mTurnDir = sead::Vector3f(0.0f, 0.0f, 0.0f);
    sead::Vector3f mRailCenter = sead::Vector3f(0.0f, 0.0f, 0.0f);
    f32 mRailClippingRadius = 0.0f;
    f32 mOriginalClippingRadius = 0.0f;
};
static_assert(sizeof(WalkerStateRailMove) == 0x50);
