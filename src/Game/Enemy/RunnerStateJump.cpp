#include "Enemy/RunnerStateJump.hpp"
#include "Enemy/Runner.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"

namespace {
NERVE_DECL(RunnerStateJump, JumpSign)
NERVE_DECL(RunnerStateJump, JumpStart)
NERVE_DECL(RunnerStateJump, Jump)
NERVE_DECL(RunnerStateJump, JumpEnd)
NERVES_MAKE_NOSTRUCT(RunnerStateJump, JumpSign, JumpStart, Jump, JumpEnd)
}

/** @brief Initializes the standard jump action names. */
RunnerStateJumpParam::RunnerStateJumpParam()
    : signAction("JumpSign"), startAction("JumpStart"), jumpAction("Jump"), endAction("JumpEnd") {}

/** @brief Initializes custom jump action names.
 * @param pSign Anticipation action. @param pStart Takeoff action. @param pJump Airborne action. @param pEnd Landing action.
 */
RunnerStateJumpParam::RunnerStateJumpParam(const char* pSign, const char* pStart, const char* pJump, const char* pEnd)
    : signAction(pSign), startAction(pStart), jumpAction(pJump), endAction(pEnd) {}

/** @brief Creates the jump state. @param pRunner Runner actor. @param pParam Jump action names. */
RunnerStateJump::RunnerStateJump(Runner* pRunner, const RunnerStateJumpParam* pParam)
    : al::ActorStateBase("State[\u30b8\u30e3\u30f3\u30d7]", pRunner), mRunner(pRunner), mParam(pParam) {}
/** @brief Initializes the anticipation nerve. */
void RunnerStateJump::init() { initNerve(&NrvRunnerStateJumpJumpSign, 0); }
/** @brief Starts jump anticipation. */
void RunnerStateJump::appear() {
    al::ActorStateBase::appear();
    al::setNerve(this, &NrvRunnerStateJumpJumpSign);
}

/** @brief Holds the actor on the ground until takeoff. */
void RunnerStateJump::exeJumpSign() {
    if (al::isFirstStep(this)) { al::startAction(mRunner, mParam->signAction.cstr()); }
    mRunner->trySetTransOnGround();
    if ((mRunner->isImmediateJump() && al::isGreaterEqualStep(this, 0)) || al::isActionEnd(mRunner)) {
        al::setNerve(this, &NrvRunnerStateJumpJumpStart);
    }
}

/** @brief Applies the initial jump impulse and takeoff gravity. */
void RunnerStateJump::exeJumpStart() {
    if (al::isFirstStep(this)) {
        al::startAction(mRunner, mParam->startAction.cstr());
        al::setVelocityY(mRunner, 18.0f);
        return;
    }
    al::addVelocityY(mRunner, -0.75f);
    al::scaleVelocityY(mRunner, 0.99f);
    if (al::isActionEnd(mRunner)) { al::setNerve(this, &NrvRunnerStateJumpJump); }
}

/** @brief Slows the jump near its apex and detects landing. */
void RunnerStateJump::exeJump() {
    if (al::isFirstStep(this)) { al::startAction(mRunner, mParam->jumpAction.cstr()); }
    f32 velocityY = al::getVelocity(mRunner).y;
    al::addVelocityY(mRunner, velocityY > -3.0f && velocityY < 3.0f ? -0.15f : -0.75f);
    al::scaleVelocityY(mRunner, 0.99f);
    if (mRunner->isGrounding()) {
        mRunner->trySetTransOnGround();
        al::setVelocityZero(mRunner);
        al::setNerve(this, &NrvRunnerStateJumpJumpEnd);
    }
}

/** @brief Plays the landing reaction and finishes the jump. */
void RunnerStateJump::exeJumpEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(mRunner, mParam->endAction.cstr());
        al::startHitReactionOnGround(mRunner);
    }
    mRunner->trySetTransOnGround();
    if (al::isActionEnd(mRunner)) { kill(); }
}
