#include "Boss/KoopaChaseStateProvocation.hpp"
#include "Boss/KoopaChase.hpp"
#include "Boss/KoopaChaseKoopa.hpp"
#include "Boss/KoopaChaseFunction.hpp"
#include "Boss/KoopaChaseStateFire.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"

namespace {
NERVE_DECL(KoopaChaseStateProvocation, Wait);
NERVE_DECL(KoopaChaseStateProvocation, Provocation);
NERVE_DECL(KoopaChaseStateProvocation, Fire);
NERVES_MAKE_STRUCT(KoopaChaseStateProvocation, Wait, Provocation, Fire)
}

/**
 * @brief Creates a provocation state with a single-fireball child state.
 * @param pHost Chase actor controlled by this state.
 * @param rInfo Actor initialization information.
 */
KoopaChaseStateProvocation::KoopaChaseStateProvocation(KoopaChase* pHost,
    const al::ActorInitInfo& rInfo)
    : al::NerveStateBase("クッパチェイスState[挑発]"), mHost(pHost) {
    initNerve(&NrvKoopaChaseStateProvocation.Provocation, 1);
    mStateFire = new KoopaChaseStateFire(mHost, rInfo, true);
    mStateFire->setFireNum(1);
    al::initNerveState(this, mStateFire, &NrvKoopaChaseStateProvocation.Fire, "State[火を吐く]");
}

/** @brief Activates the state and starts provoking. */
void KoopaChaseStateProvocation::appear() {
    al::NerveStateBase::appear();
    al::setNerve(this, &NrvKoopaChaseStateProvocation.Provocation);
}

/**
 * @brief Ends the state when neither provoking nor breathing fire.
 * @param isKeepProvoking Whether the state must stay active.
 * @return Whether the state ended.
 */
bool KoopaChaseStateProvocation::tryEnd(bool isKeepProvoking) {
    if (isKeepProvoking) { return false; }
    if (al::isNerve(this, &NrvKoopaChaseStateProvocation.Provocation)) { return false; }
    if (al::isNerve(this, &NrvKoopaChaseStateProvocation.Fire)) { return false; }
    kill();
    return true;
}

/** @brief Waits two seconds, alternating fire and provocation when enabled. */
void KoopaChaseStateProvocation::exeWait() {
    if (al::isGreaterEqualStep(this, 120)) {
        if (mIsFireEnabled && (mProvocationCount++ % 2) == 0) {
            al::setNerve(this, &NrvKoopaChaseStateProvocation.Fire);
        } else {
            al::setNerve(this, &NrvKoopaChaseStateProvocation.Provocation);
        }
    }
}

/** @brief Plays Koopa's provocation while the chase actor keeps its run pose. */
void KoopaChaseStateProvocation::exeProvocation() {
    if (al::isFirstStep(this)) {
        al::startAction(KoopaChaseFunction::getKoopa(mHost), "Provocation");
        if (mIsClearInterpole) {
            al::clearSklAnimInterpole(KoopaChaseFunction::getKoopa(mHost));
            al::clearSklAnimInterpole(mHost);
            mIsClearInterpole = false;
        }
        al::startAction(mHost, KoopaChaseFunction::getAnimNameRun(mHost));
    }
    if (al::isActionEnd(KoopaChaseFunction::getKoopa(mHost))) {
        al::startAction(KoopaChaseFunction::getKoopa(mHost), "Wait");
        al::setNerve(this, &NrvKoopaChaseStateProvocation.Wait);
    }
}

/** @brief Runs the fire child state and restores the waiting animation. */
void KoopaChaseStateProvocation::exeFire() {
    if (al::updateNerveStateAndNextNerve(this, &NrvKoopaChaseStateProvocation.Wait)) {
        al::startAction(KoopaChaseFunction::getKoopa(mHost), "Wait");
    }
}
