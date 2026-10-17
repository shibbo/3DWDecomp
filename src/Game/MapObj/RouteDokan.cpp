#include "MapObj/RouteDokan.hpp"
#include "Layout/GuideBalloon.hpp"
#include "Layout/LayoutFontUtil.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Camera/CameraTicket.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/LiveActor/SubActorUtil.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorMovementUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Play/Camera/CameraPoserCart.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Thread/Functor.hpp"
#include "MapObj/RouteDokanBazookaRider.hpp"
#include "MapObj/RouteDokanEntrance.hpp"
#include "MapObj/RouteDokanEntranceGroup.hpp"
#include "MapObj/RouteDokanRider.hpp"
#include "Player/Normal/PlayerKeyConfig.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Block/BlockRailPartsGroup.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/DemoUtil.hpp"
#include "Util/PlayerPuppetUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include <cfloat>
#include <container/seadObjArray.h>
#include <math/seadMathCalcCommon.h>

namespace {
    NERVE_DECL(RouteDokanBazooka, Wait);
    NERVE_DECL(RouteDokanBazooka, Reset);
    NERVE_DECL(RouteDokanBazooka, Appear);
    NERVE_DECL(RouteDokanBazooka, Move);
    NERVE_DECL(RouteDokanBazooka, Ready);
    NERVE_DECL(RouteDokanBazooka, Turn);
    NERVE_DECL(RouteDokanBazooka, TryAllBind);
    NERVE_DECL(RouteDokanBazooka, WaitForShoot);
    NERVE_DECL(RouteDokanBazooka, Shoot);
    NERVE_DECL(RouteDokanBazooka, ShootEnd);
    NERVES_MAKE_NOSTRUCT(RouteDokanBazooka, Wait, Reset, Appear, Move, Ready, Turn, TryAllBind,
                         WaitForShoot, Shoot, ShootEnd)

    /**
     * @brief Scales a vector to the given length, leaving a zero vector untouched.
     * @param pVec The vector to scale.
     * @param length The length the vector should have afterwards.
     */
    inline void setLength(sead::Vector3f* pVec, f32 length) {
        f32 currentLength = pVec->length();
        if (currentLength > 0.0f) {
            f32 scale = length / currentLength;
            pVec->x *= scale;
            pVec->y *= scale;
            pVec->z *= scale;
        }
    }

    /**
     * @brief Calculates the launch velocity of a parabola from one position to another.
     * @param pVelocity The resulting launch velocity.
     * @param rFrom The launch position.
     * @param rTo The landing position.
     * @param angleDegree The launch angle above the horizon, in degrees.
     * @param gravity The gravity applied to the launched object every frame.
     * @return The number of frames the flight takes.
     */
    inline f32 calcParabolaVelocity(sead::Vector3f* pVelocity, const sead::Vector3f& rFrom,
                                    const sead::Vector3f& rTo, f32 angleDegree, f32 gravity) {
        sead::Vector3f diff = rTo - rFrom;
        f32 distance = sead::Mathf::sqrt(diff.x * diff.x + diff.z * diff.z);
        f32 tanAngle = sead::Mathf::tan(sead::Mathf::deg2rad(angleDegree));
        f32 speed =
            sead::Mathf::sqrt((gravity * distance) / ((tanAngle - diff.y / distance) * 2.0f));

        f32 speedY = tanAngle * speed;
        pVelocity->set(diff.x, 0.0f, diff.z);
        setLength(pVelocity, speed);
        pVelocity->y = speedY;
        return distance / speed;
    }
}

/**
 * @brief Constructs a route pipe network.
 * @param pName The name of the actor.
 */
RouteDokan::RouteDokan(const char* pName) : al::LiveActor(pName) {}

/**
 * @brief Initializes the pipe parts, the entrances and the riders that carry players.
 * @param rInfo The actor init info.
 */
void RouteDokan::init(const al::ActorInitInfo& rInfo) {
    al::initActor(this, rInfo);
    mPartsGroup = new al::BlockRailPartsGroup();
    mPartsGroup->init(rInfo);
    if (mPartsGroup->getPartsNum() == 0) {
        makeActorDead();
        return;
    }

    mEntranceGroup = new RouteDokanEntranceGroup("ルート土管出入口", "RouteDokanTerminate");
    mEntranceGroup->init(mPartsGroup, rInfo, false);
    mEntranceGroup->setHost(this);

    f32 moveSpeed = 20.0f;
    al::tryGetArg(&moveSpeed, rInfo, "MoveSpeed");
    if (moveSpeed < 1.0f) {
        moveSpeed = 1.0f;
    }

    mRiderNum = 8;
    mRiders = new RouteDokanRider*[mRiderNum];
    for (s32 i = 0; i < mRiderNum; i++) {
        mRiders[i] = new RouteDokanRider("ルート土管ライダー", 1);
        mRiders[i]->setMoveSpeed(moveSpeed);
        al::initCreateActorNoPlacementInfo(mRiders[i], rInfo);
    }

    if (al::listenStageSwitchOnAppear(this, al::FunctorV0M(this, &RouteDokan::active))) {
        al::tryGetArg(&mIsUseSpecialAppear, rInfo, "IsUseSpecialAppear");
        deactive();
    }

    mPartsGroup->calcOffset(al::getTrans(this));
    mEntranceGroup->calcOffset(al::getTrans(this));
    makeActorDead();
}

/**
 * @brief Activates the pipe parts and the entrances.
 */
void RouteDokan::active() {
    mPartsGroup->active();
    mEntranceGroup->active();
}

/**
 * @brief Deactivates the pipe parts and the entrances.
 */
void RouteDokan::deactive() {
    mPartsGroup->deactive();
    mEntranceGroup->deactive();
}

/**
 * @brief Checks whether a player may enter the pipe.
 * @param pSender The sensor of the player trying to enter.
 * @return Whether the player is not already riding the pipe.
 */
bool RouteDokan::isBindStart(al::HitSensor* pSender) {
    s32 playerIndex = alPlayerFunction::findPlayerHolderIndex(pSender);
    for (s32 i = 0; i < mRiderNum; i++) {
        if (mRiders[i]->isActive(playerIndex)) {
            return false;
        }
    }

    return true;
}

/**
 * @brief Hands an entering player to a free rider.
 * @param pEntrance The entrance the player entered through.
 * @param pMsg The message that started the bind.
 * @param pSender The sensor of the entering player.
 * @param pReceiver The sensor of the entrance.
 * @return Whether a rider took the player.
 */
bool RouteDokan::startBind(RouteDokanEntrance* pEntrance, const al::SensorMsg* pMsg,
                           al::HitSensor* pSender, al::HitSensor* pReceiver) {
    for (s32 i = 0; i < mRiderNum; i++) {
        if (al::isDead(mRiders[i]) || mRiders[i]->canBindSingleMode()) {
            mRiders[i]->startBind(pEntrance, pSender, pReceiver);
            al::sendMsgHoldCancel(pSender, pReceiver);
            return true;
        }
    }

    return false;
}

/**
 * @brief Cancels the bind of a player on every rider.
 * @param pSender The sensor of the player.
 * @return Always true.
 */
bool RouteDokan::cancelBind(al::HitSensor* pSender) {
    for (s32 i = 0; i < mRiderNum; i++) {
        mRiders[i]->tryCancelBind(pSender);
    }

    return true;
}

/**
 * @brief Damages the player carried by any active rider.
 * @param pSender The sensor causing the damage.
 * @return Whether any rider took damage.
 */
bool RouteDokan::damagePuppet(al::HitSensor* pSender) {
    bool isDamaged = false;
    for (s32 i = 0; i < mRiderNum; i++) {
        if (!al::isDead(mRiders[i])) {
            isDamaged |= mRiders[i]->damage(pSender);
        }
    }

    return isDamaged;
}

/**
 * @brief Moves the pipe parts and the entrances along with a linked actor.
 * @param rTrans The translation of the linked actor.
 */
void RouteDokan::updateLinkedTrans(const sead::Vector3f& rTrans) {
    mPartsGroup->updateLinkedTrans(rTrans);
    mEntranceGroup->updateLinkedTrans(rTrans);
}

/**
 * @brief Constructs a route pipe cannon.
 * @param pName The name of the actor.
 */
RouteDokanBazooka::RouteDokanBazooka(const char* pName) : al::LiveActor(pName) {}

/**
 * @brief Initializes the model, the pipe network, the riders and their launch velocities.
 * @param rInfo The actor init info.
 */
void RouteDokanBazooka::init(const al::ActorInitInfo& rInfo) {
    mIsSingleMode = rInfo.getActorSceneInfo().isSingleMode;

    const char* suffix = nullptr;
    al::tryGetStringArg(&suffix, rInfo, "ModelSuffix");
    if (mIsSingleMode) {
        if (suffix == nullptr) {
            suffix = "SM";
        } else if (al::isEqualString(suffix, "NoDepthShadow")) {
            suffix = "NoDepthShadowSM";
        }
    }

    if (suffix != nullptr) {
        al::initActorSuffix(this, rInfo, suffix);
    } else {
        al::initActor(this, rInfo);
    }

    mIsInvalidClipping = al::isInvalidClipping(this);
    al::initNerve(this, &NrvRouteDokanBazookaWait, 0);

    sead::Vector3f upDir;
    al::calcUpDir(&upDir, this);

    mPartsGroup = new al::BlockRailPartsGroup();
    mPartsGroup->init(rInfo);
    if (mPartsGroup->getPartsNum() == 0) {
        makeActorDead();
        return;
    }

    mEntranceGroup =
        new RouteDokanEntranceGroup("ルート土管大砲出入口", "RouteDokanBazookaEntrance");
    mEntranceGroup->init(mPartsGroup, rInfo, false);
    mEntranceGroup->setHost(this);

    mIsValidTarget = al::tryGetLinksTrans(&mTargetTrans, rInfo, "TargetPos");
    if (mIsValidTarget && al::isNearZero(al::getTrans(this) - mTargetTrans, 0.001f)) {
        if (al::isNearZero(al::getTrans(this) - mTargetTrans, 0.001f)) {
            mTargetTrans = al::getTrans(this) + sead::Vector3f::ez * 100.0f;
        }
    }

    f32 shootAngle = 0.0f;
    if (al::tryGetArg(&shootAngle, rInfo, "ShootAngle")) {
        if (shootAngle > 0.0f && shootAngle < 90.0f) {
            mShootAngle = shootAngle;
        }
    }

    al::tryGetArg(&mGravity, rInfo, "Gravity");

    s32 shootType;
    al::tryGetArg(&shootType, rInfo, "ShootType");
    mShootType = shootType;

    f32 shootSpeed = 30.0f;
    al::tryGetArg(&shootSpeed, rInfo, "ShootSpeed");

    f32 moveSpeed = 30.0f;
    al::tryGetArg(&moveSpeed, rInfo, "MoveSpeed");
    if (moveSpeed < 1.0f) {
        moveSpeed = 1.0f;
    }

    mStartEntrance = nullptr;
    s32 entranceCount = mEntranceGroup->getEntranceCount();
    f32 closestDistance = FLT_MAX;
    for (s32 i = 0; i < entranceCount; i++) {
        RouteDokanEntrance* entrance = mEntranceGroup->getEntrance(i);
        f32 distance = (al::getTrans(this) - al::getTrans(entrance)).length();
        if (distance < closestDistance) {
            closestDistance = distance;
            mStartEntrance = entrance;
        }
    }

    RouteDokanEntrance* exitEntrance = nullptr;
    for (s32 i = 0; i < entranceCount; i++) {
        RouteDokanEntrance* entrance = mEntranceGroup->getEntrance(i);
        if (mStartEntrance == entrance) {
            continue;
        }

        al::setTrans(this, al::getTrans(entrance));
        al::setQuat(this, al::getQuat(entrance));
        al::addTransOffsetLocalDir(this, 155.0f, 2);
        exitEntrance = entrance;
    }

    al::copyPose(al::getSubActor(this, "大砲の根元"), exitEntrance);
    al::copyPose(al::getSubActor(al::getSubActor(this, "大砲の根元"), "外側モデル"), exitEntrance);
    exitEntrance->kill();

    sead::Vector3f riderUpDir = upDir;
    sead::Vector3f velocities[8];
    if (mIsValidTarget) {
        if (mShootType == ShootType_Parabola) {
            calcParabolaVelocity(&velocities[0], al::getTrans(this), mTargetTrans, mShootAngle,
                                 mGravity);
        } else if (mShootType == ShootType_Straight) {
            for (s32 i = 0; i < 8; i++) {
                velocities[i] = mTargetTrans - al::getTrans(this);
                setLength(&velocities[i], shootSpeed);
            }
        }

        sead::Vector3f toTarget = mTargetTrans - al::getTrans(this);
        if (!al::isParallelDirection(toTarget, velocities[0], 0.01f)) {
            sead::Vector3f side;
            side.setCross(toTarget.cross(velocities[0]), velocities[0]);
            if (!al::normalizeOrZero(&side)) {
                riderUpDir = side;
            }
        }

        sead::Vector3f front = velocities[0];
        al::normalizeOrZero(&front);
        al::makeQuatFrontUp(&mTargetQuat, front, sead::Vector3f::ey);
    } else {
        al::calcQuat(&mTargetQuat, this);
        for (s32 i = 0; i < 8; i++) {
            al::calcQuatFront(&velocities[i], al::getQuat(this));
            setLength(&velocities[i], shootSpeed);
        }
    }

    for (s32 i = 0; i < mRiders.capacity(); i++) {
        auto* rider = new RouteDokanBazookaRider(this, "ルート土管大砲ライダー", 1, mGravity);
        rider->setMoveSpeed(moveSpeed);
        rider->setOutVelocity(velocities[i]);
        rider->setShootType(mShootType);
        rider->setUpDir(riderUpDir);
        al::initCreateActorNoPlacementInfo(rider, rInfo);
        mRiders.pushBack(rider);
    }

    bool isUseObjectCamera = false;
    al::tryGetArg(&isUseObjectCamera, rInfo, "IsUseObjectCamera");
    if (isUseObjectCamera && !mIsSingleMode) {
        mCameraInfo = al::initObjectCamera(this, rInfo, nullptr);
    } else if (mIsSingleMode) {
        mCameraTicket = al::initObjectCamera_RS(this, rInfo, nullptr);

        bool isUseLandingAngle = false;
        al::tryGetArg(&isUseLandingAngle, rInfo, "IsUseLandingAngle");
        if (isUseLandingAngle) {
            auto* poser = mCameraTicket->getPoser<al::CameraPoserCart>();
            f32 landingAngle = 0.0f;
            s32 stepsToLandingAngle = 0;
            al::tryGetArg(&landingAngle, rInfo, "LandingAngle");
            al::tryGetArg(&stepsToLandingAngle, rInfo, "StepsToLandingAngle");
            poser->setUseDestinationAngle(landingAngle, stepsToLandingAngle);
        }
    }

    mGuideBalloon = new GuideBalloon("ガイドバルーン[発射]", al::getLayoutInitInfo(rInfo),
                                     al::getTransPtr(this), sead::Vector3f(0.0f, 200.0f, 0.0f),
                                     false, nullptr);
    al::tryGetArg(&mIsDisplayGuideIcon, rInfo, "IsDisplayGuideIcon");
    makeActorAppeared();

    s32 islandId = -1;
    al::tryGetArg(&islandId, rInfo, "IslandID");
    if (islandId > 0 &&
        SingleModeDataFunction::isIslandUnlocked(GameDataHolderAccessor(this), islandId)) {
        return;
    }

    if (mIsSingleMode) {
        sead::Vector3f rotate;
        if (al::tryGetZoneR(&rotate, rInfo.getPlacementInfo()) &&
            al::isNear(upDir.y, 1.0f, 0.001f)) {
            al::rotateQuatRadian(al::getQuatPtr(this), *al::getQuatPtr(this), sead::Vector3f::ey,
                                 sead::Mathf::deg2rad(rotate.y));
        }
    }

    al::tryGetArg(&mIsUseDelayedCamera, rInfo, "IsUseDelayedCamera");
    if (al::listenStageSwitchOnAppear(this, al::FunctorV0M(this, &RouteDokanBazooka::active))) {
        al::tryGetArg(&mIsUseSpecialAppear, rInfo, "IsUseSpecialAppear");
        al::tryGetArg(&mWaitFrameCount, rInfo, "WaitFrameCount");
        deactive();
    }
}

/**
 * @brief Makes the cannon appear and activates the pipe parts and the entrances.
 */
void RouteDokanBazooka::active() {
    makeActorAppeared();
    if (mIsUseSpecialAppear) {
        al::startSe(this, "SpecialAppearLast");
        mPartsGroup->active();
        mEntranceGroup->active();
    } else {
        mPartsGroup->active();
        mEntranceGroup->active();
    }
}

/**
 * @brief Hides the cannon and deactivates the pipe parts and the entrances.
 */
void RouteDokanBazooka::deactive() {
    makeActorDead();
    mPartsGroup->deactive();
    mEntranceGroup->deactive();
}

/**
 * @brief Resets the collision matrices after placement in single mode.
 */
void RouteDokanBazooka::initAfterPlacement() {
    if (mIsSingleMode) {
        al::resetAllCollisionMtx(this);
    }
}

/**
 * @brief Checks whether a player may enter the cannon.
 * @param pSender The sensor of the player trying to enter.
 * @return Whether the cannon is ready and the player is not already riding it.
 */
bool RouteDokanBazooka::isBindStart(al::HitSensor* pSender) {
    if (al::isNerve(this, &NrvRouteDokanBazookaReset)) {
        return false;
    }

    if (al::isNerve(this, &NrvRouteDokanBazookaWait) && al::isLessStep(this, 20)) {
        return false;
    }

    if (al::isNerve(this, &NrvRouteDokanBazookaAppear)) {
        return false;
    }

    s32 playerIndex = alPlayerFunction::findPlayerHolderIndex(pSender);
    for (auto it = mRiders.begin(); it != mRiders.end(); ++it) {
        if (it->isActive(playerIndex)) {
            return false;
        }
    }

    return true;
}

/**
 * @brief Hands an entering player to a free rider.
 * @param pEntrance The entrance the player entered through.
 * @param pMsg The message that started the bind.
 * @param pSender The sensor of the entering player.
 * @param pReceiver The sensor of the entrance.
 * @return Whether a rider took the player.
 */
bool RouteDokanBazooka::startBind(RouteDokanEntrance* pEntrance, const al::SensorMsg* pMsg,
                                  al::HitSensor* pSender, al::HitSensor* pReceiver) {
    bool isFlying =
        al::isEqualString(al::getActionName(al::getSensorHost(pSender)), "RouteDokanBazookaFly");
    for (auto it = mRiders.begin(); it != mRiders.end(); ++it) {
        if (al::isDead(&*it)) {
            it->startBind(pEntrance, pMsg, pSender, pReceiver, isFlying);
            al::sendMsgHoldCancel(pSender, pReceiver);
            mBindRiders.pushBack(&*it);
            return true;
        }
    }

    return false;
}

/**
 * @brief Cancels the bind of a player and resets the cannon once nobody is left inside.
 * @param pSender The sensor of the player.
 * @return Always true.
 */
bool RouteDokanBazooka::cancelBind(al::HitSensor* pSender) {
    rc::cancelRequestBindAndResetDisableReviveBubbleForAllPlayer(
        mStartEntrance, al::getHitSensor(mStartEntrance, "Bind"));

    bool isAnyRiderLeft = false;
    for (auto it = mBindRiders.begin(); it != mBindRiders.end(); ++it) {
        isAnyRiderLeft |= !it->tryCancelBind(pSender);
    }

    if (!isAnyRiderLeft) {
        if (mGuideBalloon->isAlive()) {
            mGuideBalloon->endShow();
        }

        al::setNerve(this, &NrvRouteDokanBazookaReset);
    }

    return true;
}

/**
 * @brief Damages the players carried by the bound riders.
 * @param pSender The sensor causing the damage.
 * @return Whether any rider took damage.
 */
bool RouteDokanBazooka::damagePuppet(al::HitSensor* pSender) {
    bool isDamaged = false;
    for (auto it = mBindRiders.begin(); it != mBindRiders.end(); ++it) {
        isDamaged |= it->damage(pSender);
    }

    return isDamaged;
}

/**
 * @brief Checks whether every alive player sits ready inside the cannon.
 * @return Whether all players are ready to be launched.
 */
bool RouteDokanBazooka::isReadyAll() const {
    if (mBindRiders.size() == 0 || mBindRiders.size() != al::getAlivePlayerNum(this)) {
        return false;
    }

    for (auto it = mBindRiders.begin(); it != mBindRiders.end(); ++it) {
        if (!it->isStateReady()) {
            return false;
        }
    }

    return true;
}

/**
 * @brief Checks whether the launch guide balloon should be shown.
 * @return Whether the guide icon is enabled or several players are inside.
 */
bool RouteDokanBazooka::isDisplayGuideBalloon() const {
    return mIsDisplayGuideIcon || mBindRiders.size() > 1;
}

/**
 * @brief Removes a bound rider that died since it entered.
 * @return Whether a rider was removed.
 */
bool RouteDokanBazooka::tryAdjustActiveRiders() {
    sead::FixedObjArray<s32, 8> deadIndices;
    for (s32 i = 0; i < mBindRiders.size(); i++) {
        if (al::isDead(mBindRiders[i]) && !deadIndices.isFull()) {
            deadIndices.insert(0, i);
        }
    }

    if (deadIndices.size() != 0) {
        mBindRiders.erase(*deadIndices[0]);
        return true;
    }

    return false;
}

/**
 * @brief Makes the bound players invincible and stops their damage flashing.
 */
void RouteDokanBazooka::invalidatePlayerDamage() {
    for (auto it = mBindRiders.begin(); it != mBindRiders.end(); ++it) {
        rc::invalidatePuppetDamage(it->getPuppet(), 2);
        rc::invalidatePuppetFlash(it->getPuppet());
    }
}

/**
 * @brief Lets the bound players flash on damage again.
 */
void RouteDokanBazooka::validatePlayerDamage() {
    for (auto it = mBindRiders.begin(); it != mBindRiders.end(); ++it) {
        rc::validatePuppetFlash(it->getPuppet());
    }
}

/**
 * @brief Builds the pipe network part by part, then shows the cannon.
 */
void RouteDokanBazooka::exeAppear() {
    if (al::isFirstStep(this)) {
        mAppearIndex = 0;
        mEntranceGroup->getEntrance(0)->appear();
    }

    if (al::isIntervalStep(this, mWaitFrameCount, 0)) {
        if (mAppearIndex < mPartsGroup->getPartsNum()) {
            mPartsGroup->specialActive(mAppearIndex);
            al::setSeSeqLocalVariableDefault(this, 0, mAppearIndex);
            al::startSe(this, "SpecialAppear");
            mAppearIndex++;
        } else {
            al::showModel(this);
            al::showModel(al::getSubActor(this, "大砲の根元"));
            mEntranceGroup->getEntrance(mEntranceGroup->getEntranceCount() - 1)->appear();
            al::setSeSeqLocalVariableDefault(this, 0, 0);
            al::startSe(this, "SpecialAppearLast");
            al::setNerve(this, &NrvRouteDokanBazookaWait);
        }
    }
}

/**
 * @brief Waits for a player to enter the cannon.
 */
void RouteDokanBazooka::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait");
        al::startAction(al::getSubActor(this, "外側モデル"), "Wait");
    }

    if (mBindRiders.size() != 0) {
        al::setNerve(this, &NrvRouteDokanBazookaMove);
    }
}

/**
 * @brief Waits for a bound player to reach the cannon.
 */
void RouteDokanBazooka::exeMove() {
    if (al::isFirstStep(this)) {
        al::invalidateClipping(this);
    }

    for (auto it = mBindRiders.begin(); it != mBindRiders.end(); ++it) {
        if (it->isStateReady()) {
            al::setNerve(this, &NrvRouteDokanBazookaReady);
            return;
        }
    }
}

/**
 * @brief Waits for every player to be ready, binding the missing ones after a while.
 */
void RouteDokanBazooka::exeReady() {
    tryAdjustActiveRiders();
    if (isReadyAll()) {
        al::setNerve(this, &NrvRouteDokanBazookaTurn);
        return;
    }

    if (mBindRiders.size() != al::getAlivePlayerNum(this) && al::isGreaterStep(this, 60)) {
        al::setNerve(this, &NrvRouteDokanBazookaTryAllBind);
    }
}

/**
 * @brief Pulls every player into the cannon.
 */
void RouteDokanBazooka::exeTryAllBind() {
    rc::requestBindAllPlayer(mStartEntrance, al::getHitSensor(mStartEntrance, "Bind"));
    tryAdjustActiveRiders();
    if (isReadyAll()) {
        al::startAction(this, "In");
        al::startAction(al::getSubActor(this, "外側モデル"), "In");
        al::setNerve(this, &NrvRouteDokanBazookaTurn);
    }
}

/**
 * @brief Starts the camera and turns the cannon towards the target.
 */
void RouteDokanBazooka::exeTurn() {
    rc::requestBindAllPlayer(mStartEntrance, al::getHitSensor(mStartEntrance, "Bind"));
    if (al::isFirstStep(this)) {
        if (mCameraInfo != nullptr) {
            al::startCamera(this, mCameraInfo, -1);
        } else if (mCameraTicket != nullptr && !mIsUseDelayedCamera) {
            if (!rc::requestStartDemoCamera(this, nullptr)) {
                al::setNerve(this, &NrvRouteDokanBazookaTurn);
                return;
            }

            al::startCamera_RS(this, mCameraTicket, -1);
        }

        al::tryOnStageSwitch(this, "StageFlyingOnOff");
        al::calcQuat(&mInitQuat, this);
    }

    if (mIsValidTarget) {
        f32 rate = al::calcNerveEaseInOutRate(this, 12);
        al::slerpQuat(al::getQuatPtr(this), mInitQuat, mTargetQuat, rate);
    }

    if (al::isGreaterEqualStep(this, 12)) {
        al::setNerve(this, &NrvRouteDokanBazookaWaitForShoot);
    }
}

/**
 * @brief Shows the launch guide and waits for a player to press the decide button.
 */
void RouteDokanBazooka::exeWaitForShoot() {
    rc::requestBindAllPlayer(mStartEntrance, al::getHitSensor(mStartEntrance, "Bind"));
    if (al::isGreaterEqualStep(this, 18) && !mGuideBalloon->isAlive() &&
        isDisplayGuideBalloon()) {
        s32 port = rc::getPlayerInputPort(this, mRiders[0]->getPlayerIndex());
        mGuideBalloon->startShow(LayoutFontUtil::getMessageFontButtonDecide(port));
    }

    for (auto it = mBindRiders.begin(); it != mBindRiders.end(); ++it) {
        if (it->getPuppet() == nullptr) {
            continue;
        }

        if (!PlayerKeyConfig::isPadTriggerDecide(
                rc::getPlayerInputPort(this, it->getPlayerIndex()), true)) {
            continue;
        }

        if (mIsSingleMode) {
            invalidatePlayerDamage();
        }

        rc::setDemoAudioType(this, static_cast<alSeFunction::DemoType>(4));
        if (mIsUseDelayedCamera && mCameraTicket != nullptr) {
            if (!rc::requestStartDemoCamera(this, nullptr)) {
                continue;
            }

            al::startCamera_RS(this, mCameraTicket, -1);
        }

        if (mGuideBalloon->isAlive()) {
            mGuideBalloon->endShow();
        }

        al::setNerve(this, &NrvRouteDokanBazookaShoot);
        return;
    }
}

/**
 * @brief Launches the riders one after another and waits until they have landed.
 */
void RouteDokanBazooka::exeShoot() {
    al::requestResetUserCameraControl(this);
    if (mIsSingleMode) {
        invalidatePlayerDamage();
    }

    sead::Vector3f target;
    for (auto it = mBindRiders.begin(); it != mBindRiders.end(); ++it) {
        s32 index = mBindRiders.indexOf(&*it);
        if (!al::isStep(this, index * 8)) {
            continue;
        }

        al::startAction(this, "Launch");
        al::startAction(al::getSubActor(this, "外側モデル"), "Launch");
        if (mShootType == ShootType_Parabola) {
            const sead::Vector3f& trans = al::getTrans(this);
            s32 riderNum = mBindRiders.size();
            if (riderNum == 1) {
                target = mTargetTrans;
            } else {
                sead::Vector3f front = mTargetTrans - trans;
                front.y = 0.0f;
                al::normalize(&front);
                sead::Vector3f side = front.cross(sead::Vector3f::ey);
                switch (riderNum) {
                case 2:
                    switch (index) {
                    case 0:
                        target = mTargetTrans - side * 100.0f;
                        break;
                    case 1:
                        target = side * 100.0f + mTargetTrans - front * 100.0f * 0.5f;
                        break;
                    }
                    break;
                case 3:
                    switch (index) {
                    case 0:
                        target = front * 100.0f + mTargetTrans;
                        break;
                    case 1:
                        target = mTargetTrans - side * 100.0f;
                        break;
                    case 2:
                        target = side * 100.0f + mTargetTrans;
                        break;
                    }
                    break;
                case 4:
                    switch (index) {
                    case 0:
                        target = front * 100.0f + mTargetTrans;
                        break;
                    case 1:
                        target = mTargetTrans - side * 100.0f;
                        break;
                    case 2:
                        target = side * 100.0f + mTargetTrans;
                        break;
                    case 3:
                        target = mTargetTrans - front * 100.0f;
                        break;
                    }
                    break;
                }
            }

            sead::Vector3f velocity;
            f32 flightFrames = calcParabolaVelocity(&velocity, al::getTrans(this), target,
                                                    mShootAngle, mGravity);
            it->setOutVelocity(velocity);
            it->setShootFrame(flightFrames - 0.5f);
        }

        it->shoot();
    }

    if (al::isLessStep(this, mBindRiders.size() * 8)) {
        return;
    }

    bool isAnyFlying = false;
    for (auto it = mBindRiders.begin(); it != mBindRiders.end(); ++it) {
        if (it->isStateFlying()) {
            isAnyFlying = true;
            break;
        }
    }

    if (isAnyFlying) {
        for (auto it = mBindRiders.begin(); it != mBindRiders.end(); ++it) {
            if (!al::isDead(&*it)) {
                return;
            }
        }

        al::setNerve(this, &NrvRouteDokanBazookaReset);
        return;
    }

    if (mCameraInfo != nullptr) {
        al::endCamera(this, mCameraInfo, -1);
    } else if (mCameraTicket != nullptr) {
        al::endCamera_RS(this, mCameraTicket, -1, false);
        rc::requestEndDemoCamera(this);
    }

    al::tryOffStageSwitch(this, "StageFlyingOnOff");
    al::setNerve(this, &NrvRouteDokanBazookaShootEnd);
}

/**
 * @brief Waits until every launched player has left their rider.
 */
void RouteDokanBazooka::exeShootEnd() {
    if (al::isFirstStep(this) && mIsSingleMode) {
        validatePlayerDamage();
    }

    for (auto it = mBindRiders.begin(); it != mBindRiders.end(); ++it) {
        if (!it->isEndBind()) {
            return;
        }
    }

    al::setNerve(this, &NrvRouteDokanBazookaReset);
}

/**
 * @brief Turns the cannon back to its initial rotation.
 */
void RouteDokanBazooka::exeReset() {
    if (al::isFirstStep(this)) {
        mBindRiders.clear();
        rc::resetDisableReviveBubbleForAllPlayer(mStartEntrance);
    }

    if (mIsValidTarget) {
        f32 rate = al::calcNerveRate(this, 90);
        al::slerpQuat(al::getQuatPtr(this), mTargetQuat, mInitQuat, rate);
    }

    if (al::isGreaterStep(this, 90)) {
        if (!mIsInvalidClipping) {
            al::validateClipping(this);
        }

        al::setNerve(this, &NrvRouteDokanBazookaWait);
    }
}
