#pragma once

#include <basis/seadTypes.h>

#include "Library/Nerve/NerveStateBase.hpp"

struct WalkerStateParam;

/** @brief Tuning for KuribonStateReverse. */
class KuribonStateReverseParam {
public:
    KuribonStateReverseParam();
    KuribonStateReverseParam(f32 recoverJumpSpeed, f32 airFriction, f32 groundFriction,
                             s32 swoonTime);

private:
    u8 _0[0x10];
};

static_assert(sizeof(KuribonStateReverseParam) == 0x10);

/** @brief State of a Kuribon flipped onto its back (start, loop, land, swoon, recover). */
class KuribonStateReverse : public al::ActorStateBase {
public:
    KuribonStateReverse(al::LiveActor* pHost, const WalkerStateParam* pParam,
                        const KuribonStateReverseParam* pReverseParam);

    void appear() override;
    void cancel();
    bool isInAir() const;
    bool isRecover() const;

private:
    u8 _20[0x18];
};

static_assert(sizeof(KuribonStateReverse) == 0x38);
