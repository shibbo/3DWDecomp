#include "Enemy/MeraWanwanTrack.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Connector/MtxConnector.hpp"
#include "Library/Collision/PartsConnectorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Math/MatrixUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Util/ProjectMsgUtil.hpp"

namespace {
NERVE_DECL(MeraWanwanTrack, Appear)
NERVE_DECL(MeraWanwanTrack, Wait)
NERVE_DECL(MeraWanwanTrack, Disappear)
NERVES_MAKE_NOSTRUCT(MeraWanwanTrack, Appear, Wait, Disappear)
}

/** @brief Creates a burning ground mark. @param pName Actor name. */
MeraWanwanTrack::MeraWanwanTrack(const char* pName) : al::LiveActor(pName) {}

/** @brief Initializes the mark and its surface connector. @param rInfo Placement information. */
void MeraWanwanTrack::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "MeraWanwanTrack", nullptr);
    al::initNerve(this, &NrvMeraWanwanTrackAppear, 0);
    mConnector = al::createMtxConnector(this);
    makeActorDead();
}

/** @brief Sends burn damage while the mark is active. @param pSelf Own sensor. @param pOther Contact sensor. */
void MeraWanwanTrack::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isNerve(this, &NrvMeraWanwanTrackWait)) {
        al::sendMsgEnemyAttack(pOther, pSelf);
        rc::sendMsgMeraWanwanAttack(pOther, pSelf);
    }
}

/** @brief Places a mark on a surface with random rotation. @param rTrans Position. @param rNormal Surface normal. @param pParts Surface collision parts, if any. */
void MeraWanwanTrack::start(const sead::Vector3f& rTrans, const sead::Vector3f& rNormal,
                           const al::CollisionParts* pParts) {
    al::setNerve(this, &NrvMeraWanwanTrackWait);
    al::makeQuatUpNoSupport(&mBaseQuat, rNormal);
    al::rotateQuatYDirDegree(&mBaseQuat, mBaseQuat, al::getRandomDegree());
    mBaseTrans.set(rTrans);
    al::resetPosition(this, rTrans, false);
    mConnector->clear();
    if (pParts)
        al::attachMtxConnectorToCollisionParts(mConnector, pParts);
    al::setNerve(this, &NrvMeraWanwanTrackWait);
    makeActorAppeared();
}

/** @brief Plays the appearance animation while following the surface. */
void MeraWanwanTrack::exeAppear() {
    if (al::isFirstStep(this))
        al::startAction(this, "Appear");
    al::connectPoseQT(this, mConnector, mBaseQuat, mBaseTrans);
    if (al::isActionEnd(this))
        al::setNerve(this, &NrvMeraWanwanTrackWait);
}

/** @brief Keeps the mark active for 130 steps. */
void MeraWanwanTrack::exeWait() {
    if (al::isFirstStep(this))
        al::startAction(this, "Wait");
    al::connectPoseQT(this, mConnector, mBaseQuat, mBaseTrans);
    if (al::isGreaterEqualStep(this, 130))
        al::setNerve(this, &NrvMeraWanwanTrackDisappear);
}

/** @brief Removes the mark after its disappearance animation. */
void MeraWanwanTrack::exeDisappear() {
    if (al::isFirstStep(this))
        al::startAction(this, "Disappear");
    al::connectPoseQT(this, mConnector, mBaseQuat, mBaseTrans);
    if (al::isActionEnd(this))
        kill();
}
