#include "Project/Effect/Core/EffectUtil.hpp"
#include "Enemy/SuperBowserFireFlamePlessieChase.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Math/MatrixUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "MapObj/DisasterModeController.hpp"
#include "MapObj/Fury/InkPuddle.hpp"
#include "Enemy/SuperBowser.hpp"
#include "Boss/SuperBowserPlessieChaseFireFlameState.hpp"
namespace {
NERVE_DECL(SuperBowserFireFlamePlessieChase, Wait);
NERVE_DECL(SuperBowserFireFlamePlessieChase, Explode);
NERVE_DECL(SuperBowserFireFlamePlessieChase, Attack);
NERVE_DECL(SuperBowserFireFlamePlessieChase, LandStart);
NERVE_DECL(SuperBowserFireFlamePlessieChase, Land);
NERVE_DECL(SuperBowserFireFlamePlessieChase, LandEnd);
NERVES_MAKE_NOSTRUCT(SuperBowserFireFlamePlessieChase, Wait, Explode, Attack, LandStart, Land, LandEnd)
}
/** @brief Creates a Fury Bowser flame. @param pName Actor name. */
SuperBowserFireFlamePlessieChase::SuperBowserFireFlamePlessieChase(const char* pName, SuperBowserPlessieChaseFireFlameState* pState) : al::LiveActor(pName), mState(pState) {}
/** @brief Destroys the flame. */
SuperBowserFireFlamePlessieChase::~SuperBowserFireFlamePlessieChase() = default;
/** @brief Initializes the flame in its inactive state. @param rInfo Initialization data. */
void SuperBowserFireFlamePlessieChase::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "SuperBowserFlamePlessieChase", nullptr);
    al::initNerve(this, &NrvSuperBowserFireFlamePlessieChaseWait, 0);
    if (mState->getHost()->isInkPuddleEnabled()) {
        mInkPuddle = new InkPuddle("InkPuddle");
        al::initCreateActorNoPlacementInfo(mInkPuddle, rInfo);
        mInkPuddle->makeActorDead();
    }
    makeActorDead();
}
/** @brief Shows the flame. */
void SuperBowserFireFlamePlessieChase::appear() { al::LiveActor::appear(); }
/** @brief Disables sensors and removes the flame. */
void SuperBowserFireFlamePlessieChase::kill() { al::invalidateHitSensors(this); al::LiveActor::kill(); }
/** @brief Launches the flame using a position, direction, and gravity setting. */
void SuperBowserFireFlamePlessieChase::startAttack(const sead::Vector3f& pos, const sead::Vector3f& dir, const sead::Vector3f& up, float effectScale, float speed, float disappearOffset, bool useGravity, float gravity, bool isFirstShot) {
    appear();
    al::resetPosition(this, pos, false);
    sead::Matrix34f mtx;
    al::makeMtxFrontUpPos(&mtx, dir, up, pos);
    al::updatePoseMtx(this, &mtx);
    al::invalidateClipping(this);
    al::setScaleAll(this, mState->getParam().flameEffectScale);
    if (useGravity) al::setVelocity(this, dir);
    else al::setVelocity(this, dir * mState->getParam().flameSpeed);
    mUseGravity = useGravity;
    mGravity = gravity;
    mIsFirstShot = isFirstShot;
    mDisappearOffset = disappearOffset;
    auto* controller = DisasterModeController::tryGetController(this);
    if (controller && controller->getSuperBowser() && controller->getSuperBowser()->getCurrentSpawnInfo()) {
        mBaseHeight = controller->getSuperBowser()->getCurrentSpawnInfo()->mTrans.y;
        if (controller->getSuperBowser()->getCurrentSpawnInfo()->mUsePreviousHeight)
            mBaseHeight = controller->getSuperBowser()->getPreviousSpawnInfo()->mTrans.y;
    }
    al::setNerve(this, &NrvSuperBowserFireFlamePlessieChaseAttack);
}
/** @brief Stops sounds and effects and removes the flame. */
void SuperBowserFireFlamePlessieChase::killForce() {
    al::tryStopSe(this, "PgFlyStart");
    al::tryStopSe(this, "PgFlyEnd");
    al::tryKillEmitterAndParticleAll(this);
    al::validateClipping(this);
    kill();
}
/** @brief Keeps the flame inactive. */
void SuperBowserFireFlamePlessieChase::exeWait() {}
/** @brief Moves the flame until impact or timeout. */
void SuperBowserFireFlamePlessieChase::exeAttack() {
    if (al::isFirstStep(this)) {
        if (!mState->isRingSePlaying() && !mState->isSweepSePlaying()) al::startSe(this, "PgFlyStart");
        al::startAction(this, "Wait");
        al::validateHitSensor(this, "Attack");
        if (al::isAlive(mInkPuddle)) mInkPuddle->quickEnd();
    }
    al::setEffectParticleScale(this, "Wait", mState->getParam().flameEffectScale);
    al::setEffectEmitterScale(this, "Wait", sead::Vector3f::ones * mState->getParam().flameEffectScale);
    if (mUseGravity) al::addVelocityToGravity(this, mGravity);
    if (al::isCollided(this) || al::isGreaterEqualStep(this, 250)) {
        al::setNerve(this, &NrvSuperBowserFireFlamePlessieChaseExplode);
    } else {
        if (!mState->isRingSePlaying() && !mState->isSweepSePlaying()) al::holdSe(this, "PgFlyLv");
        if (al::getTrans(this).y < mBaseHeight + mDisappearOffset) al::setNerve(this, &NrvSuperBowserFireFlamePlessieChaseExplode);
    }
}
/** @brief Plays impact effects and removes the flame. */
void SuperBowserFireFlamePlessieChase::exeExplode() {
    if (!mState->isRingSePlaying() && !mState->isSweepSePlaying()) al::startSe(this, "PgFlyEnd");
    al::startHitReaction(this, "死亡");
    al::validateClipping(this);
    kill();
}
/** @brief Expands the ground fire attack radius. */
void SuperBowserFireFlamePlessieChase::exeLandStart() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "LandStart");
        al::invalidateHitSensor(this, "Attack");
        al::validateHitSensor(this, "Land");
    }
    float frame = al::getActionFrame(this);
    float max = al::getActionFrameMax(this, "LandStart");
    al::setSensorRadius(this, "Land", al::lerpValue(frame / max, 100.0f, 4000.0f));
    if (al::isActionEnd(this)) {
        al::setSensorRadius(this, "Land", 4000.0f);
        al::setNerve(this, &NrvSuperBowserFireFlamePlessieChaseLand);
    }
}
/** @brief Keeps ground fire active for a limited time. */
void SuperBowserFireFlamePlessieChase::exeLand() {
    if (al::isFirstStep(this)) al::startAction(this, "LandLoop");
    if (al::isGreaterEqualStep(this, 300)) al::setNerve(this, &NrvSuperBowserFireFlamePlessieChaseLandEnd);
}
/** @brief Shrinks and removes ground fire. */
void SuperBowserFireFlamePlessieChase::exeLandEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "LandEnd");
    }
    float frame = al::getActionFrame(this);
    float max = al::getActionFrameMax(this, "LandEnd");
    al::setSensorRadius(this, "Land", al::lerpValue(frame / max, 4000.0f, 0.0f));
    if (al::isActionEnd(this)) {
        al::setSensorRadius(this, "Land", 0.0f);
        kill();
    }
}
/** @brief Handles attacks that extinguish the flame. @return Whether the message was handled. */
bool SuperBowserFireFlamePlessieChase::receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) { return false; }
/** @brief Applies attacks to actors inside the flame sensors. */
void SuperBowserFireFlamePlessieChase::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isNerve(this, &NrvSuperBowserFireFlamePlessieChaseExplode)) return;
    if (al::isNerve(this, &NrvSuperBowserFireFlamePlessieChaseAttack) && al::isGreaterEqualStep(this, 45) && (al::isSensorPlayer(pOther) || al::isSensorRide(pOther))) {
        if (al::sendMsgEnemyAttack(pOther, pSelf)) {
            auto* controller = DisasterModeController::tryGetController(this);
            if (controller && controller->getSuperBowser()) controller->getSuperBowser()->notifyPlayerHit();
            al::setNerve(this, &NrvSuperBowserFireFlamePlessieChaseExplode);
        }
        return;
    }
    if ((al::isNerve(this, &NrvSuperBowserFireFlamePlessieChaseLandStart) || al::isNerve(this, &NrvSuperBowserFireFlamePlessieChaseLand) || al::isNerve(this, &NrvSuperBowserFireFlamePlessieChaseLandEnd)) && al::isSensorPlayer(pOther) && al::isSensorName(pOther, "Foot")) {
        float radius = al::getSensorRadius(pSelf);
        const sead::Vector3f& otherRef = al::getSensorPos(pOther);
        const sead::Vector3f& selfRef = al::getSensorPos(pSelf);
        sead::Vector3f delta = otherRef - selfRef;
        delta.y -= al::getSensorRadius(pOther);
        if (delta.y < 0.0f && delta.squaredLength() < radius * radius) al::sendMsgGigaEnemyAttack(pOther, pSelf);
    }
}
