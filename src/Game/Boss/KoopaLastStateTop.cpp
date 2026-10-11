#include "Boss/KoopaLastStateTop.hpp"
#include "Boss/KoopaLastFunction.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Joint/JointAimInfo.hpp"
#include "Library/Joint/JointControllerKeeper.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Util/JointAimUtil.hpp"

namespace {
NERVE_DECL(KoopaLastStateTop, ClimbEnd);
NERVE_DECL(KoopaLastStateTop, Wait);
NERVE_DECL(KoopaLastStateTop, Shout);
NERVES_MAKE_NOSTRUCT(KoopaLastStateTop, ClimbEnd, Wait, Shout)
}

/**
 * @brief Creates the summit state and configures the neck's aiming limits.
 * @param pActor Koopa actor controlled by this state.
 */
KoopaLastStateTop::KoopaLastStateTop(al::LiveActor* pActor)
    : al::ActorStateBase("頂上", pActor) {
    mJointAimInfo = new al::JointAimInfo;
    mJointAimInfo->setBaseAimLocalDir(-sead::Vector3f::ey);
    mJointAimInfo->setEnableBackAim(false);
    mJointAimInfo->setLimitDegreeOval(5.0f, 5.0f, 45.0f, 45.0f);
    al::initJointAimController(mHostActor, mJointAimInfo, "Neck");
    initNerve(&NrvKoopaLastStateTopClimbEnd, 0);
}

/** @brief Activates the state at the end of the climb. */
void KoopaLastStateTop::appear() {
    al::NerveStateBase::appear();
    al::setNerve(this, &NrvKoopaLastStateTopClimbEnd);
}

/** @brief Finishes climbing and applies body explosion collision. */
void KoopaLastStateTop::exeClimbEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(mHostActor, "LastFrontClimbEnd");
        al::invalidateClipping(mHostActor);
    }
    KoopaLastFunction::explosionCollision(mHostActor, "Body");
    if (al::isActionEnd(mHostActor)) {
        al::setNerve(this, &NrvKoopaLastStateTopWait);
    }
}

/** @brief Aims toward players while waiting for the next shout. */
void KoopaLastStateTop::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(mHostActor, "LastFrontClimbWait");
        mWaitFrames = al::getRandom(3, 6) * 60;
    }
    JointAimUtil::updateEyeJointInfo(mHostActor, mJointAimInfo, 3000.0f, 0.05f);
    if (!al::isLessStep(this, mWaitFrames)) {
        al::setNerve(this, &NrvKoopaLastStateTopShout);
    }
}

/** @brief Plays a shout and returns to waiting when it ends. */
void KoopaLastStateTop::exeShout() {
    if (al::isFirstStep(this)) {
        al::startAction(mHostActor, "LastFrontClimbShout");
    }
    if (al::isActionEnd(mHostActor)) {
        al::setNerve(this, &NrvKoopaLastStateTopWait);
    }
}
