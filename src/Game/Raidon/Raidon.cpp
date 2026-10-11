#include "Raidon/Raidon.hpp"

#include <prim/seadSafeString.h>

#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Actor/ComboCounter.hpp"
#include "Library/Audio/System/AudioKeeper.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/Item/ItemUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorAnimUtil.hpp"
#include "Library/LiveActor/Util/ActorClippingUtil.hpp"
#include "Library/LiveActor/Util/ActorFlagUtil.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorModelUtil.hpp"
#include "Library/LiveActor/Util/ActorMovementUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Math/MatrixUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/StageSwitch/StageSwitchFunc.hpp"
#include "NPC/ActorStateSupportStroke.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Raidon/RaidonEndState.hpp"
#include "Raidon/RaidonGoalState.hpp"
#include "Raidon/RaidonPuppeteer.hpp"
#include "Raidon/RaidonRideAnimState.hpp"
#include "Raidon/RaidonRideStartState.hpp"
#include "Raidon/RaidonWaitState.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/PlayerPuppetUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"

namespace rc {
void disappearCameraChangeLayoutAndResetCameraMode(const al::IUseSceneObjHolder* pHolder);
void appearCameraChangeLayout(const al::IUseSceneObjHolder* pHolder);
void disappearGuideGameWindow(const al::IUseSceneObjHolder* pHolder);
}  // namespace rc

namespace {
NERVE_DECL(Raidon, Wait)
NERVE_DECL(Raidon, Start)
NERVE_DECL(Raidon, Ride)
NERVE_DECL(Raidon, Goal)
NERVE_DECL(Raidon, End)
NERVE_DECL(Raidon, GetOn)
NERVE_DECL(Raidon, GetOff)
NERVE_DECL(Raidon, Abyss)
NERVES_MAKE_STRUCT(Raidon, Wait, Start, Ride, Goal, End, GetOn, GetOff, Abyss)

/// Joints the riding players are attached to, by seat index.
const char* const cPlayerJointNames[] = {"Player1", "Player2", "Player3", "Player4"};

/// Blend weights of the five-way input animation (neutral, -X, +X, +Y, -Y).
struct InputBlendWeight {
    f32 neutral;
    f32 minusX;
    f32 plusX;
    f32 plusY;
    f32 minusY;

    /** @brief Splits a two-axis input into the five blend weights.
     * @param x Horizontal input.
     * @param y Vertical input.
     */
    InputBlendWeight(f32 x, f32 y) {
        f32 sum = sead::Mathf::abs(x) + sead::Mathf::abs(y);
        if (sum > 1.0f) {
            x /= sum;
            y /= sum;
            sum = 1.0f;
        }

        neutral = 1.0f - sum;
        minusX = x < 0.0f ? -x : 0.0f;
        plusX = sead::Mathf::max(x, 0.0f);
        plusY = sead::Mathf::max(y, 0.0f);
        minusY = y < 0.0f ? -y : 0.0f;
    }
};
}  // namespace

/** @param pName Actor name. */
Raidon::Raidon(const char* pName)
    : RaidonBase(pName), mTrampleComboCounter(new al::ComboCounter()),
      mInvincibleComboCounter(new al::ComboCounter()) {}

/** @brief Creates the riding states and the four player seats.
 * @param rInfo Actor init info.
 */
void Raidon::init(const al::ActorInitInfo& rInfo) {
    RaidonActor::init(rInfo, "Raidon");
    al::createAndSetColliderSpecialPurpose(this, "MoveLimit");
    al::setEffectFollowMtxPtr(this, "ScreenWet", &mScreenWetMtx);
    rc::createInvincibleUbo(this);
    al::hideInvincibleModel(this);
    mBaseQuat = al::getQuat(this);
    mBaseTrans = al::getTrans(this);
    al::calcFrontDir(&mBaseFrontDir, this);
    al::calcSideDir(&mBaseSideDir, this);

    mPuppeteerNumMax = 4;
    mPuppeteers = new RaidonPuppeteer[mPuppeteerNumMax];
    for (s32 i = 0; i < mPuppeteerNumMax; i++) {
        mPuppeteers[i].init(rInfo);
    }

    mStateSupportStroke = new ActorStateSupportStroke(this);
    mStateSupportStroke->resetFlags();
    mStateSupportStroke->appear();

    mWaitState = new RaidonWaitState("待機中状態", this, mStateSupportStroke);
    mRideStartState = new RaidonRideStartState("開始状態", this, rInfo);
    mRideAnimState = new RaidonRideAnimState("ライド状態", this);
    mGoalState = new RaidonGoalState("ゴール状態", this);
    mEndState = new RaidonEndState("終了状態", this, mStateSupportStroke);

    al::initNerve(this, &NrvRaidon.Wait, 5);
    al::initNerveState(this, mWaitState, &NrvRaidon.Wait, "待機状態");
    al::initNerveState(this, mRideStartState, &NrvRaidon.Start, "開始状態");
    al::initNerveState(this, mRideAnimState, &NrvRaidon.Ride, "ライド状態");
    al::initNerveState(this, mGoalState, &NrvRaidon.Goal, "ゴール状態");
    al::initNerveState(this, mEndState, &NrvRaidon.End, "終了状態");

    mIsEnableGoalPosition = al::tryGetLinksTrans(&mGoalPosition, rInfo, "GoalPosition");
    al::tryGetArg(&mIsNotChangeBgm, rInfo, "IsNotChangeBgm");
    offSpringControl();

    mFallAreaChecker = new al::AudioGeneralPurposeAreaChecker("RaidonFallTriggeredArea");
    mFallAreaChecker->init(getAreaObjDirector());
    mFallAreaChecker->setPlayerHolder(rInfo.getActorSceneInfo().playerHolder);
    makeActorAppeared();
}

/** @brief Pushes, tramples or attacks what Plessie runs into.
 * @param pSelf Plessie's sensor.
 * @param pOther Touched sensor.
 */
void Raidon::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isNerve(this, &NrvRaidon.End)) {
        mEndState->attackSensor(pSelf, pOther);
        return;
    }

    if (al::isSensorMapObj(pSelf)) {
        if (al::isNerve(this, &NrvRaidon.Wait) || al::isNerve(this, &NrvRaidon.GetOn)) {
            al::sendMsgPush(pOther, pSelf);
        }
    }

    if (al::isSensorRide(pSelf)) {
        if (mFirstPlayerSensor != nullptr) {
            al::sendMsgRideAllPlayerItemGet(pOther, pSelf);
            if (rc::isPlayerInvincible(this, mFirstPlayerSensor)) {
                al::sendMsgPlayerInvincibleAttack(pOther, pSelf, mInvincibleComboCounter);
            }
        }

        bool isOnGround = isOnGroundRaidon();
        bool isEnemy = al::isSensorEnemy(pOther);
        if (isOnGround) {
            if (isEnemy) {
                rc::sendMsgBobsledBodyAttack(pOther, pSelf);
            }
        } else if (isEnemy && rc::sendMsgBobsledTrample(pOther, pSelf, mTrampleComboCounter)) {
            al::getVelocityPtr(this)->y = 55.0f;
            mRideAnimState->requestBound();
        }
    }

    if (isOnGroundRaidon() && rc::sendMsgAskBobsledDashPanel(pOther, pSelf)) {
        if (mDashTimer < 40) {
            mRideAnimState->requestDash();
        }

        sead::Vector3f dir = al::getVelocity(this);
        dir.y = 0.0f;
        if (al::normalizeOrZero(&dir)) {
            al::calcFrontDir(&dir, this);
        }

        sead::Vector3f* pVelocity = al::getVelocityPtr(this);
        al::verticalizeVec(pVelocity, dir, *pVelocity);
        al::addVelocityToDirection(this, dir, 80.0f);
        mDashTimer = 50;
    }
}

/** @brief Handles binding the riders, pushes and the messages forwarded to the states.
 * @param pMsg Received message.
 * @param pOther Sender sensor.
 * @param pSelf Plessie's sensor.
 * @return Whether the message was handled.
 */
bool Raidon::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) {
    if (rc::isMsgIsEnableExitStage(pMsg)) {
        return isOnGroundRaidon();
    }

    if (al::isMsgBindCancel(pMsg)) {
        if (al::isNerve(this, &NrvRaidon.End)) {
            return true;
        }

        if (mFirstPlayerSensor != nullptr) {
            rc::cancelRequestBindAllPlayer(this, al::getHitSensor(this, "Bind"));
        }

        for (s32 i = 0; i < mPuppeteerNum; i++) {
            if (mPuppeteers[i].mPuppet != nullptr) {
                mPuppeteers[i].cancelBind();
                mPuppeteers[i].mPuppet = nullptr;
            }
        }

        mPuppeteerNum = 0;
        mFirstPlayerSensor = nullptr;
        return true;
    }

    if (al::isNerve(this, &NrvRaidon.Wait) && mWaitState->receiveMsg(pMsg, pOther, pSelf)) {
        return true;
    }

    if (al::isNerve(this, &NrvRaidon.End)) {
        return mEndState->receiveMsg(pMsg, pOther, pSelf);
    }

    if (al::isSensorMapObj(pSelf) &&
        (al::isMsgPlayerFireBallAttack(pMsg) || al::isMsgPlayerBoomerangReflect(pMsg))) {
        rc::requestHitReactionToAttackerNpc(pSelf, pOther);
        return true;
    }

    if (mFirstPlayerSensor != nullptr) {
        for (s32 i = 0; i < mPuppeteerNum; i++) {
            IUsePlayerPuppet* pPuppet = mPuppeteers[i].mPuppet;
            if (pPuppet != nullptr && rc::isMsgAskControlUserId(pMsg, rc::getPuppetSensor(pPuppet))) {
                return true;
            }
        }
    }

    if (al::isMsgPushStrong(pMsg) && al::isSensorName(pSelf, "Body")) {
        if (mFirstPlayerSensor != nullptr && rc::isPlayerInvincible(this, mFirstPlayerSensor)) {
            return false;
        }

        sead::Vector3f dir;
        al::calcDirBetweenSensorsH(&dir, pOther, pSelf);
        f32 speed = al::getVelocity(this).dot(dir);
        if (speed < 0.0f) {
            f32 pushSpeed = sead::Mathf::abs(speed) < 25.0f ? 25.0f : speed * -1.7f;
            mRideAnimState->requestHit();
            al::startHitReactionHitEffect(this, "衝突", pOther, pSelf);
            al::addVelocityToDirection(this, dir, pushSpeed);
            al::scaleVelocityHV(this, 0.8f, 1.0f);
            mHitTimer = 50;
            return true;
        }
    }

    if (al::isMsgBindStart(pMsg)) {
        return al::isNerve(this, &NrvRaidon.Wait) || al::isNerve(this, &NrvRaidon.GetOn) ||
               al::isNerve(this, &NrvRaidon.Start) || al::isNerve(this, &NrvRaidon.Ride);
    }

    if (al::isMsgBindInit(pMsg)) {
        if (mFirstPlayerSensor == nullptr) {
            mFirstPlayerSensor = pOther;
        }

        IUsePlayerPuppet* pPuppet = rc::startPuppet(pSelf, pOther);
        rc::hidePuppetShadow(pPuppet);
        if (al::isNerve(this, &NrvRaidon.Wait) ||
            (al::isNerve(this, &NrvRaidon.GetOn) && al::isLessEqualStep(this, 120))) {
            mPuppeteers[mPuppeteerNum].startGetOn(
                pPuppet, al::getJointMtxPtr(this, cPlayerJointNames[mPuppeteerNum]), false);
        } else {
            mPuppeteers[mPuppeteerNum].startBindWarp(
                pPuppet, al::getJointMtxPtr(this, cPlayerJointNames[mPuppeteerNum]));
        }

        rc::forceEndSubActionPuppet(pPuppet);
        al::sendMsgHoldCancel(pOther, pSelf);
        mPuppeteerNum++;
        if (al::isNerve(this, &NrvRaidon.Wait)) {
            al::invalidateClipping(this);
            al::setNerve(this, &NrvRaidon.GetOn);
        }

        return true;
    }

    return false;
}

/** @brief Forwards touch screen messages to the waiting and end states.
 * @param pMsg Received message.
 * @param pPointer Screen pointer.
 * @param pTarget Touched target.
 * @return Whether the message was handled.
 */
bool Raidon::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                   al::ScreenPointTarget* pTarget) {
    if (al::isNerve(this, &NrvRaidon.Wait)) {
        return mWaitState->receiveMsgScreenPoint(pMsg, pPointer, pTarget);
    }

    if (al::isNerve(this, &NrvRaidon.End)) {
        return mEndState->receiveMsgScreenPoint(pMsg, pPointer, pTarget);
    }

    return al::isMsgTouchAssist(pMsg);
}

/** @brief Updates stroking, the invincibility model and the frame counters. */
void Raidon::control() {
    mStateSupportStroke->update();
    if (mStateSupportStroke->isTrigStroke()) {
        sead::Vector3f itemPos = al::getTrans(this) + sead::Vector3f(0.0f, 200.0f, 0.0f);
        al::appearItemTiming(this, "撫でる", itemPos, sead::Vector3f::ey);
    }

    if (mFirstPlayerSensor != nullptr) {
        if (!rc::isPlayerInvincible(this, mFirstPlayerSensor)) {
            mInvincibleComboCounter->reset();
        }

        bool isAppear = rc::isPlayerInvincibleModelAppear(mFirstPlayerSensor);
        bool isHidden = al::isInvincibleModelHidden(this);
        if (isAppear) {
            if (isHidden) {
                al::showInvincibleModel(this);
                al::tryEmitEffect(this, "SuperStar", nullptr);
            }

            rc::setInvincibleColor(this, rc::getPlayerInvincibleColor(mFirstPlayerSensor));
        } else if (!isHidden) {
            sead::Vector3f velocity = al::getVelocity(this);
            al::hideInvincibleModel(this);
            al::setVelocity(this, velocity);
            al::tryDeleteEffect(this, "SuperStar");
        }
    } else if (!al::isInvincibleModelHidden(this)) {
        al::tryDeleteEffect(this, "SuperStar");
        al::hideInvincibleModel(this);
    }

    const sead::Vector3f& rTrans = al::getTrans(this);
    mDashBlurCenter = mBaseFrontDir * 3000.0f + rTrans;

    if (mDashTimer > 0) {
        mDashTimer--;
    }

    if (mMultiJumpTimer > 0) {
        mMultiJumpTimer--;
    }

    if (mHitTimer > 0) {
        mHitTimer--;
    }
}

/** @brief Moves the riding players along with their joints. */
void Raidon::calcAnim() {
    al::LiveActor::calcAnim();
    if (al::isDead(this)) {
        return;
    }

    if (al::isNerve(this, &NrvRaidon.GetOn) || al::isNerve(this, &NrvRaidon.Start) ||
        al::isNerve(this, &NrvRaidon.Ride) || al::isNerve(this, &NrvRaidon.Goal) ||
        al::isNerve(this, &NrvRaidon.GetOff)) {
        setPuppetQT();
    }
}

/** @brief Places every rider on their seat joint. */
void Raidon::setPuppetQT() {
    for (s32 i = 0; i < mPuppeteerNum; i++) {
        if (mPuppeteers[i].mPuppet != nullptr) {
            sead::Matrix34f mtx;
            al::normalizeMtxScale(&mtx, *al::getJointMtxPtr(this, cPlayerJointNames[i]));
            rc::setPuppetMtx(mPuppeteers[i].mPuppet, &mtx);
        }
    }
}

/** @brief Waits for players to get on. */
void Raidon::exeWait() {
    al::updateNerveState(this);
}

/** @brief Lets the players get on and starts once all of them are bound. */
void Raidon::exeGetOn() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "GetOn");
        rc::disappearCameraChangeLayoutAndResetCameraMode(this);
        al::tryOnStageSwitch(this, "SwitchRideOn");
    }

    backHead();
    al::turnQuatFrontToDirDegreeH(this, mBaseFrontDir, 1.0f);
    if (al::isGreaterEqualStep(this, 120)) {
        rc::requestBindAllPlayer(this, al::getHitSensor(this, "Bind"));
    }

    if (al::isActionPlaying(this, "GetOn") && al::isActionEnd(this)) {
        al::startAction(this, "ReadyWait");
    }

    for (s32 i = 0; i < mPuppeteerNum; i++) {
        mPuppeteers[i].updateNerve();
    }

    if (rc::isAllPlayerBinded(this, al::getHitSensor(this, "Bind")) &&
        al::isActionPlaying(this, "ReadyWait")) {
        rc::setDisableReviveBubbleForAllPlayer(this);
        for (s32 i = 0; i < mPuppeteerNum; i++) {
            IUsePlayerPuppet* pPuppet = mPuppeteers[i].mPuppet;
            if (pPuppet != nullptr) {
                al::setCameraLookAtPosPtr(this, rc::getPuppetSensor(pPuppet),
                                          al::getTransPtr(this));
            }
        }

        al::setNerve(this, &NrvRaidon.Start);
    }
}

/** @brief Runs the ride start state. */
void Raidon::exeStart() {
    addSpringControlRate(0.05f);
    rc::requestBindAllPlayer(this, al::getHitSensor(this, "Bind"));
    for (s32 i = 0; i < mPuppeteerNum; i++) {
        mPuppeteers[i].updateNerve();
    }

    if (al::updateNerveState(this)) {
        rc::resetDisableReviveBubbleForAllPlayer(this);
        al::setNerve(this, &NrvRaidon.Ride);
    }
}

/** @brief Moves Plessie with the riders' input until the goal is reached. */
void Raidon::exeRide() {
    addSpringControlRate(0.05f);
    if (al::isFirstStep(this)) {
        mFallAreaChecker->reset();
    }

    mFallAreaChecker->update(-1);
    if (mFallAreaChecker->isEnteredArea()) {
        startPuppetSe("LongDiving");
    }

    if (rc::isInDeathArea(this)) {
        for (s32 i = 0; i < mPuppeteerNum; i++) {
            if (mPuppeteers[i].mPuppet != nullptr) {
                rc::endBindForceAbyssAndPuppetNull(&mPuppeteers[i].mPuppet);
            }
        }

        mPuppeteerNum = 0;
        mFirstPlayerSensor = nullptr;
        al::setNerve(this, &NrvRaidon.Abyss);
        return;
    }

    if (al::HitSensor* pGroundSensor = al::tryGetCollidedGroundSensor(this)) {
        rc::sendMsgRaidonAttack(pGroundSensor, al::getHitSensor(this, "Body"));
    }

    if (al::HitSensor* pWallSensor = al::tryGetCollidedWallSensor(this)) {
        rc::sendMsgRaidonAttack(pWallSensor, al::getHitSensor(this, "Body"));
    }

    rc::requestBindAllPlayerAcceptReviveBubble(this, al::getHitSensor(this, "Bind"));
    for (s32 i = 0; i < mPuppeteerNum; i++) {
        mPuppeteers[i].updateNerve();
    }

    updatePuppetInput();
    updateHandleAndAccel();
    updateRide();
    backHead();
    if (al::isStep(this, 240)) {
        rc::disappearGuideGameWindow(this);
    }

    if (al::updateNerveState(this)) {
        mGoalState->setIsSwim(mRideAnimState->isSwim());
        al::setNerve(this, &NrvRaidon.Goal);
    }
}

/** @brief Falls after dropping into a death area. */
void Raidon::exeAbyss() {
    al::addVelocityToGravity(this, 1.5f);
    al::reboundVelocityFromCollision(this, 0.0f, 0.0f, 1.0f);
    al::scaleVelocity(this, 0.985f);
}

/** @brief Runs the goal state, then lets the players get off. */
void Raidon::exeGoal() {
    subSpringControlRate(0.05f);
    if (al::updateNerveState(this)) {
        al::setNerve(this, &NrvRaidon.GetOff);
    }
}

/** @brief Drops the players off to both sides of Plessie. */
void Raidon::exeGetOff() {
    if (al::isFirstStep(this)) {
        al::tryOnStageSwitch(this, "SwitchGoalOn");
        al::startAction(this, "GoalWait");

        sead::Vector3f sideDir;
        al::calcSideDir(&sideDir, this);
        for (s32 i = 0; i < mPuppeteerNum; i++) {
            IUsePlayerPuppet* pPuppet = mPuppeteers[i].mPuppet;
            if (pPuppet != nullptr) {
                al::setCameraLookAtPosPtr(this, rc::getPuppetSensor(pPuppet), nullptr);
                sead::Vector3f dir = i % 2 == 0 ? -sideDir : sideDir;
                mPuppeteers[i].startGetOff(dir, i * 15, false, false);
            }
        }
    }

    subSpringControlRate(0.05f);
    for (s32 i = 0; i < mPuppeteerNum; i++) {
        mPuppeteers[i].updateNerve();
    }

    if (isAllGetOffPlayer()) {
        mPuppeteerNum = 0;
        mFirstPlayerSensor = nullptr;
        al::setNerve(this, &NrvRaidon.End);
    }
}

/** @brief Runs the end state. */
void Raidon::exeEnd() {
    if (al::isFirstStep(this)) {
        rc::appearCameraChangeLayout(this);
    }

    al::updateNerveState(this);
}

/** @brief Ends the bind of every rider.
 * @param pParam How the players leave Plessie.
 */
void Raidon::endBind(const PlayerBindEndParam* pParam) {
    for (s32 i = 0; i < mPuppeteerNum; i++) {
        if (mPuppeteers[i].mPuppet != nullptr) {
            rc::endBindAndPuppetNull(&mPuppeteers[i].mPuppet, pParam);
        }
    }

    mPuppeteerNum = 0;
    mFirstPlayerSensor = nullptr;
}

/** @brief Reads the input of every rider. */
void Raidon::updatePuppetInput() {
    bool isEnableInput = !rc::isInAreaObj(this, rc::AreaObjType::RaidonNoInputArea) &&
                         !rc::isInAreaObj(this, rc::AreaObjType::BobsledGoalArea);
    for (s32 i = 0; i < mPuppeteerNum; i++) {
        mPuppeteers[i].updateInput(isOnGroundRaidon(), isEnableInput);
    }
}

/** @brief Averages the steering and acceleration input of the riders. */
void Raidon::updateHandleAndAccel() {
    mAccel = 0.0f;
    mHandle = 0.0f;
    for (s32 i = 0; i < mPuppeteerNum; i++) {
        mHandle += mPuppeteers[i].getHandle();
        mAccel += mPuppeteers[i].getAccel();
    }

    if (mPuppeteerNum > 0) {
        mHandle /= mPuppeteerNum;
        mAccel /= mPuppeteerNum;
    }
}

/** @brief Turns the ground up vector towards the ground normal below Plessie. */
void Raidon::updateGroundUpVec() {
    if (al::isOnGround(this, 5, 0.0f)) {
        sead::Vector3f normal = al::getOnGroundNormal(this, 5);
        al::verticalizeVec(&normal, mBaseSideDir, normal);
        if (!al::normalizeOrZero(&normal)) {
            al::turnVecToVecDegree(&mGroundUpVec, normal, 1.5f);
        }

        return;
    }

    al::turnVecToVecDegree(&mGroundUpVec, sead::Vector3f::ey, 1.0f);
}

/** @brief Refreshes the ground counter and the trample combo. */
void Raidon::updateOnGround() {
    if (al::isOnGround(this, 0, 0.0f)) {
        mTrampleComboCounter->reset();
        mMultiJumpTimer = 0;
        mGroundCount = 15;
    } else if (mGroundCount > 0) {
        mGroundCount--;
    }
}

/** @brief Checks whether Plessie walks on water or sand. */
void Raidon::updateMatrialCode() {
    if (!al::isCollidedGround(this)) {
        return;
    }

    mMaterialCodeName = al::getCollidedFloorMaterialCodeName(this);
    mIsInWater = mMaterialCodeName != nullptr && (al::isEqualString(mMaterialCodeName, "InWater") ||
                                                  al::isEqualString(mMaterialCodeName, "InSand"));
}

/** @brief Starts an animation on every rider.
 * @param pActionName Animation name.
 */
void Raidon::startPuppetActionAll(const char* pActionName) {
    for (s32 i = 0; i < mPuppeteerNum; i++) {
        if (mPuppeteers[i].mPuppet != nullptr) {
            rc::startPuppetAction(mPuppeteers[i].mPuppet, pActionName);
        }
    }
}

/** @brief Blends every rider's animation by their own stick input. */
void Raidon::setPuppetInputBlendAnimWeight() {
    for (s32 i = 0; i < mPuppeteerNum; i++) {
        RaidonPuppeteer& rPuppeteer = mPuppeteers[i];
        if (rPuppeteer.mPuppet != nullptr) {
            InputBlendWeight weight(rPuppeteer.getStickX(), rPuppeteer.getStickY());
            rc::setPuppetBlendAnimWeight(rPuppeteer.mPuppet, weight.neutral, weight.minusX,
                                         weight.plusX, weight.plusY, weight.minusY, 0.0f);
        }
    }
}

/** @brief Blends Plessie's animation by the averaged input. */
void Raidon::setInputBlendAnimWeight() {
    InputBlendWeight weight(mHandle, mAccel);
    al::setSklAnimBlendWeightFivefold(this, weight.neutral, weight.minusX, weight.plusX,
                                      weight.plusY, weight.minusY);
}

/** @return Whether no player rides Plessie anymore. */
bool Raidon::isAllGetOffPlayer() const {
    for (s32 i = 0; i < mPuppeteerNum; i++) {
        if (mPuppeteers[i].mPuppet != nullptr) {
            return false;
        }
    }

    return true;
}

/** @brief Plays a sound on every rider.
 * @param pName Sound name.
 */
void Raidon::startPuppetSe(const char* pName) {
    for (s32 i = 0; i < mPuppeteerNum; i++) {
        if (mPuppeteers[i].mPuppet != nullptr) {
            rc::startPuppetSe(mPuppeteers[i].mPuppet, pName);
        }
    }
}

/** @brief Moves Plessie forward during the ride start. */
void Raidon::updateStart() {
    updateGroundUpVec();
    updateOnGround();
    updateMatrialCode();

    sead::Vector3f frontDir;
    al::calcFrontDir(&frontDir, this);
    al::addVelocityToDirection(this, frontDir, 0.8f);
    al::addVelocityToGravity(this, 1.5f);
    al::reboundVelocityFromCollision(this, 0.0f, 0.0f, 1.0f);
    al::scaleVelocity(this, 0.985f);
}

/** @brief Steers, accelerates and jumps by the riders' input. */
void Raidon::updateRide() {
    f32 forwardAccel = mAccel > 0.0f ? mAccel * 0.25f : 0.0f;
    f32 backAccel = mAccel < 0.0f ? mAccel * -0.28f : 0.0f;
    f32 dashAccel = 0.0f;
    f32 accel = isOnGroundRaidon() ? forwardAccel + dashAccel + 0.55f : 0.55f;
    if (mDashTimer > 0) {
        dashAccel = al::lerpValue(mDashTimer, 30.0f, 0.0f, 0.5f, 0.0f);
    }

    f32 rotateRate = al::lerpValue(forwardAccel, 0.0f, 0.2f, 0.1f, 0.1f);
    f32 targetRotateY = al::lerpValue(rotateRate, mRotateY, mHandle * -70.0f);
    mRotateY = al::converge(mRotateY, targetRotateY, 3.0f);
    al::rotateQuatYDirDegree(this, mBaseQuat, mRotateY);

    u32 jumpNum = 0;
    for (s32 i = 0; i < mPuppeteerNum; i++) {
        if (mPuppeteers[i].isRequestJump()) {
            jumpNum++;
        }
    }

    bool isOnGround = isOnGroundRaidon();
    if (jumpNum != 0 && isOnGround && mRideAnimState->requestJump()) {
        mGroundCount = 0;
        mMultiJumpTimer = 8;
        if (al::getVelocityPtr(this)->y < 0.0f) {
            al::getVelocityPtr(this)->y = 37.5f;
        } else {
            al::getVelocityPtr(this)->y += 37.5f;
        }

        if (jumpNum >= 2 && mMultiJumpTimer > 0) {
            al::getVelocityPtr(this)->y += 15.0f;
            al::startHitReaction(this, "マルチジャンプ成功");
            mMultiJumpTimer = 0;
        }
    }

    if (al::isCollidedWall(this)) {
        const sead::Vector3f& rVelocity = al::getVelocity(this);
        if (rVelocity.dot(al::getCollidedWallNormal(this)) <= -25.0f) {
            mRideAnimState->requestHit();
        }
    }

    updateGroundUpVec();
    updateOnGround();
    updateMatrialCode();

    f32 hitRate = al::lerpValue(mHitTimer, 10.0f, 30.0f, 1.0f, 0.0f);
    f32 frontSpeed = (accel + dashAccel) * hitRate;
    sead::Vector3f frontDir;
    al::calcFrontDir(&frontDir, this);
    al::addVelocityToDirection(this, frontDir, frontSpeed);
    al::addVelocityToDirection(this, mBaseFrontDir, -(backAccel * hitRate));
    al::addVelocityToGravity(this, 1.5f);
    al::reboundVelocityFromCollision(this, 0.0f, 0.0f, 1.0f);
    al::scaleVelocity(this, 0.985f);

    sead::Quatf* pQuat = al::getQuatPtr(this);
    al::turnQuatYDirRadian(pQuat, *pQuat, mGroundUpVec, sead::Mathf::deg2rad(20.0f));
}

/** @brief Makes Plessie count as airborne. */
void Raidon::clearGroundCount() {
    mGroundCount = 0;
}
