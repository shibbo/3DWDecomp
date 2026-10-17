#pragma once
#include <prim/seadSafeString.h>
#include "Library/Nerve/NerveStateBase.hpp"
class Runner;

/** @brief Action names for the Runner's jump phases. */
struct RunnerStateJumpParam {
    RunnerStateJumpParam();
    RunnerStateJumpParam(const char* pSign, const char* pStart, const char* pJump, const char* pEnd);
    sead::FixedSafeString<32> signAction;
    sead::FixedSafeString<32> startAction;
    sead::FixedSafeString<32> jumpAction;
    sead::FixedSafeString<32> endAction;
};
static_assert(sizeof(RunnerStateJumpParam) == 0xe0);

/** @brief Controls jump anticipation, takeoff, flight, and landing. */
class RunnerStateJump : public al::ActorStateBase {
public:
    RunnerStateJump(Runner* pRunner, const RunnerStateJumpParam* pParam);
    void init() override;
    void appear() override;
    void exeJumpSign();
    void exeJumpStart();
    void exeJump();
    void exeJumpEnd();
private:
    Runner* mRunner;
    const RunnerStateJumpParam* mParam;
};
static_assert(sizeof(RunnerStateJump) == 0x30);
