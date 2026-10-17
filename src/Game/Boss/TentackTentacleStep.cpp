#include "Boss/TentackTentacleStep.hpp"
#include "Boss/TentackTentacle.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Obj/PartsFunction.hpp"
#include "Library/Obj/CollisionObj.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Util/ProjectMsgUtil.hpp"
namespace {
NERVE_DECL(TentackTentacleStep, Wait);
NERVE_DECL(TentackTentacleStep, Break);
NERVE_DECL(TentackTentacleStep, BreakAir);
NERVE_DECL(TentackTentacleStep, Throw);
NERVE_DECL(TentackTentacleStep, Fall);
NERVE_DECL(TentackTentacleStep, BreakLand);
NERVES_MAKE_NOSTRUCT(TentackTentacleStep, Wait, Break, BreakAir, Throw, Fall, BreakLand)
}
/** @brief Creates a scaffold block attached to a tentacle. @param pName Actor name. @param pTentacle Host tentacle. @param pBasePos Base position. */
TentackTentacleStep::TentackTentacleStep(const char* pName, TentackTentacle* pTentacle, const sead::Vector3f* pBasePos) : al::LiveActor(pName), mTentacle(pTentacle), mBasePos(pBasePos) {}
/** @brief Destroys the scaffold block. */
TentackTentacleStep::~TentackTentacleStep() = default;
/** @brief Creates the scaffold and its climbing collision. @param rInfo Initialization data. */
void TentackTentacleStep::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "TentackTentacleStep", nullptr);
    al::initNerve(this, &NrvTentackTentacleStepWait, 0);
    auto* sensor = al::getHitSensor(this, "CollisionParts");
    mClimbWall = al::createCollisionObjMtx(this, rInfo, "ClimbWall", sensor, getBaseMtx(), nullptr);
    al::invalidateCollisionParts(mClimbWall);
    mClimbWall->makeActorDead();
    makeActorDead();
}
/** @brief Shows the scaffold and enables its collision. */
void TentackTentacleStep::appear() {
    al::LiveActor::appear();
    al::setNerve(this, &NrvTentackTentacleStepWait);
    al::startAction(this, mBreakable ? "WaitBreakable" : "Wait");
    al::validateShadow(this);
    al::validateCollisionParts(this);
    mClimbWall->appear();
}
/** @brief Appears as a breakable scaffold. */
void TentackTentacleStep::appearBreakable() { mBreakable = true; appear(); }
/** @brief Removes both collision actors and clears movement. */
void TentackTentacleStep::kill() {
    al::LiveActor::kill();
    mClimbWall->kill();
    al::setVelocityZero(this);
    mBreakable = false;
}
/** @brief Pushes players while the scaffold moves downward. @param pSelf Own sensor. @param pOther Target sensor. */
void TentackTentacleStep::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isSensorEnemyBody(pSelf) && al::isSensorPlayer(pOther) &&
        (al::isNerve(this, &NrvTentackTentacleStepThrow) || al::isNerve(this, &NrvTentackTentacleStepFall) || al::getVelocity(this).length() > 0.0f) && !(al::getVelocity(this).y > 0.0f)) al::sendMsgPush(pOther, pSelf);
}
/** @brief Handles fire, landing, and break requests. @return Whether handled. */
bool TentackTentacleStep::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isSensorMapObj(pOther) && al::isMsgEnemyAttackFire(pMsg) &&
        !al::isNerve(this, &NrvTentackTentacleStepBreak) && !al::isNerve(this, &NrvTentackTentacleStepBreakLand) && !al::isNerve(this, &NrvTentackTentacleStepBreakAir)) {
        al::setNerve(this, &NrvTentackTentacleStepBreakLand);
        al::startSe(this, "PgBurnAdd");
        return true;
    }
    if (al::isSensorCollision(pOther) && rc::isMsgTentackMagmaBallBreak(pMsg)) return true;
    if (al::isMsgFloorTouch(pMsg) && al::isValidCollisionParts(mClimbWall)) switchCollisionParts(false);
    if (mBreakable && !al::isNerve(this, &NrvTentackTentacleStepBreak) && !al::isNerve(this, &NrvTentackTentacleStepBreakLand) && !al::isNerve(this, &NrvTentackTentacleStepBreakAir) && al::isMsgFloorTouch(pMsg) && mTentacle->isEnableStepBreak()) {
        if (al::isNerve(this, &NrvTentackTentacleStepThrow) || al::isNerve(this, &NrvTentackTentacleStepFall) || al::getVelocity(this).length() > 0.0f) {
            al::setVelocityZero(this);
            al::setNerve(this, &NrvTentackTentacleStepBreakAir);
        } else al::setNerve(this, &NrvTentackTentacleStepBreak);
        return true;
    }
    return false;
}
/** @brief Starts the ground-break state. */
void TentackTentacleStep::setBreak() { al::setNerve(this, &NrvTentackTentacleStepBreakLand); }
/** @brief Selects climbing or normal collision. @param isTentacleStiff Whether to use climbing collision. */
void TentackTentacleStep::switchCollisionParts(bool isTentacleStiff) {
    if (isTentacleStiff) {
        al::validateCollisionParts(mClimbWall);
        al::invalidateCollisionParts(this);
    } else {
        al::validateCollisionParts(this);
        al::invalidateCollisionParts(mClimbWall);
    }
}
/** @brief Throws an intact scaffold upward. @param speed Jump speed. */
void TentackTentacleStep::startThrow(float speed) {
    if (al::isNerve(this, &NrvTentackTentacleStepBreak) || al::isNerve(this, &NrvTentackTentacleStepBreakLand) || al::isNerve(this, &NrvTentackTentacleStepBreakAir) || al::isNerve(this, &NrvTentackTentacleStepThrow)) return;
    al::setVelocityJump(this, speed);
    al::setNerve(this, &NrvTentackTentacleStepThrow);
}
/** @brief Starts falling for an intact scaffold. @param speed Initial vertical speed. */
void TentackTentacleStep::startFall(float speed) {
    if (al::isNerve(this, &NrvTentackTentacleStepBreak) || al::isNerve(this, &NrvTentackTentacleStepBreakLand) || al::isNerve(this, &NrvTentackTentacleStepBreakAir) || al::isNerve(this, &NrvTentackTentacleStepFall)) return;
    al::setVelocityJump(this, speed);
    al::setNerve(this, &NrvTentackTentacleStepFall);
}
/** @brief Checks whether the scaffold is alive and intact. @return Whether intact. */
bool TentackTentacleStep::isExistStep() const {
    return al::isAlive(this) && !al::isNerve(this, &NrvTentackTentacleStepBreak) && !al::isNerve(this, &NrvTentackTentacleStepBreakLand) && !al::isNerve(this, &NrvTentackTentacleStepBreakAir);
}
/** @brief Checks whether the scaffold is in its thrown state. @return Whether thrown. */
bool TentackTentacleStep::isThrow() const { return al::isNerve(this, &NrvTentackTentacleStepThrow); }
/** @brief Moves the scaffold until it returns to tentacle height. */
void TentackTentacleStep::exeThrow() {
    if (al::tryKillByDeathArea(this)) return;
    sead::Vector3f pos(0.0f,0.0f,0.0f);
    mTentacle->calcStepTrans(&pos);
    al::addVelocityToGravity(this, 1.25f);
    al::scaleVelocity(this, 0.99f);
    float nextY = al::getTrans(this).y + al::getVelocity(this).y;
    if (al::getVelocity(this).y < 0.0f && nextY < pos.y) {
        al::setVelocityZero(this);
        al::setNerve(this, &NrvTentackTentacleStepWait);
        al::startSe(this, "PgLand");
    }
}
/** @brief Keeps the scaffold at the tentacle attachment point. */
void TentackTentacleStep::exeWait() {
    sead::Vector3f pos(0.0f,0.0f,0.0f);
    mTentacle->calcStepTrans(&pos);
    al::setTrans(this, pos);
    al::resetPosition(this, false);
}
/** @brief Starts the tentacle bite and breaks the scaffold. */
void TentackTentacleStep::exeBreak() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Break");
        al::invalidateShadow(this);
    }
    if (al::isStep(this, 30)) mTentacle->tryStartBite();
    if (al::isStep(this, 60)) invalidateCollisionPartsLocal();
    if (al::isActionEnd(this)) kill();
}
/** @brief Disables both collision actors. */
void TentackTentacleStep::invalidateCollisionPartsLocal() {
    al::invalidateCollisionParts(this);
    al::invalidateCollisionParts(mClimbWall);
}
/** @brief Falls toward the base height. */
void TentackTentacleStep::exeFall() {
    al::addVelocityToGravity(this, 1.25f);
    al::scaleVelocity(this, 0.99f);
    if (al::tryKillByDeathArea(this)) return;
    if (al::getTrans(this).y + al::getVelocity(this).y < mBasePos->y) {
        al::setVelocityZero(this);
        al::setNerve(this, &NrvTentackTentacleStepBreakLand);
    }
}
/** @brief Breaks the scaffold on the ground. */
void TentackTentacleStep::exeBreakLand() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "BreakLand");
        invalidateCollisionPartsLocal();
        al::invalidateShadow(this);
    }
    if (al::isActionEnd(this)) kill();
}
/** @brief Breaks the scaffold in the air and starts a bite. */
void TentackTentacleStep::exeBreakAir() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "BreakLand");
        invalidateCollisionPartsLocal();
        al::invalidateShadow(this);
    }
    if (al::isStep(this, 30)) mTentacle->tryStartBite();
    if (al::isActionEnd(this)) kill();
}
