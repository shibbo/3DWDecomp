#include "Enemy/TuccondorGlassBlower.hpp"
#include "Enemy/TuccondorGlass.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Joint/JointControllerKeeper.hpp"
#include "Library/Screen/ScreenPointTarget.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include <cmath>

/**
 * @brief Initializes the detachable glasses and neck rotation controller.
 * @param pHost Tuccondor actor owning the glasses.
 * @param pGlass Detachable glasses actor.
 */
TuccondorGlassBlower::TuccondorGlassBlower(al::LiveActor* pHost, TuccondorGlass* pGlass)
    : mHost(pHost), mGlass(pGlass) {
    if (al::isExistJoint(pHost, "Neck03")) {
        al::initJointControllerKeeper(mHost, 5);
        al::initJointLocalRotator(mHost, &mJointRotation, "Neck03");
    }
}

/**
 * @brief Shakes the neck and launches available glasses when touched.
 * @param pMsg Incoming touch message.
 * @param pPointer Pointer supplying the touch position.
 * @param pTarget Screen target receiving the touch.
 * @return Whether the message was handled.
 */
bool TuccondorGlassBlower::tryRequestGlassBlow(const al::SensorMsg* pMsg,
                                             al::ScreenPointer* pPointer,
                                             al::ScreenPointTarget* pTarget) {
    if (!al::isMsgTouchAssistTrig(pMsg)) {
        return false;
    }
    mShakeTimer = 40;
    if (mGlass->isReady()) {
        sead::Vector3f position;
        al::calcJointPos(&position, mHost, "Glasses");
        al::setTrans(mGlass, position);
        const sead::Vector3f& rTrans = al::getTrans(mHost);
        sead::Vector3f direction = rTrans - al::getHitScreenPointTargetPos(pPointer);
        mGlass->appearBlow(direction);
        al::startHitReaction(mHost, "めがねとばし");
        al::setJointVisibility(mHost, "Glass", false);
    } else {
        al::startSe(mHost, "PgTouched", nullptr);
    }
    return true;
}

/** @brief Applies a decaying oscillation to the neck and resets it when finished. */
void TuccondorGlassBlower::update() {
    if (mShakeTimer > 0) {
        float angle = static_cast<float>(mShakeTimer % 10) * 360.0f / 10.0f;
        float amplitude = sinf(angle * 0.017453292f) * mShakeTimer / 40.0f;
        mJointRotation.setScale(sead::Vector3f(0.0f, 20.0f, 20.0f), amplitude);
        if (--mShakeTimer == 0) {
            mJointRotation = sead::Vector3f::zero;
        }
    }
}




