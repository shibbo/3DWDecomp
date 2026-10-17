#include "Project/Effect/Core/EffectUtil.hpp"
#include "Enemy/SuperBowserFireFlame.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Math/MatrixUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Util/ProjectMsgUtil.hpp"
namespace {
NERVE_DECL(SuperBowserFireFlame, Wait);
NERVE_DECL(SuperBowserFireFlame, Explode);
NERVE_DECL(SuperBowserFireFlame, Attack);
NERVE_DECL(SuperBowserFireFlame, LandStart);
NERVE_DECL(SuperBowserFireFlame, Land);
NERVE_DECL(SuperBowserFireFlame, LandEnd);
NERVES_MAKE_NOSTRUCT(SuperBowserFireFlame, Wait, Explode, Attack, LandStart, Land, LandEnd)
}
/** @brief Creates a Fury Bowser flame. @param pName Actor name. */
SuperBowserFireFlame::SuperBowserFireFlame(const char* pName) : al::LiveActor(pName) {}
/** @brief Destroys the flame. */
SuperBowserFireFlame::~SuperBowserFireFlame() = default;
/** @brief Initializes the flame in its inactive state. @param rInfo Initialization data. */
void SuperBowserFireFlame::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "SuperBowserFlame", nullptr);
    al::initNerve(this, &NrvSuperBowserFireFlameWait, 0);
    makeActorDead();
}
/** @brief Shows the flame. */
void SuperBowserFireFlame::appear() { al::LiveActor::appear(); }
/** @brief Disables sensors and removes the flame. */
void SuperBowserFireFlame::kill() { al::invalidateHitSensors(this); al::LiveActor::kill(); }
/** @brief Gets the horizontal launch speed. @return Launch speed. */
float SuperBowserFireFlame::getSpeedH() const { return 275.0f; }
/** @brief Launches the flame using a position, direction, and gravity setting. */
void SuperBowserFireFlame::startAttack(const sead::Vector3f& pos, const sead::Vector3f& dir, const sead::Vector3f& up, bool useGravity, float gravity) {
    appear();
    al::resetPosition(this, pos, false);
    sead::Matrix34f mtx;
    al::makeMtxFrontUpPos(&mtx, dir, up, pos);
    al::updatePoseMtx(this, &mtx);
    if (useGravity) al::setVelocity(this, dir);
    else al::setVelocity(this, dir * 275.0f);
    al::invalidateClipping(this);
    al::setNerve(this, &NrvSuperBowserFireFlameAttack);
    mUseGravity = useGravity;
    mGravity = gravity;
}
/** @brief Stops sounds and effects and removes the flame. */
void SuperBowserFireFlame::killForce() {
    al::tryStopSe(this, "PgFlyStart");
    al::tryStopSe(this, "PgFlyEnd");
    al::tryKillEmitterAndParticleAll(this);
    al::validateClipping(this);
    kill();
}
/** @brief Keeps the flame inactive. */
void SuperBowserFireFlame::exeWait() {}
/** @brief Moves the flame until impact or timeout. */
void SuperBowserFireFlame::exeAttack() {
    if (al::isFirstStep(this)) {
        al::startSe(this, "PgFlyStart");
        al::startAction(this, "Wait");
        al::validateHitSensor(this, "Attack");
    }
    if (mUseGravity) al::addVelocityToGravity(this, mGravity);
    al::holdSe(this, "PgFlyLv");
    if (al::isCollided(this) || al::isGreaterEqualStep(this, 600)) {
        if (al::isCollidedGround(this)) {
            sead::Vector3f pos = al::getCollidedGroundPos(this);
            if (pos.y < 100.0f) {
                pos.y = -1000.0f;
                auto* quat = al::getQuatPtr(this);
                al::makeQuatFrontNoSupport(quat, -sead::Vector3f::ey);
                al::setVelocityZero(this);
                al::setTrans(this, pos);
                al::setNerve(this, &NrvSuperBowserFireFlameLandStart);
                return;
            }
        }
        al::setNerve(this, &NrvSuperBowserFireFlameExplode);
    }
}
/** @brief Plays impact effects and removes the flame. */
void SuperBowserFireFlame::exeExplode() {
    al::startSe(this, "PgFlyEnd");
    al::startHitReaction(this, "死亡");
    al::validateClipping(this);
    kill();
}
/** @brief Expands the ground fire attack radius. */
void SuperBowserFireFlame::exeLandStart() {
    if (al::isFirstStep(this)) {
        al::startSe(this, "PgFlyEndLv");
        al::startAction(this, "LandStart");
        al::invalidateHitSensor(this, "Attack");
        al::validateHitSensor(this, "Land");
    }
    float frame = al::getActionFrame(this);
    float max = al::getActionFrameMax(this, "LandStart");
    al::setSensorRadius(this, "Land", al::lerpValue(frame / max, 100.0f, 4000.0f));
    if (al::isActionEnd(this)) {
        al::setSensorRadius(this, "Land", 4000.0f);
        al::setNerve(this, &NrvSuperBowserFireFlameLand);
    }
}
/** @brief Keeps ground fire active for a limited time. */
void SuperBowserFireFlame::exeLand() {
    if (al::isFirstStep(this)) al::startAction(this, "LandLoop");
    if (al::isGreaterEqualStep(this, 300)) al::setNerve(this, &NrvSuperBowserFireFlameLandEnd);
}
/** @brief Shrinks and removes ground fire. */
void SuperBowserFireFlame::exeLandEnd() {
    if (al::isFirstStep(this)) {
        al::tryStopSe(this, "PgFlyEndLv");
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
bool SuperBowserFireFlame::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isMsgPlayerDisregard(pMsg)) return true;
    if (al::isNerve(this, &NrvSuperBowserFireFlameExplode)) return false;
    if (al::isNerve(this, &NrvSuperBowserFireFlameAttack) && al::isGreaterEqualStep(this, 45) &&
        (al::isMsgGigaEnemyAttack(pMsg) || rc::isMsgBombBoundKickedAttack(pMsg)) && al::isSensorEnemyBody(pSelf)) {
        al::setNerve(this, &NrvSuperBowserFireFlameExplode);
        return true;
    }
    return false;
}
/** @brief Sends attack messages to contacted surfaces. */
void SuperBowserFireFlame::updateCollider() {
    al::LiveActor::updateCollider();
    if (al::isNerve(this, &NrvSuperBowserFireFlameAttack)) {
        auto* ground = al::tryGetCollidedGroundSensor(this);
        if (ground) al::sendMsgGigaEnemyAttack(ground, al::getHitSensor(this, "Attack"));
        auto* wall = al::tryGetCollidedWallSensor(this);
        if (wall) al::sendMsgGigaEnemyAttack(wall, al::getHitSensor(this, "Attack"));
        auto* ceiling = al::tryGetCollidedCeilingSensor(this);
        if (ceiling) al::sendMsgGigaEnemyAttack(ceiling, al::getHitSensor(this, "Attack"));
    }
}
/** @brief Applies attacks to actors inside the flame sensors. */
void SuperBowserFireFlame::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isNerve(this, &NrvSuperBowserFireFlameExplode)) return;
    if (al::isNerve(this, &NrvSuperBowserFireFlameAttack) && al::isGreaterEqualStep(this, 45)) {
        if (al::isSensorEnemyAttack(pOther) || al::isSensorBindableAll(pOther) || al::isSensorNpc(pOther)) return;
        if (al::sendMsgGigaEnemyAttack(pOther, pSelf)) {
            al::setNerve(this, &NrvSuperBowserFireFlameExplode);
            return;
        }
    } else if (al::isSensorEnemy(pOther) || al::isSensorPlayerType(pOther)) {
        if (al::sendMsgGigaEnemyAttack(pOther, pSelf) || al::isSensorPlayerType(pOther)) {
            al::setNerve(this, &NrvSuperBowserFireFlameExplode);
            return;
        }
    }
    if ((al::isNerve(this, &NrvSuperBowserFireFlameLandStart) || al::isNerve(this, &NrvSuperBowserFireFlameLand) || al::isNerve(this, &NrvSuperBowserFireFlameLandEnd)) && al::isSensorPlayer(pOther) && al::isSensorName(pOther, "Foot")) {
        float radius = al::getSensorRadius(pSelf);
        const sead::Vector3f& otherRef = al::getSensorPos(pOther);
        const sead::Vector3f& selfRef = al::getSensorPos(pSelf);
        sead::Vector3f delta = otherRef - selfRef;
        delta.y -= al::getSensorRadius(pOther);
        if (delta.y < 0.0f && delta.squaredLength() < radius * radius) al::sendMsgGigaEnemyAttack(pOther, pSelf);
    }
}
