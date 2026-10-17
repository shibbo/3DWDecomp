#include "Enemy/ActorMicRumbler.hpp"

#include "Library/Audio/System/AudioVolumeCtrl.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/Movement/AnimScaleController.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"

namespace {
NERVE_DECL(ActorMicRumbler, Wait);
NERVE_DECL(ActorMicRumbler, Vibration);
NERVES_MAKE_NOSTRUCT(ActorMicRumbler, Wait, Vibration)

/**
 * @brief Builds the scale animation parameters used when the owner supplies none.
 * @return The default vibration parameters.
 */
al::AnimScaleParam createDefaultParam() {
    al::AnimScaleParam param;
    param._24 = 0.8f;
    param._28 = 10.0f;
    param._2c = 0.1f;
    return param;
}

al::AnimScaleParam sDefaultParam = createDefaultParam();
}  // namespace

/**
 * @brief Constructs a microphone rumbler for an actor.
 * @param pActor Actor whose scale is animated.
 * @param pParam Scale animation parameters, or nullptr to use the defaults.
 */
ActorMicRumbler::ActorMicRumbler(al::LiveActor* pActor, const al::AnimScaleParam* pParam)
    : al::NerveExecutor("LiveActorのマイク振動"), mActor(pActor), mParam(pParam) {
    initNerve(&NrvActorMicRumblerWait, 0);
    if (mParam == nullptr) {
        mParam = &sDefaultParam;
    }

    mAnimScaleController = new al::AnimScaleController(mParam);
    mAnimScaleController->setOriginalScale(al::getScale(mActor));
}

/** @brief Advances the rumbler's nerve. */
void ActorMicRumbler::update() {
    updateNerve();
}

/** @brief Stops the vibration, restores the actor's scale and returns to waiting. */
void ActorMicRumbler::stopAndReset() {
    mAnimScaleController->stopAndReset();
    al::setScale(mActor, mAnimScaleController->getScale());
    al::setNerve(this, &NrvActorMicRumblerWait);
}

/** @brief Waits for microphone input before starting to vibrate. */
void ActorMicRumbler::exeWait() {
    if (al::isMicInputOn(mActor)) {
        al::setNerve(this, &NrvActorMicRumblerVibration);
    }
}

/** @brief Vibrates the actor while microphone input continues; stops after a short silence. */
void ActorMicRumbler::exeVibration() {
    if (al::isMicInputOn(mActor)) {
        mSilentFrames = 0;
    } else if (mSilentFrames++ >= 5) {
        stopAndReset();
        return;
    }

    if (al::isFirstStep(this)) {
        mAnimScaleController->startVibration();
    }

    mAnimScaleController->update();
    al::setScale(mActor, mAnimScaleController->getScale());
}
