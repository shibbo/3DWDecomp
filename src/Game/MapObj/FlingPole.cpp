#include "MapObj/FlingPole.hpp"

#include <attributes.h>
#include <cmath>

#include <math/seadMathCalcCommon.h>
#include <prim/seadSafeString.h>

#include "Camera/DummyCameraTarget.hpp"
#include "Library/ActorUtil.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Library/Camera/CameraPoser_RS.hpp"
#include "Library/Camera/CameraTicket.hpp"
#include "Library/Camera/CameraTurnInfo.hpp"
#include "Library/Collision/PartsConnectorUtil.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/Controller/PadRumbleFunction.hpp"
#include "Library/Joint/JointControllerKeeper.hpp"
#include "Library/Joint/JointLocalAxisRotator.hpp"
#include "Library/Joint/JointSpringController.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/LiveActor/Util/ActorSceneInfo.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Layout/GuideGameWindow.hpp"
#include "Player/PlayerBindEndParam.hpp"
#include "Project/Camera/Main/CameraDirector_RS.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/DemoUtil.hpp"
#include "Util/PlayerPuppetUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"

namespace {
NERVE_DECL(FlingPole, Wait);
NERVE_DECL(FlingPole, BindTop);
NERVE_DECL(FlingPole, BindPoleWait);
NERVE_DECL(FlingPole, Damping);
NERVE_DECL(FlingPole, Jump);
NERVE_DECL(FlingPole, BindPoleDown);
NERVE_DECL(FlingPole, BindPoleClimb);
NERVE_DECL(FlingPole, Shoot);
NERVES_MAKE_NOSTRUCT(FlingPole, Wait, BindTop, BindPoleWait, Damping, Jump, BindPoleDown,
                     BindPoleClimb, Shoot)

/// End of the bind when the player jumps off the side of the pole.
const PlayerBindEndParam cBindEndParamJumpSide = {
    {}, 1, 20, true, true, true, 0, 1.2f, 0, false, {}};

/// End of the bind when the player jumps off the top of the pole.
const PlayerBindEndParam cBindEndParamJumpTop = {
    {}, 1, 0, true, false, true, 0, 1.2f, 0, false, {}};

/// End of the bind when the player slides off the bottom of the pole.
const PlayerBindEndParam cBindEndParamFall = {
    {}, 1, 20, true, true, true, 0, -1.0f, 15, false, {}};

/// Joints of the pole, from the bottom to the top.
const char* const cStickJointNames[] = {"Stick1", "Stick2", "Stick3", "Stick4", "Stick5", "Stick6"};

/// Joints of the lighthouse flag cloth.
const char* const cFlagJointNames[] = {"JntFlag1", "JntFlag2", "JntFlag3",
                                       "JntFlag4", "JntFlag5", "JntFlag6"};

/// Spring setup of one lighthouse flag joint.
struct FlagSpringParam {
    sead::Vector3f mChildLocalPos;
    f32 mStability;
    f32 mFriction;
    f32 mLimitDegree;
};

const FlagSpringParam cFlagSpringParams[] = {
    {{0.0f, 100.0f, 0.0f}, 0.9f, 0.85f, 20.0f},  {{0.0f, 100.0f, 10.0f}, 0.8f, 0.4f, 40.0f},
    {{0.0f, 100.0f, 20.0f}, 0.8f, 0.4f, 60.0f},  {{0.0f, 100.0f, 30.0f}, 0.8f, 0.4f, 60.0f},
    {{0.0f, 100.0f, 20.0f}, 0.8f, 0.4f, 60.0f},  {{0.0f, 100.0f, 10.0f}, 0.8f, 0.4f, 60.0f},
};

/**
 * @brief Check whether a sensor touches one of the segments of the pole.
 * @param pPole The pole.
 * @param pSelf The pole's sensor, only the eye sensor is checked.
 * @param pOther The other sensor.
 * @param pPushDir Receives the direction the other sensor is pushed out in.
 * @param pIndex Receives the index of the upper joint of the touched segment (optional).
 * @return Whether the sensor touches the pole.
 */
bool checkHitStick(const al::LiveActor* pPole, al::HitSensor* pSelf, al::HitSensor* pOther,
                   sead::Vector3f* pPushDir, s32* pIndex) {
    if (!al::isSensorEye(pSelf)) {
        return false;
    }

    sead::Vector3f prevPos;
    al::calcJointPos(&prevPos, pPole, cStickJointNames[0]);
    for (s32 i = 1; i < 6; i++) {
        sead::Vector3f pos;
        al::calcJointPos(&pos, pPole, cStickJointNames[i]);
        sead::Vector3f dir = pos - prevPos;
        f32 length = dir.length();
        if (length > sead::Mathf::epsilon()) {
            f32 dot = (al::getSensorPos(pOther) - prevPos).dot(dir) / length;
            f32 radius = al::getSensorRadius(pOther);
            if (dot > -radius && dot < length + radius) {
                dir *= 1.0f / length;
                if (al::isHitCylinderSensor(nullptr, pPushDir, pOther, prevPos, dir, 20.0f)) {
                    if (pIndex != nullptr) {
                        *pIndex = i;
                    }

                    return true;
                }
            }
        }

        prevPos = pos;
    }

    return false;
}

/**
 * @brief Calculate the rate of an analog stick axis past its dead zone.
 * @param value The stick axis value.
 * @return The rate in [0, 1].
 */
f32 calcStickRate(f32 value) {
    return (sead::Mathf::abs(value) - 0.3f) / 0.7f;
}

/**
 * @brief Calculate the volume of the wobble sound.
 * @param rate The swing rate.
 * @return The volume.
 */
f32 calcWobbleSeVolume(f32 rate) {
    return sead::Mathf::clamp(sead::lerp(0.5f, 1.0f, rate), 0.0f, 1.0f);
}
}  // namespace

/**
 * @brief Read a snap point from its placement.
 * @param info Placement of the snap point.
 * @param index Index of the snap point in the pole's links.
 */
inline FlingPole::SnapPoint::SnapPoint(al::PlacementInfo info, s32 index) : mIndex(index) {
    al::tryGetTrans(&mTrans, info);
    al::tryGetArg(&mStrength, info, "SnapStrength");
    sead::Vector3f scale;
    al::tryGetScale(&scale, info);
    mRadius = scale.x * 1500.0f * 0.5f;
}

/**
 * @brief Construct a fling pole.
 * @param pName Name of the actor.
 * @param isLighthouseFlag Whether this is the flag pole of a lighthouse.
 */
FlingPole::FlingPole(const char* pName, bool isLighthouseFlag)
    : al::LiveActor(pName),
      mBindEndParam(new PlayerBindEndParam{{}, 1, 20, true, false, true, 1, 0.8f, 10, false, {}}),
      mIsLighthouseFlag(isLighthouseFlag) {}

/**
 * @brief Initialize the pole from its placement.
 * @param rInfo Placement info.
 */
void FlingPole::init(const al::ActorInitInfo& rInfo) {
    Param* param = mParam;
    al::tryGetArg(&param->mFlingPower, rInfo, "FlingPower");
    al::tryGetArg(&param->mFlingDashTime, rInfo, "FlingDashTime");
    al::tryGetArg(&param->mIsConnectToCollision, rInfo, "IsConnectToCollision");
    al::tryGetArg(&param->mCameraDist, rInfo, "CameraDist");
    al::tryGetArg(&param->mIsDisabledPR, rInfo, "isDisabledPR");
    al::tryGetArg(&param->mIsDisablePlessieChase, rInfo, "isDisablePlessieChase");
    al::tryGetStringArg(&param->mComment, rInfo, "Comment");

    if (mIsLighthouseFlag) {
        al::initActorWithArchiveName(this, rInfo, "FlingPoleFlag", nullptr);
    } else {
        al::initActor(this, rInfo);
    }

    al::initNerve(this, &NrvFlingPoleWait, 0);
    initJointKeeper();

    s32 snapPointNum = al::calcLinkChildNum(rInfo, "SnapPoint");
    if (snapPointNum >= 1) {
        mSnapPoints.allocBuffer(snapPointNum, nullptr);
        al::PlacementInfo linkInfo;
        for (s32 i = 0; i < snapPointNum; i++) {
            al::getLinksInfoByIndex(&linkInfo, rInfo, "SnapPoint", i);
            mSnapPoints.pushBack(new SnapPoint(linkInfo, i));
        }
    }

    if (mParam->mIsConnectToCollision) {
        mConnector = al::tryCreateMtxConnector(this, rInfo);
    }

    mCameraTarget = new DummyCameraTarget("FlingPoleCamera");
    mCameraTarget->init(rInfo);
    makeActorAppeared();
}

/**
 * @brief Set up the joint controllers bending the pole (and waving the lighthouse flag).
 */
void FlingPole::initJointKeeper() {
    al::initJointControllerKeeper(this, mIsLighthouseFlag ? 18 : 12);
    mStickRotators.allocBuffer(6, nullptr);
    for (s32 i = 0; i < 6; i++) {
        mStickRotators.pushBack(al::initJointLocalAxisRotator_RS(this, mRotateAxis, &mRotateAngle,
                                                                 cStickJointNames[i], false));
        al::initJointLocalXRotator(this, &mStickXAngle, cStickJointNames[i]);
    }

    if (!mIsLighthouseFlag) {
        return;
    }

    for (s32 i = 0; i < 6; i++) {
        al::JointSpringController* spring = al::initJointSpringController(this, cFlagJointNames[i]);
        spring->setChildLocalPos(cFlagSpringParams[i].mChildLocalPos);
        spring->setStability(cFlagSpringParams[i].mStability);
        spring->setFriction(cFlagSpringParams[i].mFriction);
        spring->setLimitDegree(cFlagSpringParams[i].mLimitDegree);
    }
}

/**
 * @brief Attach the pole to its collision and listen to disaster mode changes.
 */
void FlingPole::initAfterPlacement() {
    if (mConnector != nullptr) {
        al::attachMtxConnectorToCollision(mConnector, this, false);
    }

    DisasterModeController* controller = DisasterModeController::tryGetController(this);
    if (controller != nullptr) {
        controller->registerStateListener(this);
    }
}

/**
 * @brief Move the pole along with what it is linked to.
 * @param rTrans The new position.
 */
void FlingPole::updateLinkedTrans(const sead::Vector3f& rTrans) {
    al::LiveActor::updateLinkedTrans(rTrans);
}

/**
 * @brief Disable the collision while clipped.
 */
void FlingPole::startClipped() {
    al::LiveActor::startClipped();
    if (getCollisionParts() != nullptr) {
        al::invalidateCollisionParts(this);
    }
}

/**
 * @brief Enable the collision again once unclipped.
 */
void FlingPole::endClipped() {
    al::LiveActor::endClipped();
    if (getCollisionParts() != nullptr) {
        al::validateCollisionParts(this);
    }
}

/**
 * @brief Push away and get shaken by whatever touches the pole.
 * @param pSelf The pole's sensor.
 * @param pOther The touching sensor.
 */
void FlingPole::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    sead::Vector3f pushDir;
    if (al::isSensorKoopaJr(pOther)) {
        if (!al::isSensorName(pSelf, "Push")) {
            if (!checkHitStick(this, pSelf, pOther, &pushDir, nullptr)) {
                return;
            }

            rc::sendMsgAddForce(pOther, pSelf, pushDir * 0.5f);
            if (al::isVelocityFast(al::getSensorHost(pOther), 5.0f)) {
                tryShakePole(-4.0f);
            }

            return;
        }
    } else {
        if (!((al::isSensorPlayer(pOther) && !isBindPole()) || al::isSensorRide(pOther) ||
              (al::isSensorNpc(pOther) && al::isSensorName(pOther, "Body")) ||
              (al::isSensorMapObj(pOther) && al::isSensorName(pOther, "Body") &&
               !al::isSensorHostName(pOther, "コイン")) ||
              (al::isSensorEnemyBody(pOther) && al::isSensorName(pOther, "Body")) ||
              (al::isSensorEnemy(pOther) && al::isSensorName(pOther, "Body")) ||
              (al::isSensorEnemyAttack(pOther) && al::isSensorName(pOther, "Attack")) ||
              al::isSensorKickKoura(pOther))) {
            return;
        }

        if (!al::isSensorName(pSelf, "Push") && !al::isSensorName(pSelf, "PushBase")) {
            s32 index;
            if (!checkHitStick(this, pSelf, pOther, &pushDir, &index)) {
                return;
            }

            rc::sendMsgPushDir(pOther, pSelf, pushDir);
            al::sendMsgVanish(pOther, pSelf);
            if (al::isVelocityFast(al::getSensorHost(pOther), 5.0f)) {
                tryShakePole(index > 2 ? -8.0f : -4.0f);
            }

            return;
        }
    }

    al::sendMsgPushStrong(pOther, pSelf);
}

/**
 * @brief Start shaking the pole if it is resting.
 * @param angle Initial bend angle of the shake.
 */
void FlingPole::tryShakePole(f32 angle) {
    if (!isIdle()) {
        return;
    }

    if (al::isNerve(this, &NrvFlingPoleDamping)) {
        if (mShakeCoolTime > 0) {
            return;
        }

        mShakeCoolTime = 20;
        mRotateAngle = angle;
        return;
    }

    mRotateAngle = angle;
    al::setNerve(this, &NrvFlingPoleDamping);
    mShakeCoolTime = 20;
}

/**
 * @brief Check whether the player hangs on the side of the pole.
 * @return Whether the player hangs on the pole.
 */
bool FlingPole::isBindPole() const {
    return al::isNerve(this, &NrvFlingPoleBindPoleWait) ||
           al::isNerve(this, &NrvFlingPoleBindPoleClimb) ||
           al::isNerve(this, &NrvFlingPoleBindPoleDown);
}

/**
 * @brief Enable or disable grabbing and pushing the pole.
 * @param isDisabled Whether to disable the pole.
 */
void FlingPole::setDisabled(bool isDisabled) {
    mIsDisabled = isDisabled;
    if (isDisabled) {
        al::invalidateHitSensor(this, "Push");
        al::invalidateHitSensor(this, "PushBase");
        al::invalidateHitSensor(this, "PushPole");
        al::invalidateHitSensor(this, "BindBase");
        al::invalidateHitSensor(this, "BindJoint3");
        al::invalidateHitSensor(this, "BindJoint4");
        al::invalidateHitSensor(this, "BindJoint5");
        al::invalidateHitSensor(this, "Top");
        return;
    }

    al::validateHitSensor(this, "Push");
    al::validateHitSensor(this, "PushBase");
    al::validateHitSensor(this, "PushPole");
    if (isIdle()) {
        al::validateHitSensor(this, "BindBase");
        al::validateHitSensor(this, "BindJoint3");
        al::validateHitSensor(this, "BindJoint4");
        al::validateHitSensor(this, "BindJoint5");
        al::validateHitSensor(this, "Top");
    }
}

/**
 * @brief Check whether the pole is free and (almost) at rest.
 * @return Whether the pole is idle.
 */
bool FlingPole::isIdle() const {
    if (al::isNerve(this, &NrvFlingPoleWait)) {
        return true;
    }

    return al::isNerve(this, &NrvFlingPoleDamping) && al::isGreaterStep(this, 20);
}

/**
 * @brief Handle the player grabbing the pole and attacks shaking it.
 * @param pMsg The message.
 * @param pOther The sending sensor.
 * @param pSelf The pole's sensor.
 * @return Whether the message was handled.
 */
bool FlingPole::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                           al::HitSensor* pSelf) {
    if (al::isMsgBindStart(pMsg)) {
        if (mPuppet != nullptr) {
            return false;
        }

        if (canBind() && al::isSensorPlayer(pOther) && !rc::isPlayerHoldingSomething(pOther) &&
            !rc::isAnyActiveDemo(al::getSensorHost(pOther))) {
            return !mIsDisabled;
        }

        return false;
    }

    if (al::isMsgBindInit(pMsg)) {
        al::invalidateClipping(this);
        al::invalidateHitSensor(this, "BindBase");
        al::invalidateHitSensor(this, "BindJoint3");
        al::invalidateHitSensor(this, "BindJoint4");
        al::invalidateHitSensor(this, "BindJoint5");
        al::invalidateHitSensor(this, "Top");
        mPuppet = rc::startPuppet(pSelf, pOther);
        rc::tryRequestClearFlingPoleDashFlag(al::getSensorHost(pOther));
        rc::tryRequestClearDashFlag(al::getSensorHost(pOther));
        calcNearestPos();
        mClimbSpeed = 0.0f;
        mIsPulling = false;
        mRotateAngleVel = 0.0f;
        if (al::isSensorName(pSelf, "Top") ||
            (rc::getPuppetTrans(mPuppet) - calcTop()).length() < al::getSensorRadius(this, "Top")) {
            setPuppetOnTop();
            mRotateAngle = -3.0f;
            al::setNerve(this, &NrvFlingPoleBindTop);
            mWobbleSeTime = 27;
        } else {
            mRotateAngle = -3.0f;
            al::setNerve(this, &NrvFlingPoleBindPoleWait);
            mWobbleSeTime = 27;
            mIsFirstCatch = true;
        }

        return true;
    }

    if (al::isMsgBindCancel(pMsg)) {
        if (mPuppet != nullptr) {
            alPadRumbleFunction::stopPadRumbleDirectValue(this, rc::getPuppetInputPort(mPuppet));
        }

        turnOffCameraTarget();
        al::validateClipping(this);
        al::setNerve(this, &NrvFlingPoleDamping);
        rc::disappearGuideGameWindow(this);
        mPuppet = nullptr;
        return true;
    }

    if (mPuppet != nullptr && al::isMsgBindDamage(pMsg)) {
        rc::damagePuppet(mPuppet);
        return true;
    }

    if (al::isMsgPlayerFireBallAttack(pMsg)) {
        tryShakePole(-1.0f);
        rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        return true;
    }

    if (!isIdle()) {
        return false;
    }

    if ((al::isMsgPlayerBoomerangAttack(pMsg) || al::isMsgPlayerSpinAttack(pMsg) ||
         al::isMsgPlayerClimbAttack(pMsg) || al::isMsgPlayerTailAttack(pMsg) ||
         al::isMsgPlayerBodyAttackReflect(pMsg)) &&
        al::isSensorName(pSelf, "PushBase") &&
        al::isHitCylinderSensor(pOther, pSelf, sead::Vector3f::ey, 50.0f)) {
        tryShakePole(-1.0f);
        return true;
    }

    if (al::isMsgPlayerClimbRollingAttack(pMsg) || al::isMsgPlayerClimbSlidingAttack(pMsg) ||
        al::isMsgPlayerObjHipDropAll(pMsg) || al::isMsgPlayerObjRollingAttack(pMsg) ||
        al::isMsgExplosion(pMsg)) {
        tryShakePole(-4.0f);
        return true;
    }

    if (al::isMsgBallAttack(pMsg) || rc::isMsgSkateShoesAttack(pMsg) ||
        al::isMsgEnemyAttackFire(pMsg) || rc::isMsgItemReflect(pMsg) ||
        al::isMsgKickKouraReflect(pMsg)) {
        tryShakePole(-1.0f);
        return true;
    }

    return false;
}

/**
 * @brief Check whether the player may grab the pole.
 * @return Whether the pole can be grabbed.
 */
bool FlingPole::canBind() const {
    if (al::isNerve(this, &NrvFlingPoleWait)) {
        return true;
    }

    return al::isNerve(this, &NrvFlingPoleDamping) && al::isGreaterStep(this, 40);
}

/**
 * @brief Snap the hanging player to the nearest point of the pole.
 */
void FlingPole::calcNearestPos() {
    bool isMini = rc::isPlayerMini(rc::getPuppetSensor(mPuppet));
    sead::Vector3f frontOffset = rc::getPuppetFrontVec(mPuppet) * (isMini ? -30.0f : -15.0f);
    sead::Vector3f trans = rc::getPuppetTrans(mPuppet);

    sead::Vector3f start;
    sead::Vector3f end;
    for (s32 i = 0; i < 5; i++) {
        start = calcJointAt(i) + frontOffset;
        end = calcJointAt(i + 1) + frontOffset;
        sead::Vector3f dir = end - start;
        f32 length = dir.length();
        al::normalize(&dir);
        f32 dot = (trans - start).dot(dir);
        if (dot >= 0.0f && dot < length) {
            rc::setPuppetTrans(mPuppet, start + dir * dot);
            return;
        }
    }

    start = calcBottom() + frontOffset;
    end = calcTop() + frontOffset;
    if ((trans - start).length() < (trans - end).length()) {
        rc::setPuppetTrans(mPuppet, start);
    } else {
        rc::setPuppetTrans(mPuppet, end);
    }
}

/**
 * @brief Calculate the position the player stands at on top of the pole.
 * @return The top position.
 */
sead::Vector3f FlingPole::calcTop() const {
    sead::Vector3f pos;
    al::calcJointPos(&pos, this, cStickJointNames[5]);
    pos += sead::Vector3f(0.0f, 30.0f, 0.0f);
    return pos;
}

/**
 * @brief Move the player hanging on the pole onto its top.
 */
void FlingPole::setPuppetOnTop() {
    if (!isBindPole()) {
        return;
    }

    rc::setPuppetTrans(mPuppet, calcTop());
    mIsRequestTop = false;
}

/**
 * @brief Stop the camera from following the pole.
 */
void FlingPole::turnOffCameraTarget() {
    if (!mIsCameraTargetOn) {
        return;
    }

    mCameraTarget->setNoCameraReset(false);
    mCameraTarget->offTarget();
    mCameraTarget->setRequestDistance(-1.0f);
    mIsCameraTargetOn = false;
}

/**
 * @brief Move the bound player and update the connection and the flag.
 */
void FlingPole::control() {
    if (mShakeCoolTime > 0) {
        mShakeCoolTime--;
    }

    if (isBindPole()) {
        if (mIsRequestTop) {
            setPuppetOnTop();
            al::setNerve(this, &NrvFlingPoleBindTop);
            return;
        }

        rc::setPuppetTrans(mPuppet,
                           rc::getPuppetTrans(mPuppet) + sead::Vector3f(0.0f, mClimbSpeed, 0.0f));
        calcNearestPos();
        mClimbSpeed = 0.0f;
        if (isOnTop()) {
            al::setNerve(this, &NrvFlingPoleBindTop);
            return;
        }
    }

    if (canJump() && rc::isPuppetTrigJumpButton(mPuppet)) {
        if (al::isNerve(this, &NrvFlingPoleBindTop)) {
            rc::disappearGuideGameWindow(this);
        }

        alPadRumbleFunction::stopPadRumbleDirectValue(this, rc::getPuppetInputPort(mPuppet));
        al::setNerve(this, &NrvFlingPoleJump);
        return;
    }

    if (mConnector != nullptr) {
        al::calcConnectQT(&mBaseQuat, al::getTransPtr(this), mConnector,
                          al::getConnectBaseQuat(mConnector), al::getConnectBaseTrans(mConnector));
    }

    if (mIsLighthouseFlag) {
        if (isBindPole() || isOnTop()) {
            sead::Vector3f flagPos;
            al::calcJointPos(&flagPos, this, cFlagJointNames[0]);
            if (mPuppet != nullptr && rc::getPuppetTrans(mPuppet).y > flagPos.y + -280.0f) {
                al::tryStartMclAnimIfNotPlaying(this, "FlagHide");
            } else {
                al::tryStartMclAnimIfNotPlaying(this, "FlagShow");
            }
        } else if (isIdle()) {
            al::tryStartMclAnimIfNotPlaying(this, "FlagShow");
        }
    }

    if (mGuideType != GuideType::None && !rc::isCurrentGuideGameWindowUser(this)) {
        mGuideType = GuideType::None;
    }
}

/**
 * @brief Check whether the bound player is at the height of the top of the pole.
 * @return Whether the player is on top.
 */
bool FlingPole::isOnTop() const {
    if (mPuppet == nullptr) {
        return false;
    }

    sead::Vector3f top = calcTop();
    sead::Vector3f pos(top.x, rc::getPuppetTrans(mPuppet).y, top.z);
    return (pos - top).length() < 200.0f;
}

/**
 * @brief Check whether the bound player may jump off.
 * @return Whether the player can jump.
 */
bool FlingPole::canJump() const {
    if (mPuppet == nullptr) {
        return false;
    }

    if (isBindPole()) {
        return true;
    }

    return al::isNerve(this, &NrvFlingPoleBindTop);
}

/**
 * @brief Get the position of the bound player.
 * @param pTrans Receives the position.
 * @return Whether a player is bound.
 */
bool FlingPole::calcPuppetTrans(sead::Vector3f* pTrans) const {
    if (mPuppet == nullptr) {
        return false;
    }

    pTrans->set(rc::getPuppetTrans(mPuppet));
    return true;
}

/**
 * @brief Switch the flag animation when disaster mode starts or ends.
 * @param state The new disaster mode state.
 */
void FlingPole::onDisasterModeStateChange(DisasterModeController::State state) {
    if (state == DisasterModeController::State::Disaster ||
        state == DisasterModeController::State::Normal) {
        DisasterModeAnimUpdate();
    }
}

/**
 * @brief Play the flag flap matching the current disaster mode state.
 */
void FlingPole::DisasterModeAnimUpdate() {
    if (!mIsLighthouseFlag) {
        return;
    }

    DisasterModeController* controller = DisasterModeController::tryGetController(this);
    if (controller == nullptr) {
        return;
    }

    if (controller->isDisasterModeAnim()) {
        if (al::isSklAnimPlaying(this, "LighthouseFlagFlap", 0)) {
            al::tryStartSklAnimIfExist(this, "LighthouseFlagFlapDisasterMode");
        }
    } else {
        if (al::isSklAnimPlaying(this, "LighthouseFlagFlapDisasterMode", 0)) {
            al::tryStartSklAnimIfExist(this, "LighthouseFlagFlap");
        }
    }
}

/**
 * @brief Show or hide the lighthouse flag.
 * @param isShow Whether to show the flag.
 */
void FlingPole::showFlag(bool isShow) {
    if (isShow) {
        al::tryStartMclAnimIfNotPlaying(this, "FlagShow");
    } else {
        al::tryStartMclAnimIfNotPlaying(this, "FlagHide");
    }
}

/**
 * @brief Put the pole back at rest.
 */
void FlingPole::startNrvWait() {
    al::setNerve(this, &NrvFlingPoleWait);
}

/**
 * @brief Make the bind sensors grabbable again unless the pole is disabled.
 */
inline void FlingPole::validateBindSensors() {
    if (mIsDisabled) {
        return;
    }

    al::validateHitSensor(this, "BindBase");
    al::validateHitSensor(this, "BindJoint3");
    al::validateHitSensor(this, "BindJoint4");
    al::validateHitSensor(this, "BindJoint5");
    al::validateHitSensor(this, "Top");
}

/**
 * @brief Rest and track the height of a nearby player.
 */
void FlingPole::exeWait() {
    if (al::isFirstStep(this)) {
        mIsFlipped = false;
        validateBindSensors();
        if (mPuppet != nullptr) {
            alPadRumbleFunction::stopPadRumbleDirectValue(this, rc::getPuppetInputPort(mPuppet));
        }
    }

    DisasterModeAnimUpdate();

    f32 bottomY = calcBottom().y;
    f32 topY = calcTop().y;
    al::LiveActor* player =
        rc::tryFindNearestActivePlayerActorInCylinder(this, 200.0f, 0.0f, topY - bottomY);
    if (player != nullptr) {
        mPlayerPos.set(al::getTrans(this));
        mPlayerPos.y = sead::Mathf::clamp(al::getTrans(player).y, bottomY, topY);
    }
}

/**
 * @brief Calculate the lowest point of the pole the player can hang at.
 * @return The bottom position.
 */
sead::Vector3f FlingPole::calcBottom() const {
    sead::Vector3f pos;
    al::calcJointPos(&pos, this, cStickJointNames[0]);
    pos += sead::Vector3f(0.0f, -50.0f, 0.0f);
    return pos;
}

/**
 * @brief Calculate how strongly the pole swings, for its sounds.
 * @return The swing rate in [0, 1].
 */
inline f32 FlingPole::calcWobbleSeRate() const {
    f32 rate = al::normalizeAbs(mRotateAngle, 0.0f, 24.0f);
    return sead::Mathf::abs(rate * rate * rate);
}

/**
 * @brief Keep the wobble sound playing while the pole swings.
 */
inline void FlingPole::holdWobbleSe() {
    f32 rate = calcWobbleSeRate();
    f32 pitch = sead::lerp(0.9f, 1.0f, rate);
    f32 volume = calcWobbleSeVolume(rate);
    al::holdSeSetPitchVolumeByName(this, "Wobble", pitch, volume);
}

/**
 * @brief Hang on the side of the pole and wait for stick input.
 */
void FlingPole::exeBindPoleWait() {
    if (al::isFirstStep(this)) {
        if (mIsFirstCatch) {
            rc::startPuppetAction(mPuppet, "FlingPoleCatch");
        }

        mRotateAxis.set(rc::getPuppetFrontVec(mPuppet));
        al::rotateVectorDegreeY(&mRotateAxis, -90.0f);
        al::normalizeOrDirZ(&mRotateAxis);
        mIsFirstCatch = false;
    }

    controlSpring(0.92f);
    if (mWobbleSeTime-- > 0) {
        holdWobbleSe();
    }

    f32 stickX = rc::getPuppetStickX(mPuppet);
    f32 stickY = rc::getPuppetStickY(mPuppet);
    if (al::isNearZero(stickX, 0.3f)) {
        if (rc::isPuppetAction(mPuppet, "FlingPoleCatch") && rc::isPuppetActionEnd(mPuppet)) {
            rc::startPuppetAction(mPuppet, "FlingPoleWait");
        } else if (!rc::isPuppetAction(mPuppet, "FlingPoleCatch") &&
                   !rc::isPuppetAction(mPuppet, "FlingPoleWait")) {
            rc::startPuppetAction(mPuppet, "FlingPoleWait");
        }

        if (al::isNearZero(stickY, 0.001f)) {
            return;
        }
    }

    if (sead::Mathf::abs(stickX) < sead::Mathf::abs(stickY)) {
        if (stickY < 0.0f) {
            al::setNerve(this, &NrvFlingPoleBindPoleDown);
        } else {
            al::setNerve(this, &NrvFlingPoleBindPoleClimb);
        }

        return;
    }

    if (al::isNearZero(stickX, 0.3f)) {
        return;
    }

    sead::Vector3f front = rc::getPuppetFrontVec(mPuppet);
    if (stickX < 0.0f) {
        al::rotateVectorDegreeY(&front, -3.0f);
    } else {
        al::rotateVectorDegreeY(&front, 3.0f);
    }

    if (rc::isPuppetAction(mPuppet, "FlingPoleWait")) {
        rc::startPuppetAction(mPuppet, "FlingPoleClimb");
    }

    rc::setPuppetFrontVec(mPuppet, front);
}

/**
 * @brief Advance the damped spring bending the pole.
 * @param damping Damping applied to the bend angle.
 */
void FlingPole::controlSpring(f32 damping) {
    f32 angle = mRotateAngle * damping;
    mRotateAngleVel += angle * -0.5f;
    mRotateAngle = angle + mRotateAngleVel;
}

/**
 * @brief Climb up the pole.
 */
void FlingPole::exeBindPoleClimb() {
    if (rc::isPuppetAction(mPuppet, "FlingPoleCatch") && rc::isPuppetActionEnd(mPuppet)) {
        rc::startPuppetAction(mPuppet, "FlingPoleClimb");
    } else if (!rc::isPuppetAction(mPuppet, "FlingPoleClimb")) {
        rc::startPuppetAction(mPuppet, "FlingPoleClimb");
    }

    controlSpring(0.92f);
    if (mWobbleSeTime-- > 0) {
        holdWobbleSe();
    }

    f32 stickX = rc::getPuppetStickX(mPuppet);
    f32 stickY = rc::getPuppetStickY(mPuppet);
    if (al::isNearZero(stickY, 0.001f)) {
        al::setNerve(this, &NrvFlingPoleBindPoleWait);
        return;
    }

    if (stickY < 0.0f) {
        al::setNerve(this, &NrvFlingPoleBindPoleDown);
        return;
    }

    mClimbSpeed = stickY * 8.0f;
    if (!al::isNearZero(stickX, 0.3f)) {
        sead::Vector3f front = rc::getPuppetFrontVec(mPuppet);
        if (stickX < 0.0f) {
            al::rotateVectorDegreeY(&front, -3.0f);
        } else {
            al::rotateVectorDegreeY(&front, 3.0f);
        }

        rc::setPuppetFrontVec(mPuppet, front);
    }
}

/**
 * @brief Slide down the pole, falling off at its bottom.
 */
void FlingPole::exeBindPoleDown() {
    if (rc::isPuppetAction(mPuppet, "FlingPoleCatch") && rc::isPuppetActionEnd(mPuppet)) {
        rc::startPuppetAction(mPuppet, "FlingPoleFall");
    } else if (!rc::isPuppetAction(mPuppet, "FlingPoleFall")) {
        rc::startPuppetAction(mPuppet, "FlingPoleFall");
    }

    controlSpring(0.92f);
    if (mWobbleSeTime-- > 0) {
        holdWobbleSe();
    }

    f32 stickX = rc::getPuppetStickX(mPuppet);
    f32 stickY = rc::getPuppetStickY(mPuppet);
    if (al::isNearZero(stickY, 0.001f)) {
        al::setNerve(this, &NrvFlingPoleBindPoleWait);
        return;
    }

    if (stickY > 0.0f) {
        al::setNerve(this, &NrvFlingPoleBindPoleClimb);
        return;
    }

    mClimbSpeed = stickY * 14.0f;
    if (!al::isNearZero(stickX, 0.3f)) {
        sead::Vector3f front = rc::getPuppetFrontVec(mPuppet);
        if (stickX < 0.0f) {
            al::rotateVectorDegreeY(&front, -3.0f);
        } else {
            al::rotateVectorDegreeY(&front, 3.0f);
        }

        rc::setPuppetFrontVec(mPuppet, front);
    }

    if (!isOnBottom()) {
        return;
    }

    rc::startPuppetAction(mPuppet, "Fall");
    mRotateAngle = -0.3f;
    mWobbleSeTime = 0;
    const sead::Vector3f& front = rc::getPuppetFrontVec(mPuppet);
    sead::Vector3f velocity(-front.x, 0.2f, -front.z);
    al::normalize(&velocity);
    velocity *= 15.0f;
    rc::setPuppetVelocity(mPuppet, velocity);
    rc::endBindAndPuppetNull(&mPuppet, &cBindEndParamFall);
    al::setNerve(this, &NrvFlingPoleDamping);
}

/**
 * @brief Check whether the bound player reached the bottom of the pole.
 * @return Whether the player is at the bottom.
 */
bool FlingPole::isOnBottom() const {
    if (mPuppet == nullptr) {
        return false;
    }

    sead::Vector3f bottom = calcBottom();
    sead::Vector3f pos(bottom.x, rc::getPuppetTrans(mPuppet).y, bottom.z);
    return (pos - bottom).length() < 50.0f;
}

/**
 * @brief Stand on top of the pole, bend it with the stick and release it to fling the player.
 */
void FlingPole::exeBindTop() {
    sead::Vector3f up;
    al::calcUpDir(&up, this);
    if (al::isFirstStep(this)) {
        rc::startPuppetAction(mPuppet, "TreeHandStandStart");
        turnOnCameraTarget();
        mRotateAxis.set(rc::getPuppetFrontVec(mPuppet));
        al::rotateVectorDegreeY(&mRotateAxis, -90.0f);
        al::normalizeOrDirZ(&mRotateAxis);
    }

    sead::Vector3f top = calcTop();
    rc::setPuppetTrans(mPuppet, top);
    if (!rc::isPuppetAction(mPuppet, "TreeHandStandWait") && rc::isPuppetActionEnd(mPuppet)) {
        rc::startPuppetAction(mPuppet, "TreeHandStandWait");
    }

    if (mIsPulling) {
        mRotateAngle *= 0.94f;
    } else {
        mRotateAngle = sead::Mathf::clamp(mRotateAngle, -1.0f, 1.0f);
        controlSpring(0.94f);
        if (mWobbleSeTime-- > 0) {
            holdWobbleSe();
        }
    }

    mTurnAngle *= 0.8f;
    mCameraOffsetY *= 0.9f;

    f32 snapRate = 0.3f;
    f32 stickX = 0.0f;
    if (!al::isNearZero(rc::getPuppetStickX(mPuppet), snapRate)) {
        stickX = copysignf(calcStickRate(rc::getPuppetStickX(mPuppet)),
                           rc::getPuppetStickX(mPuppet));
    }

    s32 port = rc::getPuppetInputPort(mPuppet);
    f32 reverseV = SingleModeDataFunction::getCameraReverseVertical(this) ? -1.0f : 1.0f;
    f32 reverseH = SingleModeDataFunction::getCameraReverseHorizontal(this) ? -1.0f : 1.0f;
    f32 rightStickX = al::getRightStick(port).x;
    f32 rightStickY = al::getRightStick(port).y;
    f32 stickY = 0.0f;
    if (mParam->mFlingPower > 0.0f) {
        stickY = rc::getPuppetStickY(mPuppet);
    }

    sead::Vector3f pullDir(0.0f, 0.0f, stickY);
    f32 power = sead::Mathf::clamp(
        pullDir.length() + sead::Mathf::clamp(stickX * stickX - 0.25f, 0.0f, 1.0f), 0.0f, 1.0f);
    f32 turnInput = reverseH * rightStickX;
    f32 heightInput = reverseV * rightStickY;
    al::rotateVectorQuat(&pullDir, mBaseQuat);
    f32 turn = sead::Mathf::clamp(stickX * (mIsPulling ? 2.0f : 1.0f), -1.0f, 1.0f);
    mTurnAngle += (turn - turnInput) * 0.35f;
    mCameraOffsetY =
        sead::Mathf::clamp(mCameraOffsetY + heightInput * -65.0f, -500.0f, 700.0f);

    if (mIsPulling && findBestInRangeSnapPoint()) {
        sead::Vector3f trans = al::getTrans(this);
        sead::Vector3f snapTrans = mBestSnapPoint->mTrans;
        sead::Vector3f hitPos = mBestSnapPoint->mHitPos;
        sead::Vector3f enterPos = mBestSnapPoint->mEnterPos;
        sead::Vector3f dir(trans.x - snapTrans.x, 0.0f, trans.z - snapTrans.z);
        dir.normalize();
        sead::Vector3f edge =
            sead::Vector3f(snapTrans.x, 0.0f, snapTrans.z) + dir * mBestSnapPoint->mRadius;
        sead::Vector3f hitPosXZ(hitPos.x, 0.0f, hitPos.z);
        // The distance from the edge is computed twice; the first result is unused.
        (edge - hitPosXZ).length();
        f32 distance = (hitPosXZ - edge).length();
        sead::Vector2f toEnter(enterPos.x - trans.x, enterPos.z - trans.z);
        sead::Vector2f toCenter(snapTrans.x - trans.x, snapTrans.z - trans.z);
        f32 angle = al::calcAngleDegree(toEnter, toCenter);
        f32 rate = distance / (mBestSnapPoint->mRadius * snapRate);
        snapRate = sead::Mathf::clamp(rate * (1.0f / rate), snapRate,
                                      distance < 120.0f ? 0.2f : 2.0f);
        if (distance > 60.0f) {
            mTurnAngle += copysignf(snapRate * 0.1f * mBestSnapPoint->mStrength, -angle);
        }
    }

    al::rotateVectorDegreeY(&mRotateAxis, mTurnAngle);
    if (!mIsPulling && !al::isNearZero(mTurnAngle, 0.1f)) {
        if (rc::isPuppetAction(mPuppet, "TreeHandStandWait")) {
            rc::startPuppetAction(mPuppet, "TreeHandStandTurn");
        }
    } else if (rc::isPuppetAction(mPuppet, "TreeHandStandTurn")) {
        rc::startPuppetAction(mPuppet, "TreeHandStandWait");
    }

    if (mIsPulling) {
        mPullDir = mPullDir * 0.5f + pullDir * 0.5f;
        al::normalizeOrZero(&mPullDir);
        sead::Vector3f side;
        side.setCross(sead::Vector3f::ey, mPullDir);
        al::normalizeOrDirZ(&side);
        mRotateAngle = sead::Mathf::min(24.0f, power * 1.6f + mRotateAngle);
        mRotateAngleVel = 0.0f;
        if (!al::tryNormalizeOrZero(&mRotateAxis)) {
            al::calcSideDir(&mRotateAxis, this);
        }

        for (s32 i = 0; i < mStickRotators.size(); i++) {
            mStickRotators[i]->setAxis(mRotateAxis);
        }
    }

    bool isStrongBend = mIsPulling && mRotateAngle > 10.0f;

    sead::Vector3f front = mRotateAxis;
    al::rotateVectorDegreeY(&front, 90.0f);
    al::normalizeOrDirZ(&front);
    rc::setPuppetFrontVec(mPuppet, front);
    if (isStrongBend) {
        f32 pitch = sead::Mathf::max(40.0f, power * 135.0f);
        alPadRumbleFunction::startPadRumbleDirectValue(this, 40.0f, pitch, 0.0f, 0.7f, power, power,
                                                       port);
    } else {
        alPadRumbleFunction::stopPadRumbleDirectValue(this, port);
    }

    if (mIsPulling) {
        f32 rate = al::normalizeAbs(mRotateAngle, 0.0f, 24.0f);
        rate = rate * rate * rate;
        f32 pitch = sead::lerp(0.85f, 1.4f, rate);
        f32 volume = sead::Mathf::clamp(sead::lerp(0.0f, 50.0f, rate), 0.0f, 1.0f);
        al::holdSeSetPitchVolumeByName(this, "Bending", pitch, volume);
        mWobbleSeTime = sead::Mathf::abs(mRotateAngle) > 0.5f ? 27 : 0;
    } else {
        al::stopSeByName(this, "Bending");
    }

    if (isStrongBend && power < 0.1f) {
        alPadRumbleFunction::stopPadRumbleDirectValue(this, port);
        mIsFlipped = true;
        if (rc::isCurrentGuideGameWindowUser(this)) {
            mGuideType = GuideType::None;
            rc::disappearGuideGameWindow(this);
        }

        al::setNerve(this, &NrvFlingPoleShoot);
        return;
    }

    bool isSingleJoycon = al::isPadTypeJoySingle(al::getMainControllerPort());
    switch (mGuideType) {
    case GuideType::None:
        if (isSingleJoycon) {
            if (rc::appearGuideGameWindowWithPriority(this, "SingleMode_GuideMessage",
                                                      "FlingPoleGuide_SingleJoycons",
                                                      GuideMessagePriority(1), -1, 0.0f)) {
                mGuideType = GuideType::SingleJoycons;
            }
        } else if (rc::appearGuideGameWindowWithPriority(this, "SingleMode_GuideMessage",
                                                         "FlingPoleGuide_DualJoycons",
                                                         GuideMessagePriority(1), -1, 0.0f)) {
            mGuideType = GuideType::DualJoycons;
        }

        break;
    case GuideType::DualJoycons:
        if (isSingleJoycon &&
            rc::appearGuideGameWindowWithPriority(this, "SingleMode_GuideMessage",
                                                  "FlingPoleGuide_SingleJoycons",
                                                  GuideMessagePriority(1), -1, 0.0f)) {
            mGuideType = GuideType::SingleJoycons;
        }

        break;
    case GuideType::SingleJoycons:
        if (!isSingleJoycon &&
            rc::appearGuideGameWindowWithPriority(this, "SingleMode_GuideMessage",
                                                  "FlingPoleGuide_DualJoycons",
                                                  GuideMessagePriority(1), -1, 0.0f)) {
            mGuideType = GuideType::DualJoycons;
        }

        break;
    }

    f32 bendRate = al::normalizeAbs(mRotateAngle, 0.0f, 24.0f);
    sead::Vector3f trans = al::getTrans(this);
    f32 atHeight = al::lerpValue(bendRate, 200.0f, 100.0f);
    f32 posHeight = al::lerpValue(bendRate, 600.0f, 200.0f);
    f32 distance = al::lerpValue(bendRate, 1000.0f, 750.0f) * mParam->mCameraDist;
    mCameraAt = sead::Vector3f(trans.x, top.y, trans.z) + up * atHeight;
    sead::Vector3f cameraPos = top + up * posHeight + up * mCameraOffsetY;
    mCameraPos = cameraPos - rc::getPuppetFrontVec(mPuppet) * distance;
    mIsPulling = rc::getPuppetStickY(mPuppet) < -0.101f;
    al::setTrans(mCameraTarget, rc::getPuppetTrans(mPuppet));

    al::CameraPoser_RS* poser =
        mActorSceneInfo->cameraDirector->getCurrentTicket()->getPoser();
    mCameraTarget->setRequestDistance(distance);
    if (al::isEqualString(poser->getName(), "Follow") ||
        al::isEqualString(poser->getName(), "Parallel")) {
        // The camera's yaw is computed here but never used (the game also lerps it towards a
        // poser-specific angle and drops the result, which is omitted here).
        al::wrapAngle(sead::Mathf::rad2deg(
                          atan2f(mCameraAt.x - mCameraPos.x, mCameraAt.z - mCameraPos.z)) +
                      180.0f);
        al::CameraTurnInfo turnInfo;
        turnInfo.mRequesterName = mCameraTarget->getName();
        turnInfo.mDir = front;
        turnInfo._14 = 0.3f;
        turnInfo._18 = 0.5f;
        turnInfo._1c = true;
        turnInfo._1d = false;
        poser->requestTurnToDirection(&turnInfo);
    }
}

/**
 * @brief Make the camera follow the pole.
 */
void FlingPole::turnOnCameraTarget() {
    if (mIsCameraTargetOn) {
        return;
    }

    mCameraTarget->onTarget();
    mCameraTarget->setNoCameraReset(true);
    mIsCameraTargetOn = true;
}

/**
 * @brief Find the first snap point in front of the camera.
 * @return Whether a snap point was found.
 */
bool FlingPole::findBestInRangeSnapPoint() {
    mBestSnapPoint = nullptr;
    for (s32 i = 0; i < mSnapPoints.size(); i++) {
        mSnapPoints(i)->mIsHit = false;
        const SnapPoint* snapPoint = mSnapPoints[i];
        sead::Vector3f hitNormal;
        sead::Vector3f hitPos;
        sead::Vector3f enterPos;
        sead::Vector3f center = snapPoint->mTrans;
        center.y = 0.0f;
        sead::Vector3f start = al::getTrans(this);
        start.y = 0.0f;
        sead::Vector3f dir(mCameraAt.x - mCameraPos.x, 0.0f, mCameraAt.z - mCameraPos.z);
        dir.normalize();
        sead::Vector3f end = start + dir * 999999.0f;
        f32 radius = mSnapPoints(i)->mRadius;
        if (al::checkHitSegmentSphereNearDepth(center, start, end, radius, &hitPos, &hitNormal)) {
            al::checkHitSegmentSphere(center, start, end, radius, nullptr, &enterPos);
            enterPos.y = 0.0f;
            hitNormal.y = 0.0f;
            hitPos.y = 0.0f;
            hitNormal.normalize();

            SnapPoint* hitPoint = mSnapPoints(i);
            hitPoint->mIsHit = true;
            hitPoint->mHitPos.set(hitPos);
            hitPoint->mHitNormal = hitNormal;
            hitPoint->mEnterPos.set(enterPos);
            mBestSnapPoint = mSnapPoints[i];
            return true;
        }
    }

    return false;
}

/**
 * @brief Swing the pole up and fling the player off its top.
 */
void FlingPole::exeShoot() {
    if (al::isFirstStep(this)) {
        mShootRate = sead::Mathf::clamp((sead::Mathf::abs(mRotateAngle) + 4.0f) / 24.0f,
                                        1.0f / 3.0f, 1.0f);
    }

    controlSpring(0.92f);
    rc::setPuppetTrans(mPuppet, calcTop());
    if (mRotateAngle < 0.0f) {
        mIsFlipped = true;
        f32 upY = mCameraOffsetY < -250.0f ? 1.5f : 1.0f;
        upY = mCameraOffsetY > 350.0f ? 0.7f : upY;
        shoot(mShootRate * 60.0f * mParam->mFlingPower, upY);
        al::setNerve(this, &NrvFlingPoleDamping);
    }
}

/**
 * @brief Release the player with a velocity.
 * @param speed Speed of the player.
 * @param upY Vertical part of the direction before normalizing.
 */
void FlingPole::shoot(f32 speed, f32 upY) {
    calcNearestPos();
    sead::Vector3f front = rc::getPuppetFrontVec(mPuppet);
    if (!isOnTop()) {
        front = -front;
        front.y = 0.0f;
        al::normalize(&front);
    }

    sead::Vector3f velocity = front;
    velocity.y = upY;
    al::normalize(&velocity);
    velocity *= speed;
    rc::setPuppetVelocity(mPuppet, velocity);
    rc::setPuppetFrontVec(mPuppet, front);
    if (mIsFlipped) {
        mBindEndParam->_44 = mParam->mFlingDashTime;
        rc::startPuppetAction(mPuppet, "RollingAir");
    } else {
        mBindEndParam->_44 = 0.0f;
        rc::startPuppetAction(mPuppet, speed > 0.0f ? "Jump" : "Fall");
    }

    rc::endBindAndPuppetNull(&mPuppet, mBindEndParam);
    turnOffCameraTarget();
}

/**
 * @brief Let the player jump off the pole.
 */
void FlingPole::exeJump() {
    controlSpring(0.92f);
    mIsFlipped = false;
    if (mIsPulling) {
        shoot(0.0f, 1.0f);
        al::setNerve(this, &NrvFlingPoleDamping);
        return;
    }

    sead::Vector3f velocity;
    sead::Vector3f front = rc::getPuppetFrontVec(mPuppet);
    f32 speed;
    if (isOnTop()) {
        rc::startPuppetAction(mPuppet, "TreeHandStandJump");
        speed = 8.0f;
    } else {
        rc::startPuppetAction(mPuppet, "JumpTree");
        rc::setPuppetFrontVec(mPuppet, -front);
        f32 backOffset = rc::isPlayerMini(rc::getPuppetSensor(mPuppet)) ? 40.0f : 80.0f;
        sead::Vector3f trans =
            rc::getPuppetTrans(mPuppet) - front * 40.0f - sead::Vector3f::ey * backOffset;
        rc::setPuppetTrans(mPuppet, trans);
        speed = -8.0f;
    }

    velocity.set(front.x * speed, 35.0f, front.z * speed);
    rc::setPuppetVelocity(mPuppet, velocity);
    if (isOnTop()) {
        rc::endBindAndPuppetNull(&mPuppet, &cBindEndParamJumpTop);
    } else {
        rc::endBindAndPuppetNull(&mPuppet, &cBindEndParamJumpSide);
    }

    turnOffCameraTarget();
    mRotateAngle = -3.0f;
    al::setNerve(this, &NrvFlingPoleDamping);
}

/**
 * @brief Let the pole swing out after it was released or hit.
 */
void FlingPole::exeDamping() {
    if (al::isFirstStep(this)) {
        if (sead::Mathf::abs(mRotateAngle) > 0.5f) {
            mWobbleSeTime = mIsFlipped ? 45 : 27;
        } else {
            mWobbleSeTime = 0;
        }
    }

    if (al::isNearZero(mRotateAngleVel, 0.08f) && al::isNearZero(mRotateAngle, 0.08f)) {
        if (al::isNearZero(mStickXAngle, 0.08f)) {
            mStickXAngle = 0.0f;
            mRotateAngle = 0.0f;
            mRotateAngleVel = 0.0f;
            al::validateClipping(this);
            al::setNerve(this, &NrvFlingPoleWait);
            return;
        }

        mStickXAngle *= 0.8f;
    } else {
        controlSpring(0.92f);
        if (mWobbleSeTime-- > 0) {
            f32 rate = calcWobbleSeRate();
            f32 pitch = sead::lerp(0.9f, 1.0f, rate);
            f32 volume = sead::lerp(0.5f, 1.0f, rate);
            if (mIsFlipped) {
                al::holdSeSetPitchVolumeByName(this, "Flipped", pitch,
                                               sead::Mathf::clamp(volume, 0.0f, 1.0f));
            } else {
                al::holdSeSetPitchVolumeByName(this, "Wobble", pitch,
                                               sead::Mathf::clamp(volume, 0.0f, 1.0f));
            }
        }
    }

    if (al::isGreaterEqualStep(this, 40)) {
        validateBindSensors();
    }
}

/**
 * @brief Calculate the position of a joint of the pole.
 * @param index Index of the joint, 0 is the bottom and 5 the top.
 * @return The joint position.
 */
NOINLINE sead::Vector3f FlingPole::calcJointAt(s32 index) const {
    switch (index) {
    case 0:
        return calcBottom();
    case 5:
        return calcTop();
    default:
        break;
    }

    sead::Vector3f pos;
    al::calcJointPos(&pos, this, cStickJointNames[index]);
    return pos;
}
