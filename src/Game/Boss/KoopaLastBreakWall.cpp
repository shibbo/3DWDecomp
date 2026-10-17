#include "Boss/KoopaLastBreakWall.hpp"
#include "Boss/KoopaLastFunction.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Joint/JointAimInfo.hpp"
#include "Library/Joint/JointDirectionInfo.hpp"
#include "Library/Joint/JointControllerKeeper.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Thread/Functor.hpp"
#include "Util/JointAimUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
namespace {
NERVE_DECL(KoopaLastBreakWall, InsideWall);
NERVE_DECL(KoopaLastBreakWall, BreakSign);
NERVE_DECL(KoopaLastBreakWall, BreakWall);
NERVE_DECL(KoopaLastBreakWall, Attack);
NERVE_DECL(KoopaLastBreakWall, AttackEnd);
NERVE_DECL(KoopaLastBreakWall, Wait);
NERVES_MAKE_NOSTRUCT(KoopaLastBreakWall, InsideWall, BreakSign, BreakWall, Attack, AttackEnd, Wait)
const char* const sWaitActions[] = {"BreakWall01Wait", "BreakWall02Wait", "BreakWall03Wait", "BreakWall04Wait"};
const char* const sSignActions[] = {"BreakWall01Sign", "BreakWall02Sign", "BreakWall03Sign", "BreakWall04Sign"};
const char* const sBreakActions[] = {"BreakWall01AttackStart", "BreakWall02AttackStart", "BreakWall03AttackStart", "BreakWall04AttackStart"};
const char* const sAttackActions[] = {"BreakWall01Attack", "BreakWall02Attack", "BreakWall03Attack", "BreakWall04Attack"};
const char* const sEndActions[] = {"BreakWall01End", "BreakWall02End", "BreakWall03End", "BreakWall04End"};
}
/** @brief Creates the wall-breaking Bowser actor. @param pName Actor name. */
KoopaLastBreakWall::KoopaLastBreakWall(const char* pName) : al::LiveActor(pName) {}
/** @brief Destroys the actor. */
KoopaLastBreakWall::~KoopaLastBreakWall() = default;
/** @brief Sets up the animation variant, switches, sensors, and joint aiming. @param rInfo Initialization data. */
void KoopaLastBreakWall::init(const al::ActorInitInfo& rInfo) {
    al::tryGetArg(&mAnimType, rInfo, "AnimType");
    if (mAnimType == 3) KoopaLastFunction::initActorKoopaLastCommon(this, rInfo, "BreakWallHip", nullptr);
    else KoopaLastFunction::initActorKoopaLastCommon(this, rInfo, "BreakWall", nullptr);
    al::listenStageSwitchOnStart(this, al::Functor(this, &KoopaLastBreakWall::start));
    al::listenStageSwitchOnStop(this, al::Functor(this, &KoopaLastBreakWall::stop));
    al::listenStageSwitchOnKill(this, al::Functor(this, &KoopaLastBreakWall::killBySwitch));
    mNeck = new al::JointAimInfo;
    al::rotateVectorDegreeZ(&mAimDir, 30.0f);
    mAimUp.setCross(sead::Vector3f::ez, mAimDir);
    mNeck->setBaseAimLocalDir(mAimDir);
    mNeck->setBaseSideLocalDir(sead::Vector3f::ez);
    mNeck->setBaseUpLocalDir(mAimUp);
    mNeck->setEnableBackAim(false);
    mNeck->setLimitDegreeOval(30.0f,30.0f,60.0f,30.0f);
    mNeck->setPowerRate(0.0f);
    al::initJointAimController(this, mNeck, "Neck");
    mEyeL = new al::JointAimInfo;
    mEyeL->setBaseAimLocalDir(sead::Vector3f::ex);
    mEyeL->setBaseSideLocalDir(sead::Vector3f::ez);
    mEyeL->setEnableBackAim(true);
    mEyeL->setLimitDegreeOval(8.0f,35.0f,5.0f,5.0f);
    mEyeL->setPowerRate(0.0f);
    mEyeL->setInterpoleRate(0.01f);
    al::initJointAimController(this, mEyeL, "EyeL");
    mEyeR = new al::JointAimInfo;
    mEyeR->setBaseAimLocalDir(sead::Vector3f::ex);
    mEyeR->setBaseSideLocalDir(sead::Vector3f::ez);
    mEyeR->setEnableBackAim(true);
    mEyeR->setLimitDegreeOval(35.0f,8.0f,5.0f,5.0f);
    mEyeR->setPowerRate(0.0f);
    mEyeR->setInterpoleRate(0.01f);
    al::initJointAimController(this, mEyeR, "EyeR");
    mFace = new al::JointDirectionInfo;
    mFace->setLocalBaseDir(sead::Vector3f::ey);
    mFace->setLocalRotateAxis(sead::Vector3f::ex);
    mFace->setWorldTargetDir(sead::Vector3f::ey);
    mFace->setPowerRate(0.0f);
    mFace->setLimitDegree(30.0f);
    al::initJointLocalDirController(this, mFace, "Face");
    al::initNerve(this, &NrvKoopaLastBreakWallInsideWall, 0);
    int animType = mAnimType;
    al::invalidateHitSensor(this, "Attack");
    if (animType == 3) {
        al::invalidateHitSensor(this, "Tail1");
        al::invalidateHitSensor(this, "Tail2");
        al::invalidateHitSensor(this, "Tail3");
    }
    makeActorAppeared();
}
/** @brief Starts breaking the wall if the actor has not stopped. */
void KoopaLastBreakWall::start() { if (!mStopped) al::setNerve(this, &NrvKoopaLastBreakWallBreakWall); }
/** @brief Prevents further attacks. */
void KoopaLastBreakWall::stop() { mStopped = true; }
/** @brief Stops and removes the actor. */
void KoopaLastBreakWall::killBySwitch() { mStopped = true; kill(); }
/** @brief Attacks players and nearby reaction objects. @param pSelf Own sensor. @param pOther Target sensor. */
void KoopaLastBreakWall::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isSensorEnemyBody(pSelf)) {
        if (!al::isSensorPlayer(pOther) || al::sendMsgPush(pOther, pSelf)) return;
    }
    if (al::isSensorEnemyAttack(pSelf)) {
        if (al::isSensorPlayer(pOther)) al::sendMsgEnemyAttack(pOther, pSelf);
        else if (!al::isSensorName(pSelf, "Attack")) rc::sendMsgKoopaLastReactionObj(pOther, pSelf);
    }
}
/** @brief Forwards common Bowser messages. @return Whether handled. */
bool KoopaLastBreakWall::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSelf, al::HitSensor* pOther) { return KoopaLastFunction::receiveMsgCommon(pMsg, pSelf, pOther); }
/** @brief Forwards pointer messages. @return Whether handled. */
bool KoopaLastBreakWall::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer, al::ScreenPointTarget* pTarget) { return KoopaLastFunction::receiveMsgScreenPointCommon(pMsg, pPointer, pTarget); }
/** @brief Leaves updates to the active nerve. */
void KoopaLastBreakWall::control() {}
/** @brief Waits inside the wall. */
void KoopaLastBreakWall::exeInsideWall() { if (al::isFirstStep(this)) al::startAction(this, sWaitActions[mAnimType]); }
/** @brief Tracks players during the warning animation. */
void KoopaLastBreakWall::exeBreakSign() {
    mNeck->subPowerRate(0.05f);
    mFace->setPowerRate(mNeck->mPowerRate);
    JointAimUtil::updateEyeJointInfo(this, mEyeL, 1500.0f, 0.05f);
    mEyeR->setTargetPos(mEyeL->mTargetPos);
    mEyeR->setPowerRate(mEyeR->mPowerRate);
    if (al::isFirstStep(this)) al::startAction(this, sSignActions[mAnimType]);
    if (al::isActionEnd(this)) al::setNerve(this, &NrvKoopaLastBreakWallBreakWall);
}
/** @brief Enables attack sensors while breaking the wall. */
void KoopaLastBreakWall::exeBreakWall() {
    if (al::isFirstStep(this)) {
        al::startAction(this, sBreakActions[mAnimType]);
        int animType = mAnimType;
        al::validateHitSensor(this, "Attack");
        if (animType == 3) {
            al::validateHitSensor(this, "Tail1");
            al::validateHitSensor(this, "Tail2");
            al::validateHitSensor(this, "Tail3");
        }
    }
    JointAimUtil::updateEyeJointInfo(this, mEyeL, 1500.0f, 0.05f);
    mEyeR->setTargetPos(mEyeL->mTargetPos);
    mEyeR->setPowerRate(mEyeR->mPowerRate);
    if (al::isActionEnd(this)) al::setNerve(this, &NrvKoopaLastBreakWallAttack);
}
/** @brief Tracks players during the attack. */
void KoopaLastBreakWall::exeAttack() {
    if (al::isFirstStep(this)) al::startAction(this, sAttackActions[mAnimType]);
    JointAimUtil::updateEyeJointInfo(this, mEyeL, 1500.0f, 0.05f);
    mEyeR->setTargetPos(mEyeL->mTargetPos);
    mEyeR->setPowerRate(mEyeR->mPowerRate);
    if (al::isActionEnd(this)) al::setNerve(this, &NrvKoopaLastBreakWallAttackEnd);
}
/** @brief Disables attack sensors after the ending animation. */
void KoopaLastBreakWall::exeAttackEnd() {
    mEyeR->subPowerRate(0.025f);
    mEyeR->setPowerRate(mEyeR->mPowerRate);
    if (al::isFirstStep(this)) al::startAction(this, sEndActions[mAnimType]);
    if (al::isActionEnd(this)) {
        int animType = mAnimType;
        al::invalidateHitSensor(this, "Attack");
        if (animType == 3) {
            al::invalidateHitSensor(this, "Tail1");
            al::invalidateHitSensor(this, "Tail2");
            al::invalidateHitSensor(this, "Tail3");
        }
        al::setNerve(this, &NrvKoopaLastBreakWallWait);
    }
}
/** @brief Tracks nearby players and waits before attacking again. */
void KoopaLastBreakWall::exeWait() {
    auto* player = rc::tryFindNearestActivePlayerActorInSphere(this, 1500.0f);
    if (player) {
        mNeck->setTargetPos(al::getTrans(player));
        mNeck->addPowerRate(0.05f);
        mEyeL->setTargetPos(al::getTrans(player));
        mEyeL->addPowerRate(0.05f);
    } else {
        mNeck->subPowerRate(0.05f);
        mEyeL->subPowerRate(0.05f);
    }
    mEyeR->setTargetPos(mNeck->mTargetPos);
    mEyeR->setPowerRate(mNeck->mPowerRate);
    mFace->setPowerRate(mNeck->mPowerRate);
    if (al::isFirstStep(this)) al::startAction(this, sWaitActions[mAnimType]);
    if (!al::isLessStep(this, 120) && !mStopped) al::setNerve(this, &NrvKoopaLastBreakWallBreakSign);
}
