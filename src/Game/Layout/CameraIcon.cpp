#include "Layout/CameraIcon.hpp"

#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"

namespace {
const char* const sOwnerActions[] = {"BrigadeP1", "BrigadeP2", "BrigadeP3", "BrigadeP4"};
NERVE_DECL(CameraIcon, Appear);
NERVE_DECL(CameraIcon, SwitchOwner);
NERVE_DECL(CameraIcon, Wait);
NERVE_DECL(CameraIcon, End);
NERVE_DECL(CameraIcon, NoCamera);
NERVES_MAKE_NOSTRUCT(CameraIcon, Appear, SwitchOwner, Wait, End, NoCamera)
}

/** @brief Creates an initially hidden camera indicator.
 * @param rInfo Layout initialization context.
 * @param isOwnerChangeEnabled Whether camera ownership can change. */
CameraIcon::CameraIcon(const al::LayoutInitInfo& rInfo, bool isOwnerChangeEnabled)
    : al::LayoutActor("CameraIcon Layout"), mIsOwnerChangeEnabled(isOwnerChangeEnabled) {
    al::initLayoutActor(this, rInfo, "CameraIcon", nullptr);
    initNerve(&NrvCameraIconAppear, 0);
    kill();
}

/** @brief Selects a player and starts the owner-change animation when ready.
 * @param ownerId Player index, from zero through three. */
void CameraIcon::setCameraOwnerID(int ownerId) {
    if (ownerId < 0 || ownerId >= 4 || !mIsOwnerChangeEnabled || mCameraOwnerId == ownerId)
        return;
    mCameraOwnerId = ownerId;
    if (!al::isNerve(this, &NrvCameraIconAppear))
        al::setNerve(this, &NrvCameraIconSwitchOwner);
}

/** @brief Reveals the indicator if it is currently hidden. */
void CameraIcon::startAppear() {
    if (isAlive())
        return;
    al::setNerve(this, &NrvCameraIconAppear);
    al::startAction(this, "Hide", nullptr);
    appear();
}

/** @brief Starts hiding an idle indicator. */
void CameraIcon::startEnd() {
    if (isAlive() && al::isNerve(this, &NrvCameraIconWait))
        al::setNerve(this, &NrvCameraIconEnd);
}

/** @brief Restores the idle camera animation once the initial feedback has played. */
void CameraIcon::reset() {
    if (!isAlive() || al::isNerve(this, &NrvCameraIconAppear))
        return;
    if (al::isNerve(this, &NrvCameraIconNoCamera) && al::isLessEqualStep(this, 1))
        return;
    al::startAction(this, mIsBrigade ? "Brigade" : "Normal", "Camera");
    al::setNerve(this, &NrvCameraIconWait);
}

/** @brief Shows feedback that camera control is unavailable. */
void CameraIcon::setCameraNoIcon() {
    if (!isAlive()) {
        al::startAction(this, "Hide", nullptr);
        al::startAction(this, mIsBrigade ? "Brigade" : "Normal", "Camera");
        appear();
    }
    if (!al::isNerve(this, &NrvCameraIconNoCamera))
        al::setNerve(this, &NrvCameraIconNoCamera);
}

/** @brief Plays the appearance animation, then selects the pending camera owner. */
void CameraIcon::exeAppear() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Appear", nullptr);
        al::startAction(this, mIsBrigade ? "Brigade" : "Normal", "Camera");
    }
    if (al::isActionEnd(this, nullptr) && al::isActionEnd(this, "Camera")) {
        if (mCameraOwnerId >= 0)
            al::setNerve(this, &NrvCameraIconSwitchOwner);
        else
            al::setNerve(this, &NrvCameraIconWait);
    }
}

/** @brief Holds the idle camera indicator. */
void CameraIcon::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait", nullptr);
        al::startAction(this, mIsBrigade ? "Brigade" : "Normal", "Camera");
    }
}

/** @brief Plays the unavailable-camera feedback and returns to idle. */
void CameraIcon::exeNoCamera() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "No", nullptr);
        al::startAction(this, mIsBrigade ? "Brigade" : "Normal", "Camera");
    }
    if (al::isActionEnd(this, nullptr))
        al::setNerve(this, &NrvCameraIconWait);
}

/** @brief Hides the indicator after its end animation. */
void CameraIcon::exeEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "End", nullptr);
        al::startAction(this, mIsBrigade ? "Brigade" : "Normal", "Camera");
    }
    if (al::isActionEnd(this, nullptr))
        kill();
}

/** @brief Changes to the selected player's camera indicator. */
void CameraIcon::exeSwitchOwner() {
    if (al::isFirstStep(this)) {
        mIsBrigade = true;
        al::startAction(this, "BrigadeChange", nullptr);
        al::startAction(this, "Brigade", "Camera");
    }
    if (al::isActionEnd(this, nullptr)) {
        al::startAction(this, sOwnerActions[mCameraOwnerId], "Brigade");
        al::setNerve(this, &NrvCameraIconWait);
    }
}


