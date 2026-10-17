#pragma once

#include "Library/Nerve/NerveStateBase.hpp"
#include <prim/seadSafeString.h>

struct WalkerStateParam;

struct WalkerStateAttackSuccessParam {
    WalkerStateAttackSuccessParam();
    WalkerStateAttackSuccessParam(const char* pAction, bool isJump, float jumpSpeed);

    sead::FixedSafeString<32> mActionStart;
    sead::FixedSafeString<32> mActionLoop;
    sead::FixedSafeString<32> mActionEnd;
    bool mIsJump;
    float mJumpSpeed;
};

class WalkerStateAttackSuccess : public al::ActorStateBase {
public:
    WalkerStateAttackSuccess(al::LiveActor* pHost, const WalkerStateParam* pParam,
                             const WalkerStateAttackSuccessParam* pAttackParam);
    /** @brief Destroys the attack-success state. */
    ~WalkerStateAttackSuccess() override = default;
    void appear() override;
    void exeAttackSuccess();
    void exeAttackSuccessStart();
    void exeAttackSuccessLoop();
    void exeAttackSuccessEnd();

private:
    const WalkerStateParam* mParam;
    const WalkerStateAttackSuccessParam* mAttackParam;
};

static_assert(sizeof(WalkerStateAttackSuccessParam) == 0xb0);
static_assert(sizeof(WalkerStateAttackSuccess) == 0x30);
