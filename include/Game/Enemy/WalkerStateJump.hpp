#pragma once

#include "Library/Nerve/NerveStateBase.hpp"
#include <prim/seadSafeString.h>

struct WalkerStateParam;

class WalkerStateJumpParam {
public:
    WalkerStateJumpParam();
    WalkerStateJumpParam(float speed, bool replaceVelocity);
    WalkerStateJumpParam(float speed, const char* pAction, bool replaceVelocity);
    WalkerStateJumpParam(float speed, const char* pStart, const char* pLoop,
                         const char* pEnd, bool replaceVelocity);

    float mJumpSpeed;
    bool mReplaceVelocity;
    bool mUseAnimation;
    sead::FixedSafeString<32> mActionStart;
    sead::FixedSafeString<32> mActionLoop;
    sead::FixedSafeString<32> mActionEnd;
};

class WalkerStateJump : public al::ActorStateBase {
public:
    WalkerStateJump(al::LiveActor* pHost, const WalkerStateParam* pParam,
                    const WalkerStateJumpParam* pJumpParam);
    /** @brief Destroys the jumping state. */
    ~WalkerStateJump() override = default;
    void appear() override;
    void exeJumpStart();
    void exeJumpLoop();
    void exeJumpEnd();

private:
    const WalkerStateParam* mParam;
    const WalkerStateJumpParam* mJumpParam;
};

static_assert(sizeof(WalkerStateJumpParam) == 0xb0);
static_assert(sizeof(WalkerStateJump) == 0x30);
