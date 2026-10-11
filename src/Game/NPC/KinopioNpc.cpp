#include "NPC/KinopioNpc.hpp"

#include <cstdlib>

#include "NPC/IUseNekoModeActor.hpp"
#include "NPC/Neko.hpp"
#include "NPC/NpcHeadController.hpp"
#include "NPC/NpcStateFunction.hpp"
#include "NPC/NpcStateParam.hpp"
#include "NPC/NpcStateWait.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Library/Collision/Collider.hpp"
#include "Library/Collision/CollisionPartsKeeperUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorAnimUtil.hpp"
#include "Library/LiveActor/Util/ActorFlagUtil.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorMovementUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Play/Placement/PlacementHolder.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Yaml/ByamlIter.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Collision/CollisionUtil.hpp"

namespace {
NERVE_DECL(KinopioNpc, Wait)

NERVES_MAKE_NOSTRUCT(KinopioNpc, Wait)

const sead::Vector3f sWaitOffset(0.0f, 150.0f, 0.0f);
const NpcStateWaitParam sWaitParam("Wait", nullptr, "Turn", "Reaction", "ReactionMic",
                                   "TouchJoy", "Trampled", true, &sWaitOffset, false);
const NpcStateTurnParam sTurnParam(5.0f, 0.0f, 3.5f, 3000.0f, true, false, 20);
}  // namespace

/**
 * @brief Construct the Toad.
 * @param pName Name of the actor.
 */
KinopioNpc::KinopioNpc(const char* pName) : al::LiveActor(pName) {}

/**
 * @brief Initialize the actor, apply its coat color and start waiting.
 * @param rInfo Placement info of the actor.
 */
void KinopioNpc::init(const al::ActorInitInfo& rInfo) {
    al::initActor(this, rInfo);
    al::initNerve(this, &NrvKinopioNpcWait, 1);

    s32 color = 0;
    al::tryGetArg(&color, rInfo, "KinopioColor");
    al::startAction(this, "Color");
    al::setMtpAnimFrameAndStop(this, color);

    mStateWait = new NpcStateWait(this, rInfo, &sWaitParam, &sTurnParam, nullptr);
    al::initNerveState(this, mStateWait, &NrvKinopioNpcWait, "NPC待機");
    makeActorAppeared();
}

/**
 * @brief Forward sensor hits to the wait state.
 * @param pSelf Own sensor.
 * @param pOther Sensor that was hit.
 */
void KinopioNpc::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    mStateWait->attackSensor(pSelf, pOther);
}

/**
 * @brief Forward sensor messages to the wait state.
 * @param pMsg The received message.
 * @param pSelf Own sensor.
 * @param pOther Sensor of the sender.
 * @return Whether the message was handled.
 */
bool KinopioNpc::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSelf,
                            al::HitSensor* pOther) {
    return mStateWait->receiveMsg(pMsg, pSelf, pOther);
}

/**
 * @brief Forward screen point messages (touch) to the wait state.
 * @param pMsg The received message.
 * @param pPointer The pointer touching the actor.
 * @param pTarget Own screen point target.
 * @return Whether the message was handled.
 */
bool KinopioNpc::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                       al::ScreenPointTarget* pTarget) {
    return mStateWait->receiveMsgScreenPoint(pMsg, pPointer, pTarget);
}

/** @brief Wait nerve: run the wait state. */
void KinopioNpc::exeWait() {
    al::updateNerveState(this);
}

namespace neko {

/**
 * @brief Create the head controller of a cat from its BYAML parameters.
 * @param pActor The cat mode actor whose joints are controlled.
 * @param iter Head controller parameters.
 * @param pTargetFinder Target finder the head looks at targets of.
 * @param targetType Kinds of targets to look at.
 * @return The created controller, or nullptr if the parameters have no "ActionList".
 */
NpcHeadController* makeHeadController(IUseNekoModeActor* pActor, al::ByamlIter iter,
                                      NpcTargetFinder* pTargetFinder,
                                      npc::NpcFindTargetType targetType) {
    const char* jointName = nullptr;
    iter.tryGetStringByKey(&jointName, "JointName");

    sead::Vector3f jointFront = {0.0f, 0.0f, 0.0f};
    al::ByamlIter jointFrontIter;
    if (iter.tryGetIterByKey(&jointFrontIter, "JointFront")) {
        jointFrontIter.tryGetFloatByKey(&jointFront.x, "X");
        jointFrontIter.tryGetFloatByKey(&jointFront.y, "Y");
        jointFrontIter.tryGetFloatByKey(&jointFront.z, "Z");
    }

    sead::Vector3f jointUp = {0.0f, 0.0f, 0.0f};
    al::ByamlIter jointUpIter;
    if (iter.tryGetIterByKey(&jointUpIter, "JointUp")) {
        jointUpIter.tryGetFloatByKey(&jointUp.x, "X");
        jointUpIter.tryGetFloatByKey(&jointUp.y, "Y");
        jointUpIter.tryGetFloatByKey(&jointUp.z, "Z");
    }

    f32 lookAtRate = 0.15f;
    iter.tryGetFloatByKey(&lookAtRate, "LookAtRate");

    sead::Vector2f lookYawRange = {-50.0f, 50.0f};
    al::ByamlIter lookYawRangeIter;
    if (iter.tryGetIterByKey(&lookYawRangeIter, "LookYawRange")) {
        lookYawRangeIter.tryGetFloatByKey(&lookYawRange.x, "X");
        lookYawRangeIter.tryGetFloatByKey(&lookYawRange.y, "Y");
    }

    sead::Vector2f lookPitchRange = {-30.0f, 30.0f};
    al::ByamlIter lookPitchRangeIter;
    if (iter.tryGetIterByKey(&lookPitchRangeIter, "LookPitchRange")) {
        lookPitchRangeIter.tryGetFloatByKey(&lookPitchRange.x, "X");
        lookPitchRangeIter.tryGetFloatByKey(&lookPitchRange.y, "Y");
    }

    const char* spineJointName = nullptr;
    sead::Vector3f spineJointFront = {0.0f, 0.0f, 0.0f};
    sead::Vector3f spineJointUp = {0.0f, 0.0f, 0.0f};
    f32 spineLookAtRate = 0.15f;
    sead::Vector2f spineLookYawRange = {-50.0f, 50.0f};
    sead::Vector2f spineLookPitchRange = {-30.0f, 30.0f};
    // The spine parameters are read in their own scope (their iterators share stack slots).
    {
        iter.tryGetStringByKey(&spineJointName, "SpineJointName");

        al::ByamlIter spineJointFrontIter;
        if (iter.tryGetIterByKey(&spineJointFrontIter, "SpineJointFront")) {
            spineJointFrontIter.tryGetFloatByKey(&spineJointFront.x, "X");
            spineJointFrontIter.tryGetFloatByKey(&spineJointFront.y, "Y");
            spineJointFrontIter.tryGetFloatByKey(&spineJointFront.z, "Z");
        }

        al::ByamlIter spineJointUpIter;
        if (iter.tryGetIterByKey(&spineJointUpIter, "SpineJointUp")) {
            spineJointUpIter.tryGetFloatByKey(&spineJointUp.x, "X");
            spineJointUpIter.tryGetFloatByKey(&spineJointUp.y, "Y");
            spineJointUpIter.tryGetFloatByKey(&spineJointUp.z, "Z");
        }

        iter.tryGetFloatByKey(&spineLookAtRate, "SpineLookAtRate");

        al::ByamlIter spineLookYawRangeIter;
        if (iter.tryGetIterByKey(&spineLookYawRangeIter, "SpineLookYawRange")) {
            spineLookYawRangeIter.tryGetFloatByKey(&spineLookYawRange.x, "X");
            spineLookYawRangeIter.tryGetFloatByKey(&spineLookYawRange.y, "Y");
        }

        al::ByamlIter spineLookPitchRangeIter;
        if (iter.tryGetIterByKey(&spineLookPitchRangeIter, "SpineLookPitchRange")) {
            spineLookPitchRangeIter.tryGetFloatByKey(&spineLookPitchRange.x, "X");
            spineLookPitchRangeIter.tryGetFloatByKey(&spineLookPitchRange.y, "Y");
        }
    }

    s32 minLookAtFrame = 60;
    iter.tryGetIntByKey(&minLookAtFrame, "MinLookAtFrame");

    sead::Vector3f lookAtOffset = {0.0f, 0.0f, 0.0f};
    {
        al::ByamlIter lookAtOffsetIter;
        if (iter.tryGetIterByKey(&lookAtOffsetIter, "LookAtOffset")) {
            lookAtOffsetIter.tryGetFloatByKey(&lookAtOffset.x, "X");
            lookAtOffsetIter.tryGetFloatByKey(&lookAtOffset.y, "Y");
            lookAtOffsetIter.tryGetFloatByKey(&lookAtOffset.z, "Z");
        }
    }

    al::ByamlIter actionList;
    if (!iter.tryGetIterByKey(&actionList, "ActionList")) {
        return nullptr;
    }

    s32 actionNum = actionList.getSize();
    auto* headController = new NpcHeadController(
        pActor,
        new NpcHeadControllerParam(jointName, lookAtRate, lookYawRange, lookPitchRange,
                                   jointFront, jointUp),
        new NpcHeadControllerParam(spineJointName, spineLookAtRate, spineLookYawRange,
                                   spineLookPitchRange, spineJointFront, spineJointUp),
        actionNum, pTargetFinder, targetType, minLookAtFrame, lookAtOffset);

    for (s32 i = 0; i < actionNum; i++) {
        al::ByamlIter action;
        if (!actionList.tryGetIterByIndex(&action, i)) {
            continue;
        }

        const char* actionName = nullptr;
        if (!action.tryGetStringByKey(&actionName, "ActionName")) {
            continue;
        }

        bool isControlHead = true;
        bool isControlSpine = true;
        action.tryGetBoolByKey(&isControlHead, "ControlHead");
        action.tryGetBoolByKey(&isControlSpine, "ControlSpine");
        headController->addAction(actionName, isControlHead, isControlSpine);
    }

    return headController;
}

/**
 * @brief Match the speed of the run and walk animations to the horizontal speed of the cat.
 * @param pActor The cat mode actor.
 */
void setAnimationRate(IUseNekoModeActor* pActor) {
    al::isSklAnimPlaying(pActor, 0);
    if (!al::isSklAnimPlaying(pActor, 0)) {
        return;
    }

    if (al::isActionPlaying(pActor, "Run")) {
        f32 rate = al::calcSpeedH(pActor) * 0.11f;
        al::setActionFrameRate(pActor, sead::Mathf::clamp(rate, 0.7f, 1.3f));
        return;
    }

    if (al::isActionPlaying(pActor, "Walk")) {
        f32 rate = al::calcSpeedH(pActor) * 0.4f;
        al::setActionFrameRate(pActor, sead::Mathf::clamp(rate, 0.5f, 2.0f));
        return;
    }

    al::setActionFrameRate(pActor, 1.0f);
}

/**
 * @brief Check whether a cat should keep acting: while airborne, or while a player is near.
 * @param pActor The cat mode actor.
 * @param range Distance a player has to be within.
 * @return Whether the cat is active.
 */
bool isActive(const IUseNekoModeActor* pActor, f32 range) {
    if (!al::isOnGround(pActor, 0, 0.0f)) {
        return true;
    }

    return al::isNearPlayer(pActor, range);
}

/**
 * @brief Check whether an actor is within the chase range of a cat.
 * @param pActor The cat mode actor.
 * @param pTarget The actor to check.
 * @return Whether the target is in range.
 */
bool isInChaseRange(const IUseNekoModeActor* pActor, const al::LiveActor* pTarget) {
    return isInRange(pActor, al::getTrans(pTarget), pActor->getChaseRange());
}

/**
 * @brief Check whether a position is within the chase range of a cat.
 * @param pActor The cat mode actor.
 * @param rPos The position to check.
 * @return Whether the position is in range.
 */
bool isInChaseRange(const IUseNekoModeActor* pActor, const sead::Vector3f& rPos) {
    return isInRange(pActor, rPos, pActor->getChaseRange());
}

/**
 * @brief Check whether a position is within a horizontal range of an actor.
 * @param pActor The actor.
 * @param rCenter The position to check.
 * @param range The range; a negative range is infinite, a zero range contains nothing.
 * @return Whether the position is in range.
 */
bool isInRange(const al::LiveActor* pActor, const sead::Vector3f& rCenter, f32 range) {
    if (range < 0.0f) {
        return true;
    }

    if (range == 0.0f) {
        return false;
    }

    return al::calcDistanceH(pActor, rCenter) <= range;
}

/**
 * @brief Set a nerve unless it is already the current one.
 * @param pUser The nerve user.
 * @param pNerve The nerve to set.
 * @return Whether the nerve was changed.
 */
bool trySetNerve(al::IUseNerve* pUser, const al::Nerve* pNerve) {
    if (al::isNerve(pUser, pNerve)) {
        return false;
    }

    al::setNerve(pUser, pNerve);
    return true;
}

/**
 * @brief Construct a target from its placement.
 * @param rInfo Placement info of the target.
 * @param parentId Id of the cat parent the target belongs to.
 * @param index Index of the target.
 * @param pHost Actor the target follows, or nullptr.
 */
Target::Target(const al::PlacementInfo& rInfo, s32 parentId, s32 index,
               const al::LiveActor* pHost) {
    mParentId = parentId;
    mIndex = index;
    mHost = pHost;
    al::tryGetTrans(&mTrans, rInfo);

    s32 behavior;
    _28 = al::tryGetArg(&behavior, rInfo, "TargetBehavior") ? behavior : 7;
}

/**
 * @brief Get the position of the actor the target follows.
 * @return The position of the host, or the zero vector if there is none.
 */
const sead::Vector3f* Target::tryGetHostTrans() const {
    if (mHost == nullptr) {
        return &sead::Vector3f::zero;
    }

    return &al::getTrans(mHost);
}

/**
 * @brief Calculate a unique id of a placed cat from its zone and placement id ("objNNN").
 * @param holder Placement holder of the cat.
 * @return The zone number in the upper bits, the placement id number in the lower bits.
 */
s32 calcUID(al::PlacementHolder holder) {
    s32 zoneNo = holder.getZoneNo();
    al::StringTmp<32> id("%s", holder.getId());
    return atoi(id.getPart(3).cstr()) | zoneNo << 16;
}

/**
 * @brief Multiply the radius and offset of every hit sensor of an actor.
 * @param pActor The actor.
 * @param scale The scale to apply.
 */
void scaleHitSensors(al::LiveActor* pActor, f32 scale) {
    for (s32 i = 0; i < al::getHitSensorNum(pActor); i++) {
        al::setSensorRadius(pActor, i, al::getSensorRadius(pActor, i) * scale);
        sead::Vector3f offset = al::getSensorFollowPosOffset(pActor, i) * scale;
        al::setSensorFollowPosOffset(pActor, i, offset);
    }
}

/**
 * @brief Check whether a player sensor touched the eye sensor of a cat.
 * @param pSelf Sensor of the cat.
 * @param pOther Sensor that was hit.
 * @return Whether the cat saw the player.
 */
bool attackSensor(const al::HitSensor* pSelf, const al::HitSensor* pOther) {
    if (al::isSensorEye(pOther) && al::isSensorPlayer(pSelf)) {
        return true;
    }

    return false;
}

/**
 * @brief Check whether a sensor belongs to an enemy attack a cat reacts to.
 * @param pSensor The sensor.
 * @return Whether the cat reacts to the sensor.
 */
bool isSensorEnemyReactAttack(const al::HitSensor* pSensor) {
    if (al::isSensorEnemy(pSensor) && al::isSensorHostName(pSensor, "ハンマー")) {
        return true;
    }

    if (al::isSensorEnemyBody(pSensor) && al::isSensorHostName(pSensor, "ブーメラン")) {
        return true;
    }

    if (al::isSensorEnemyAttack(pSensor) &&
        al::isSensorHostName(pSensor, "ファイアブロスファイアボール")) {
        return true;
    }

    if (al::isSensorEnemyBody(pSensor) &&
        al::isSensorHostName(pSensor, "ファイアパックンファイアボール")) {
        return true;
    }

    return al::isSensorEnemyBody(pSensor) && al::isSensorHostName(pSensor, "サーチキラー");
}

/**
 * @brief Check whether a sensor belongs to a map object a cat reacts to.
 * @param pSensor The sensor.
 * @return Whether the cat reacts to the sensor.
 */
bool isSensorMapObjReactAttack(const al::HitSensor* pSensor) {
    if (al::isSensorMapObj(pSensor) && al::isSensorHostName(pSensor, "木箱")) {
        return true;
    }

    if (al::isSensorHostName(pSensor, "SignBoardCat")) {
        return true;
    }

    return al::isSensorHostName(pSensor, "DoorLock");
}

/**
 * @brief Check whether a message is a player or ally attack a cat reacts to.
 * @param pActor The cat mode actor.
 * @param pMsg The message.
 * @param pOther Sensor of the sender.
 * @param pSelf Sensor of the cat.
 * @return Whether the cat reacts to the message.
 */
bool isMsgNpcAttackerHitReaction(const IUseNekoModeActor* pActor, const al::SensorMsg* pMsg,
                                 const al::HitSensor* pOther, const al::HitSensor* pSelf) {
    return al::isMsgPlayerBoomerangAttack(pMsg) || al::isMsgPlayerBoomerangReflect(pMsg) ||
           al::isMsgPlayerFireBallAttack(pMsg) || al::isMsgPlayerSpinAttack(pMsg) ||
           al::isMsgPlayerClimbAttack(pMsg) || al::isMsgPlayerClimbRollingAttack(pMsg) ||
           al::isMsgPlayerClimbSlidingAttack(pMsg) || al::isMsgPlayerTailAttack(pMsg) ||
           al::isMsgNekoAttack(pMsg) || al::isMsgBallAttack(pMsg) || al::isMsgKeyThrow(pMsg);
}

/**
 * @brief Check whether a message is any attack a cat reacts to.
 * @param pActor The cat mode actor.
 * @param pMsg The message.
 * @param pOther Sensor of the sender.
 * @param pSelf Sensor of the cat.
 * @return Whether the cat reacts to the message.
 */
bool isMsgHitReaction(const IUseNekoModeActor* pActor, const al::SensorMsg* pMsg,
                      const al::HitSensor* pOther, const al::HitSensor* pSelf) {
    return isMsgNpcAttackerHitReaction(pActor, pMsg, pOther, pSelf) ||
           rc::isMsgSkateShoesAttack(pMsg) || al::isMsgExplosion(pMsg) ||
           al::isMsgEnemyAttack(pMsg) || al::isMsgEnemyAttackFire(pMsg) ||
           rc::isMsgItemReflect(pMsg) || rc::isMsgPackunEat(pMsg);
}

/**
 * @brief Check whether a message is an attack by the fire trail of a Fire Chain Chomp.
 * @param pActor The cat mode actor.
 * @param pMsg The message.
 * @param pOther Sensor of the sender.
 * @param pSelf Sensor of the cat.
 * @return Whether the message is a fire trail attack.
 */
bool isMsgMeraWanwanTrackAttack(const IUseNekoModeActor* pActor, const al::SensorMsg* pMsg,
                                const al::HitSensor* pOther, const al::HitSensor* pSelf) {
    return al::isMsgEnemyAttack(pMsg) && al::isSensorHostName(pOther, "メラワンワン跡");
}

/**
 * @brief Check whether two sensors are within a range of each other.
 * @param radius Range added to the radius of pOther; a negative range is infinite.
 * @param pSelf First sensor.
 * @param pOther Second sensor.
 * @return Whether the sensors are within range.
 */
bool isHitSensorRadius(f32 radius, const al::HitSensor* pSelf, const al::HitSensor* pOther) {
    if (radius >= 0.0f) {
        f32 distance = (al::getSensorPos(pSelf) - al::getSensorPos(pOther)).length();
        if (distance > al::getSensorRadius(pOther) + radius) {
            return false;
        }
    }

    return true;
}

/**
 * @brief Push an actor horizontally away from the sender of a push or touch message.
 * @param pActor The actor to push.
 * @param pMsg The message.
 * @param pOther Sensor of the sender.
 * @param pSelf Sensor of the actor.
 * @param pushSpeed Horizontal speed to reach away from the sender (doubled for strong pushes).
 * @param radius Range the sensors have to be within (see isHitSensorRadius).
 * @param isCheckFall Whether to refuse pushes that would make the actor fall.
 * @param isCheckAvoidArea Whether to refuse pushes into NPC avoid areas.
 * @return Whether the actor was pushed.
 */
bool tryReceiveMsgPushAndAddVelocityH(al::LiveActor* pActor, const al::SensorMsg* pMsg,
                                      const al::HitSensor* pOther, const al::HitSensor* pSelf,
                                      f32 pushSpeed, f32 radius, bool isCheckFall,
                                      bool isCheckAvoidArea) {
    if (!(al::isMsgPush(pMsg) || al::isMsgPushStrong(pMsg) || al::isMsgPushVeryStrong(pMsg) ||
          al::isMsgNpcTouch(pMsg) || rc::isMsgImozoTouch(pMsg))) {
        return false;
    }

    if (!isHitSensorRadius(radius, pSelf, pOther)) {
        return false;
    }

    if (al::isMsgPushStrong(pMsg) || al::isMsgPushVeryStrong(pMsg)) {
        pushSpeed *= 2.0f;
    }

    sead::Vector3f dir = al::getSensorPos(pSelf) - al::getSensorPos(pOther);
    dir.y = 0.0f;
    al::normalizeOrDirZ(&dir);

    f32 addSpeed = pushSpeed - al::getVelocity(pActor).dot(dir);
    sead::Vector3f addVelocity = dir * addSpeed;
    if (addSpeed > 0.0f) {
        if (isCheckFall && NpcStateFunction::isFallNextMove(pActor, al::getTrans(pActor),
                                                            addVelocity, al::getGravity(pActor),
                                                            100.0f, 150.0f, 150.0f, false)) {
            return false;
        }

        if (isCheckAvoidArea &&
            NpcStateFunction::isNPCAvoidAreaNextMove(pActor, al::getTrans(pActor), addVelocity,
                                                     al::getGravity(pActor), 150.0f, 20.0f,
                                                     100.0f, false)) {
            return false;
        }

        *al::getVelocityPtr(pActor) += addVelocity;
        return true;
    }

    return false;
}

/**
 * @brief Check whether an actor stands in a puddle.
 * @param pActor The actor.
 * @return Whether the actor is on ground with the "Puddle" material.
 */
bool isInPuddle(const al::LiveActor* pActor) {
    if (!al::isOnGround(pActor, 0, 0.0f)) {
        return false;
    }

    return al::isMaterialCode("Puddle", al::getActorCollider(pActor)->mFloor.mTriangle);
}

/**
 * @brief Check whether an actor is moving.
 * @param pActor The actor, may be nullptr.
 * @return Whether the actor exists and moves faster than 1.
 */
bool isInMotion(const al::LiveActor* pActor) {
    return pActor != nullptr && al::isVelocityFast(pActor, 1.0f);
}

/**
 * @brief Check whether an actor is on ground or has ground right below it.
 * @param pActor The actor.
 * @return Whether there is ground under the actor.
 */
bool checkGround(const al::LiveActor* pActor) {
    sead::Vector3f gravity = al::getGravity(pActor);
    if (al::isNearZero(gravity, 0.001f)) {
        return false;
    }

    if (al::isOnGround(pActor, 0, 0.0f)) {
        return true;
    }

    sead::Vector3f start = al::getTrans(pActor) - gravity * 25.0f;
    sead::Vector3f arrow = gravity * 60.0f;
    return alCollisionUtil::getFirstPolyOnArrow(pActor, nullptr, start, arrow, nullptr, nullptr);
}

/**
 * @brief Update an on-ground flag of an actor.
 * @param pActor The actor.
 * @param rIsOnGround The flag to set; never on ground while collision is disabled.
 */
void setOnGroundFlag(const al::LiveActor* pActor, bool& rIsOnGround) {
    if (al::isNoCollide(pActor)) {
        rIsOnGround = false;
        return;
    }

    rIsOnGround = checkGround(pActor);
}

}  // namespace neko
