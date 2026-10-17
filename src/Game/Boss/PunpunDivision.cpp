#include "Library/Actor/ActorInitInfo.hpp"
#include "Boss/PunpunDivision.hpp"
#include "Boss/Punpun.hpp"
#include "Boss/PunpunShuriken.hpp"
#include "Enemy/ActorJointLookController.hpp"
#include "Enemy/EnemyStateUtil.hpp"
#include "MapObj/ChameleonStateUtil.hpp"
#include "System/GameDataFunction.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
namespace {
NERVE_DECL(PunpunDivision, Appear);
NERVE_DECL(PunpunDivision, Divide);
NERVE_DECL(PunpunDivision, DivideAppear);
NERVE_DECL(PunpunDivision, ThrowWait);
NERVE_DECL(PunpunDivision, ThrowSign);
NERVE_DECL(PunpunDivision, Throw);
NERVE_DECL(PunpunDivision, ThrowEnd);
NERVE_DECL(PunpunDivision, AttackSuccess);
NERVES_MAKE_NOSTRUCT(PunpunDivision, Appear, Divide, DivideAppear, ThrowWait, ThrowSign, Throw, ThrowEnd, AttackSuccess)
const RenderMaterialIndirectParam sHiddenParam = {0.0f,0.0f,{1,1,1,1},{1,1,1,0},{0,0,0,1}};
const RenderMaterialIndirectParam sVisibleParam = {0.0f,0.1f,{1,1,1,0},{1,1,1,1},{0,0,0,1}};
}
/** @brief Creates a clone, its shuriken, and its material parameters. @param pName Actor name. */
PunpunDivision::PunpunDivision(const char* pName) : al::LiveActor(pName), mShuriken(new PunpunShuriken("プンプン[分身]の手裏剣")), mMaterialParam(new RenderMaterialIndirectParam) {}
/** @brief Destroys the clone. */
PunpunDivision::~PunpunDivision() = default;
/** @brief Initializes clone resources and its look controller. @param rInfo Initialization data. */
void PunpunDivision::init(const al::ActorInitInfo& rInfo) {
    if (rInfo.getActorSceneInfo().isSingleMode) al::initActorWithArchiveName(this, rInfo, "PunpunDivisionFur", nullptr);
    else al::initActorWithArchiveName(this, rInfo, "PunpunDivision", nullptr);
    al::initNerve(this, &NrvPunpunDivisionAppear, 0);
    mShuriken->initShurikenWithArchiveName(rInfo, "PunpunShurikenDivision");
    mLookController = Punpun::makePunpunLookController(this);
    auto* param = mMaterialParam;
    param->mBlendRate = sHiddenParam.mBlendRate;
    param->mIntensity = sHiddenParam.mIntensity;
    param->mVector0.e = sHiddenParam.mVector0.e;
    param->mVector1.e = sHiddenParam.mVector1.e;
    param->mVector2.e = sHiddenParam.mVector2.e;
    ChameleonStateUtil::setRenderMaterialIndirectParam(this, mMaterialParam);
    makeActorDead();
}
/** @brief Updates the joint look controller. */
void PunpunDivision::control() { mLookController->update(); }
/** @brief Removes the clone and its shuriken. */
void PunpunDivision::kill() {
    al::startHitReactionDisappear(this);
    al::LiveActor::kill();
    if (al::isAlive(mShuriken)) mShuriken->kill();
    mLookController->resetRotate(true);
}
/** @brief Attacks players and pushes single-mode objects. @param pSelf Own sensor. @param pOther Target sensor. */
void PunpunDivision::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isNerve(this, &NrvPunpunDivisionDivide)) return;
    if ((al::isSensorPlayer(pOther) || al::isSensorKoopaJr(pOther)) && al::isSensorEnemyAttack(pSelf)) {
        al::sendMsgPush(pOther, pSelf);
        if (al::sendMsgEnemyAttackForCrossoverSensor(pOther, pSelf)) {
            al::setNerve(this, &NrvPunpunDivisionAttackSuccess);
            return;
        }
    }
    if (GameDataFunction::isSingleMode(GameDataHolderAccessor(this)) && (al::isSensorHostName(pOther, "DoorKey") || al::isSensorHostName(pOther, "コウラ"))) al::sendMsgPush(pOther, pSelf);
}
/** @brief Handles attacks that destroy the clone. @return Whether the message is consumed. */
bool PunpunDivision::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isNerve(this, &NrvPunpunDivisionDivide)) return false;
    if (EnemyStateUtil::isMsgPressDownForCrossoverSensor(pMsg, pSelf, pOther) || EnemyStateUtil::isMsgBlowDown(pMsg) || al::isMsgKeyThrow(pMsg)) {
        al::startHitReactionDisappear(this);
        kill();
        return al::isMsgExplosion(pMsg) || al::isMsgPlayerFireBallAttack(pMsg);
    }
    return false;
}
/** @brief Waits for the clone to split off. */
void PunpunDivision::exeAppear() {}
/** @brief Reveals the clone during the splitting animation. */
void PunpunDivision::exeDivide() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "LightWait");
        al::invalidateShadow(this);
    }
    if (al::isStep(this, 1)) al::showModelIfHide(this);
}
/** @brief Interpolates material parameters as the clone appears. */
void PunpunDivision::exeDivideAppear() {
    updateShurikenPose();
    if (al::isLessStep(this, 70)) {
        float rate = al::calcNerveRate(this, 70);
        ChameleonStateUtil::updateIndirectParam(mMaterialParam, rate, &sHiddenParam, &sVisibleParam);
        ChameleonStateUtil::setRenderMaterialIndirectParam(this, mMaterialParam);
    }
    if (al::isStep(this, 70)) {
        ChameleonStateUtil::updateIndirectParam(mMaterialParam, &sVisibleParam);
        ChameleonStateUtil::setRenderMaterialIndirectParam(this, mMaterialParam);
    }
    al::setNerveAtActionEnd(this, &NrvPunpunDivisionThrowWait);
}
/** @brief Keeps the held shuriken aligned with the wrist joint. */
void PunpunDivision::updateShurikenPose() { al::updatePoseMtx(mShuriken, al::getJointMtxPtr(this, "WristR")); }
/** @brief Tracks the nearest player while holding the shuriken. */
void PunpunDivision::exeThrowWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "ThrowWait");
        al::startAction(mShuriken, "ThrowWait");
        ChameleonStateUtil::updateIndirectParam(mMaterialParam, &sVisibleParam);
        ChameleonStateUtil::setRenderMaterialIndirectParam(this, mMaterialParam);
    }
    mLookController->setLookAtNearestPlayer(-1.0f);
    updateShurikenPose();
}
/** @brief Turns toward the player before throwing. */
void PunpunDivision::exeThrowSign() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "ThrowSign");
        al::startAction(mShuriken, "ThrowSign");
        mLookController->stopLook();
    }
    al::turnToTarget(this, al::findNearestPlayerPos(this), Punpun::getThrowWaitTurnSpeed());
    updateShurikenPose();
    if (al::isActionEnd(this)) al::setNerve(this, &NrvPunpunDivisionThrow);
}
/** @brief Launches the shuriken forward. */
void PunpunDivision::exeThrow() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Throw");
        sead::Vector3f front(0.0f,0.0f,0.0f);
        al::calcFrontDir(&front, this);
        mShuriken->shoot(front, false);
    }
    if (al::isActionEnd(this)) al::setNerve(this, &NrvPunpunDivisionThrowEnd);
}
/** @brief Returns to the waiting animation and tracks players. */
void PunpunDivision::exeThrowEnd() {
    if (al::isFirstStep(this)) al::startAction(this, "ThrowEnd");
    if (al::isActionPlaying(this, "ThrowEnd") && al::isActionEnd(this)) al::startAction(this, "Wait");
    mLookController->setLookAtNearestPlayer(-1.0f);
}
/** @brief Plays the successful attack animation and removes the clone. */
void PunpunDivision::exeAttackSuccess() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "AttackHit");
        mLookController->stopLook();
    }
    if (al::isAlive(mShuriken)) updateShurikenPose();
    if (al::isActionEnd(this)) kill();
}
/** @brief Appears in the hidden splitting state. */
void PunpunDivision::startDivide() {
    appear();
    al::setNerve(this, &NrvPunpunDivisionDivide);
    al::hideModelIfShow(this);
}
/** @brief Starts the appearance animation and shows the held shuriken. */
void PunpunDivision::startDivideAppear() {
    al::setNerve(this, &NrvPunpunDivisionDivideAppear);
    al::startAction(this, "AppearDivision");
    auto* param = mMaterialParam;
    param->mBlendRate = sHiddenParam.mBlendRate;
    param->mIntensity = sHiddenParam.mIntensity;
    param->mVector0.e = sHiddenParam.mVector0.e;
    param->mVector1.e = sHiddenParam.mVector1.e;
    param->mVector2.e = sHiddenParam.mVector2.e;
    al::validateShadow(this);
    mShuriken->appear();
    al::resetPosition(mShuriken, al::getTrans(this), false);
    updateShurikenPose();
}
/** @brief Starts a throw when the clone is ready. */
void PunpunDivision::tryStartThrow() {
    if (al::isAlive(this) && (al::isNerve(this, &NrvPunpunDivisionDivideAppear) || al::isNerve(this, &NrvPunpunDivisionThrowWait))) {
        al::setNerve(this, &NrvPunpunDivisionThrowSign);
        mLookController->stopLook();
    }
}
