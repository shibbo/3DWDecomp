#include "Enemy/KuribonStateReverse.hpp"
#include "Enemy/WalkerStateFunction.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Nerve/NerveKeeper.hpp"
#include "Library/Nerve/NerveSetup.hpp"

namespace {
const KuribonStateReverseParam sDefaultParam;
NERVE_DECL(KuribonStateReverse, Start)
NERVE_DECL(KuribonStateReverse, Loop)
NERVE_DECL(KuribonStateReverse, Recover)
NERVE_DECL(KuribonStateReverse, Swoon)
NERVE_DECL(KuribonStateReverse, Land)
NERVES_MAKE_NOSTRUCT(KuribonStateReverse, Start, Loop, Recover, Swoon, Land)
}

/** @brief Creates the flipped-over state.
 * @param pHost Kuribon actor. @param pParam Walking physics. @param pReverseParam Flipped-over physics, or nullptr for defaults.
 */
KuribonStateReverse::KuribonStateReverse(al::LiveActor* pHost, const WalkerStateParam* pParam,
                                       const KuribonStateReverseParam* pReverseParam)
    : al::ActorStateBase("\u30af\u30ea\u30dc\u30f3\u306e\u3072\u3063\u304f\u308a\u8fd4\u308a\u72b6\u614b", pHost),
      mReverseParam(pReverseParam), mWalkerParam(pParam) {
    if (!mReverseParam) { mReverseParam = &sDefaultParam; }
    initNerve(&NrvKuribonStateReverseStart, 0);
}

/** @brief Resumes an interrupted state or starts a new reversal. */
void KuribonStateReverse::appear() {
    al::ActorStateBase::appear();
    if (mResumeNerve) {
        al::setNerve(this, mResumeNerve);
        mResumeNerve = nullptr;
    } else {
        al::setNerve(this, &NrvKuribonStateReverseStart);
        al::invalidateClipping(mHostActor);
    }
}

/** @brief Saves the current state for a later re-entry. */
void KuribonStateReverse::cancel() { mResumeNerve = getNerveKeeper()->getCurrentNerve(); }
/** @brief Tests whether the actor is still airborne. @return Whether it is in the start or loop state. */
bool KuribonStateReverse::isInAir() const {
    return al::isNerve(this, &NrvKuribonStateReverseStart) || al::isNerve(this, &NrvKuribonStateReverseLoop);
}
/** @brief Tests whether the actor is recovering. @return Whether the recovery animation is active. */
bool KuribonStateReverse::isRecover() const { return al::isNerve(this, &NrvKuribonStateReverseRecover); }

/** @brief Applies airborne reversed movement and detects landing. */
void KuribonStateReverse::exeStart() {
    if (al::isFirstStep(this)) { al::tryStartActionIfNotPlaying(mHostActor, "ReverseStart"); }
    if (al::isOnGround(mHostActor, 0, 0.0f)) {
        al::validateClipping(mHostActor);
        al::setNerve(this, &NrvKuribonStateReverseLand);
        return;
    }
    al::addVelocityToGravity(mHostActor, mReverseParam->gravity);
    al::scaleVelocity(mHostActor, mReverseParam->airFriction);
    if (al::isActionEnd(mHostActor)) { al::setNerve(this, &NrvKuribonStateReverseLoop); }
}

/** @brief Applies airborne reversed movement and detects landing. */
void KuribonStateReverse::exeLoop() {
    if (al::isFirstStep(this)) { al::tryStartActionIfNotPlaying(mHostActor, "ReverseLoop"); }
    if (al::isOnGround(mHostActor, 0, 0.0f)) {
        al::validateClipping(mHostActor);
        al::setNerve(this, &NrvKuribonStateReverseLand);
        return;
    }
    al::addVelocityToGravity(mHostActor, mReverseParam->gravity);
    al::scaleVelocity(mHostActor, mReverseParam->airFriction);
}

/** @brief Stops the actor after its landing animation. */
void KuribonStateReverse::exeLand() {
    if (al::isFirstStep(this)) { al::tryStartActionIfNotPlaying(mHostActor, "ReverseLand"); }
    al::scaleVelocity(mHostActor, mReverseParam->groundFriction);
    if (al::isActionEnd(mHostActor)) {
        al::setVelocityZero(mHostActor);
        al::setNerve(this, &NrvKuribonStateReverseSwoon);
    }
}

/** @brief Keeps the actor stunned until its recovery timer expires. */
void KuribonStateReverse::exeSwoon() {
    if (al::isFirstStep(this)) { al::tryStartActionIfNotPlaying(mHostActor, "ReverseSwoon"); }
    WalkerStateFunction::calcPassiveMovement(mHostActor, mWalkerParam);
    if (al::isGreaterStep(this, mReverseParam->swoonTime)) { al::setNerve(this, &NrvKuribonStateReverseRecover); }
}

/** @brief Finishes the reversed state after recovery. */
void KuribonStateReverse::exeRecover() {
    if (al::isFirstStep(this)) { al::tryStartActionIfNotPlaying(mHostActor, "ReverseRecover"); }
    WalkerStateFunction::calcPassiveMovement(mHostActor, mWalkerParam);
    if (al::isActionEnd(mHostActor)) { kill(); }
}

/** @brief Initializes the default reversed movement settings. */
KuribonStateReverseParam::KuribonStateReverseParam()
    : gravity(1.0f), airFriction(0.98f), groundFriction(0.92f), swoonTime(180) {}

/** @brief Initializes custom reversed movement settings.
 * @param gravity Gravity acceleration. @param airFriction Air velocity multiplier.
 * @param groundFriction Ground velocity multiplier. @param swoonTime Stun duration.
 */
KuribonStateReverseParam::KuribonStateReverseParam(f32 gravity, f32 airFriction, f32 groundFriction, s32 swoonTime)
    : gravity(gravity), airFriction(airFriction), groundFriction(groundFriction), swoonTime(swoonTime) {}
