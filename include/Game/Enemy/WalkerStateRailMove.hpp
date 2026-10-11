#pragma once

#include <basis/seadTypes.h>

#include "Library/Nerve/NerveStateBase.hpp"

struct WalkerStateParam;

/** @brief Tuning for WalkerStateRailMove (rail walking speed and actions). */
class WalkerStateRailMoveParam {
public:
    WalkerStateRailMoveParam();
    WalkerStateRailMoveParam(f32 speed);

private:
    alignas(8) u8 _0[0x90];
};

static_assert(sizeof(WalkerStateRailMoveParam) == 0x90);

/** @brief Walker state that walks along the actor's rail. */
class WalkerStateRailMove : public al::ActorStateBase {
public:
    WalkerStateRailMove(al::LiveActor* pHost, const WalkerStateParam* pParam,
                        const WalkerStateRailMoveParam* pRailMoveParam);

    void setPositionToStart();

private:
    u8 _20[0x30];
};

static_assert(sizeof(WalkerStateRailMove) == 0x50);
