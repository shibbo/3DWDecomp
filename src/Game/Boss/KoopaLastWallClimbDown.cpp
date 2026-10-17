#include "Boss/KoopaLastWallClimbDown.hpp"
#include "Boss/KoopaLastFunction.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/KeyPose/KeyPoseKeeper.hpp"
#include "Library/KeyPose/KeyPoseKeeperUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
namespace {
NERVE_DECL(KoopaLastWallClimbDown, Appear);
NERVE_DECL(KoopaLastWallClimbDown, DownStart);
NERVE_DECL(KoopaLastWallClimbDown, Down);
NERVE_DECL(KoopaLastWallClimbDown, DownEnd);
NERVE_DECL(KoopaLastWallClimbDown, Fall);
NERVE_DECL(KoopaLastWallClimbDown, Hide);
NERVES_MAKE_NOSTRUCT(KoopaLastWallClimbDown, Appear, DownStart, Down, DownEnd, Fall, Hide)
}
/** @brief Creates the wall-descent actor. @param pName Actor name. */
KoopaLastWallClimbDown::KoopaLastWallClimbDown(const char* pName) : al::LiveActor(pName) {}
/** @brief Destroys the wall-descent actor. */
KoopaLastWallClimbDown::~KoopaLastWallClimbDown() = default;
/** @brief Loads common Bowser data, key poses, and clipping. @param rInfo Initialization data. */
void KoopaLastWallClimbDown::init(const al::ActorInitInfo& rInfo) {
    KoopaLastFunction::initActorKoopaLastCommon(this, rInfo, nullptr, nullptr);
    al::initNerve(this, &NrvKoopaLastWallClimbDownAppear, 0);
    mKeyPoseKeeper = al::createKeyPoseKeeper(rInfo);
    float radius = 0.0f;
    al::calcKeyMoveClippingInfo(&mClippingCenter, &radius, mKeyPoseKeeper, 500.0f);
    al::setClippingInfo(this, radius, &mClippingCenter);
    al::tryGetArg(&mIsUseAppear, rInfo, "IsUseAppear");
    if (!al::tryListenStageSwitchAppear(this)) appear();
}
/** @brief Shows Bowser and selects the configured entry state. */
void KoopaLastWallClimbDown::appear() {
    al::showModelIfHide(this);
    al::LiveActor::appear();
    if (mIsUseAppear) al::setNerve(this, &NrvKoopaLastWallClimbDownAppear);
    else al::setNerve(this, &NrvKoopaLastWallClimbDownDown);
}
/** @brief Handles common Bowser messages. @param pMsg Message. @param pSelf Own sensor. @param pOther Sender sensor. @return Whether handled. */
bool KoopaLastWallClimbDown::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSelf, al::HitSensor* pOther) { return KoopaLastFunction::receiveMsgCommon(pMsg, pSelf, pOther); }
/** @brief Handles common pointer messages. @param pMsg Message. @param pPointer Pointer. @param pTarget Pointer target. @return Whether handled. */
bool KoopaLastWallClimbDown::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer, al::ScreenPointTarget* pTarget) { return KoopaLastFunction::receiveMsgScreenPointCommon(pMsg, pPointer, pTarget); }
/** @brief Handles contact attacks while visible. @param pSelf Own sensor. @param pOther Target sensor. */
void KoopaLastWallClimbDown::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isNerve(this, &NrvKoopaLastWallClimbDownHide)) return;
    KoopaLastFunction::attackSensorCommon(pSelf, pOther);
}
/** @brief Eases Bowser out of the wall to the first key pose. */
void KoopaLastWallClimbDown::exeAppear() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "ClimbOutAppear");
        mAppearPos = al::getCurrentKeyTrans(mKeyPoseKeeper);
        al::setQuat(this, al::getCurrentKeyQuat(mKeyPoseKeeper));
    }
    const auto& quat = al::getCurrentKeyQuat(mKeyPoseKeeper);
    sead::Vector3f up = sead::Vector3f::ez;
    sead::Vector3f front = sead::Vector3f::ey;
    al::calcQuatFront(&front, quat);
    al::calcQuatUp(&up, quat);
    float upOffset = al::calcNerveEaseOutValue(this, 20, 1000.0f, 0.0f);
    float frontOffset = al::calcNerveEaseInValue(this, 20, -500.0f, 0.0f);
    al::setTrans(this, mAppearPos + up * upOffset + front * frontOffset);
    if (al::isLessStep(this, 20)) return;
    al::setNerve(this, &NrvKoopaLastWallClimbDownDownStart);
}
/** @brief Plays the start animation and waits before descending. */
void KoopaLastWallClimbDown::exeDownStart() {
    if (al::isFirstStep(this)) al::startAction(this, "ClimbOutStart");
    if (al::isActionPlaying(this, "ClimbOutStart") && al::isActionEnd(this)) al::startAction(this, "ClimbOutWait");
    if (al::isLessStep(this, 90)) return;
    al::setNerve(this, &NrvKoopaLastWallClimbDownDown);
}
/** @brief Moves between key poses and checks the final disappearance setting. */
void KoopaLastWallClimbDown::exeDown() {
    if (al::isFirstStep(this)) {
        al::tryStartActionIfNotPlaying(this, "ClimbOutLoop");
        al::tryGetArg(&mMoveSpeed, al::getCurrentKeyPlacementInfo(mKeyPoseKeeper), "MoveSpeed");
        mMoveDistance = 0.0f;
        mKeyDistance = al::calcDistanceNextKeyTrans(mKeyPoseKeeper);
        if (al::isNearZero(mKeyDistance, 0.001f)) mKeyDistance = 1.0f;
        al::calcSlerpKeyQuat(al::getQuatPtr(this), mKeyPoseKeeper, 1.0f);
    }
    mMoveDistance = mMoveSpeed + mMoveDistance;
    float rate = mMoveDistance / mKeyDistance;
    float clampedRate = rate < 1.0f ? rate : 1.0f;
    al::calcLerpKeyTrans(al::getTransPtr(this), mKeyPoseKeeper, clampedRate);
    KoopaLastFunction::explosionCollision(this, "BodyUnder");
    if (rate < 1.0f) return;
    mKeyPoseKeeper->next();
    if (mKeyPoseKeeper->isLastKey()) {
        bool disappear = false;
        al::tryGetArg(&disappear, al::getCurrentKeyPlacementInfo(mKeyPoseKeeper), "IsDisappear");
        if (disappear) al::setNerve(this, &NrvKoopaLastWallClimbDownDownEnd);
        else al::setNerve(this, &NrvKoopaLastWallClimbDownHide);
    } else al::setNerve(this, &NrvKoopaLastWallClimbDownHide);
}
/** @brief Plays the end animation before falling. */
void KoopaLastWallClimbDown::exeDownEnd() {
    if (al::isFirstStep(this)) al::startAction(this, "ClimbOutEnd");
    if (al::isActionEnd(this)) al::setNerve(this, &NrvKoopaLastWallClimbDownFall);
}
/** @brief Plays the falling animation and breaks nearby blocks. */
void KoopaLastWallClimbDown::exeFall() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "ClimbOutFall");
        al::invalidateClipping(this);
    }
    KoopaLastFunction::explosionCollision(this, "BodyUnder");
    if (al::isActionEnd(this)) al::setNerve(this, &NrvKoopaLastWallClimbDownHide);
}
/** @brief Advances the key pose and hides the actor. */
void KoopaLastWallClimbDown::exeHide() {
    if (al::isFirstStep(this)) {
        mKeyPoseKeeper->next();
        al::hideModelIfShow(this);
        al::validateClipping(this);
        kill();
    }
}
