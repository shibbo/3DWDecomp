#pragma once

#include <basis/seadTypes.h>

#include "Library/Nerve/NerveStateBase.hpp"

struct WalkerStateParam;

/** @brief Tuning for WalkerStateJump (jump speed and actions). */
class WalkerStateJumpParam {
public:
    WalkerStateJumpParam();
    WalkerStateJumpParam(f32 jumpSpeed, bool isUseAction);
    WalkerStateJumpParam(f32 jumpSpeed, const char* pJumpAction, bool isUseAction);
    WalkerStateJumpParam(f32 jumpSpeed, const char* pJumpAction, const char* pFallAction,
                         const char* pLandAction, bool isUseAction);

private:
    alignas(8) u8 _0[0xB0];
};

static_assert(sizeof(WalkerStateJumpParam) == 0xB0);

/**
 * @brief Walker state that makes the host jump.
 * @note Only what reconstructed code needs is declared so far.
 */
class WalkerStateJump : public al::ActorStateBase {
public:
    WalkerStateJump(al::LiveActor* pHost, const WalkerStateParam* pParam,
                    const WalkerStateJumpParam* pJumpParam);

private:
    u8 _20[0x30 - 0x20];
};

static_assert(sizeof(WalkerStateJump) == 0x30);
