#include "NPC/Rabbit.hpp"

#include <prim/seadEnum.h>

#include "Camera/NpcGoalCameraTarget.hpp"
#include "CourseSelect/BgmBeatAnimeController.hpp"
#include "Enemy/ActorStateSupportFreeze.hpp"
#include "Layout/IslandMap.hpp"
#include "MapObj/Bush.hpp"
#include "MapObj/CoinBlowGenerator.hpp"
#include "MapObj/GoalItem.hpp"
#include "MapObj/GreenStar.hpp"
#include "MapObj/ItemStatePopUpFront.hpp"
#include "MapObj/KinokoBig.hpp"
#include "MapObj/TimerManager.hpp"
#include "NPC/RabbitInitPlacePoint.hpp"
#include "NPC/RabbitRoute.hpp"
#include "NPC/RabbitRouteRider.hpp"
#include "NPC/RabbitStateReverse.hpp"
#include "NPC/RabbitStateSwoon.hpp"
#include "Player/Normal/PlayerActor.hpp"
#include "Scene/SceneObjID.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/IslandDataList.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/DemoUtil.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/Item/ItemUtil.hpp"
#include "Library/Joint/JointControllerKeeper.hpp"
#include "Library/Joint/JointSpringController.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Play/Placement/PlacementHolder.hpp"
#include "Library/Play/Placement/PlacementInfo.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Thread/Functor.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Collision/CollisionUtil.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"

namespace rc {
void appearGuideGameWindow(const al::IUseSceneObjHolder* pHolder, const char* pCategory,
                           const char* pLabel, s32 frame, f32 delay);
void disappearGuideGameWindow(const al::IUseSceneObjHolder* pHolder);
}  // namespace rc

namespace {
NERVE_DECL(Rabbit, Wait)
NERVE_DECL(Rabbit, Reverse)
NERVE_DECL(Rabbit, Swoon)
NERVE_DECL(Rabbit, SupportFreeze)
NERVE_DECL(Rabbit, WaitAtInitPoint)
NERVE_DECL(Rabbit, Hide)
NERVE_DECL(Rabbit, Catch)
NERVE_DECL(Rabbit, Poof)
NERVE_DECL(Rabbit, Jump)
NERVE_DECL(Rabbit, Appear)
NERVE_DECL(Rabbit, AppearWait)
NERVE_DECL(Rabbit, Find)
class RabbitNrvFindFirst : public al::Nerve {
public:
    void execute(al::NerveKeeper* pKeeper) const override {
        pKeeper->getParent<Rabbit>()->exeFind();
    }
};
NERVE_DECL(Rabbit, FindAtInitPoint)
NERVE_DECL(Rabbit, JumpStart)
NERVE_DECL(Rabbit, Run)
NERVE_DECL(Rabbit, RunBreak)
NERVE_DECL(Rabbit, DisappearGoalItem)
NERVE_DECL(Rabbit, Disappear)

NERVES_MAKE_NOSTRUCT(Rabbit, Poof, AppearWait, Find, FindAtInitPoint)
NERVES_MAKE_STRUCT(Rabbit, Wait, Reverse, Swoon, SupportFreeze, WaitAtInitPoint, Hide, Catch, Jump,
                   Appear, FindFirst, JumpStart, Run, RunBreak, DisappearGoalItem, Disappear)

// clang-format off
SEAD_ENUM(RabbitEarJoint, EarL1, EarL2, EarR1, EarR2)
// clang-format on

struct EarSpringParam {
    f32 stability;
    f32 friction;
    f32 limitDegree;
};

const EarSpringParam sEarSpringParams[] = {
    {0.08f, 0.95f, 25.0f},
    {0.08f, 0.95f, 35.0f},
    {0.08f, 0.95f, 25.0f},
    {0.08f, 0.95f, 35.0f},
};

const sead::Vector3f sCoinBlowOffset(-50.0f, 350.0f, 100.0f);
const sead::Vector3f sKinokoBigOffset(-50.0f, 200.0f, 100.0f);
const ActorStateSupportFreezeParam sSupportFreezeParam(true, 15, false, true, 120,
                                                       sead::Vector3f(0.0f, 150.0f, 0.0f));

typedef al::FunctorV0M<Rabbit*, void (Rabbit::*)()> RabbitFunctor;
}  // namespace

/**
 * @brief Checks whether the scenario of the rabbit's shine has been completed.
 * @return Whether the scenario is complete.
 */
inline bool Rabbit::isScenarioComplete() const {
    return SingleModeDataFunction::isScenarioComplete(GameDataHolderAccessor(this), mIslandId - 1,
                                                      mScenarioId - 1);
}

/**
 * @brief Hides the dash guide window and blocks it from reappearing.
 */
inline void Rabbit::hideGuide() {
    rc::disappearGuideGameWindow(this);
    mGuideTimer = 360;
}

/**
 * @brief Knocks the rabbit out.
 * @param pMsg Message that caused the knock-out.
 */
inline void Rabbit::startSwoon(const al::SensorMsg* pMsg) {
    mIsSwoonRequested = true;
    al::startHitReactionHit(this);
    mStateSwoon->setStartActionName(al::isMsgBlockUpperPunch(pMsg) ? "SwoonStartUpperPunch" :
                                                                     "SwoonStart");
    if (!al::isNerve(this, &NrvRabbit.Jump)) {
        al::setNerve(this, &NrvRabbit.Swoon);
    }
}

/**
 * @brief Turns back on the route.
 * @param pNextNerve Nerve to start after reversing.
 */
inline void Rabbit::startReverse(const al::Nerve* pNextNerve) {
    mRouteRider->reverse();
    mReverseCoolTime = 90;
    mNerveAfterReverse = pNextNerve;
    al::setNerve(this, &NrvRabbit.Reverse);
}

/**
 * @brief Turns back on the route without stopping first.
 * @param pNextNerve Nerve to start after reversing.
 */
inline void Rabbit::startReverseNoStop(const al::Nerve* pNextNerve) {
    mRouteRider->reverse();
    mReverseCoolTime = 90;
    mNerveAfterReverse = pNextNerve;
    mStateReverse->setNoStop();
    al::setNerve(this, &NrvRabbit.Reverse);
}

/**
 * @brief Constructs the rabbit.
 * @param pName Actor name.
 */
Rabbit::Rabbit(const char* pName) : al::LiveActor(pName) {}

/**
 * @brief Initializes the model, ear springs, states, route, carried item and stage switches.
 * @param rInfo Actor placement and scene initialization information.
 */
void Rabbit::init(const al::ActorInitInfo& rInfo) {
    mIsSingleMode = rInfo.getActorSceneInfo().isSingleMode;
    mIsBig = al::isObjectName(rInfo, "RabbitBig");
    mJumpStartStep = 10;
    if (mIsSingleMode && mIsBig) {
        al::initActorSuffix(this, rInfo, "Water");
        mJumpStartStep = 2;
    } else {
        al::initActor(this, rInfo);
    }

    al::initJointControllerKeeper(this, 4);
    for (auto it = RabbitEarJoint::begin(); it != RabbitEarJoint::end(); ++it) {
        auto* pController = al::initJointSpringController(this, (*it).text());
        pController->setStability(sEarSpringParams[*it].stability);
        pController->setFriction(sEarSpringParams[*it].friction);
        pController->setLimitDegree(sEarSpringParams[*it].limitDegree);
    }

    al::initNerve(this, &NrvRabbit.Wait, 4);
    mStateSwoon = new RabbitStateSwoon(this);
    mStateReverse = new RabbitStateReverse(this);
    mStateSupportFreeze = new ActorStateSupportFreeze(this, &sSupportFreezeParam);
    al::initNerveState(this, mStateReverse, &NrvRabbit.Reverse, "反転");
    al::initNerveState(this, mStateSwoon, &NrvRabbit.Swoon, "気絶");
    al::initNerveState(this, mStateSupportFreeze, &NrvRabbit.SupportFreeze, "[state]フリーズ");

    al::PlacementInfo routeInfo;
    al::getLinksInfo(&routeInfo, al::getPlacementInfo(rInfo), "StartPoint");
    auto* pRoute = new RabbitRoute(routeInfo);
    mRoute = pRoute;
    mRouteRider = new RabbitRouteRider(this, pRoute, mIsBig);
    al::setTrans(this, mRouteRider->getPos());
    al::tryGetArg(&mSpeed, rInfo, "Speed");
    mRouteRider->setSpeed(mSpeed);
    al::tryGetArg(&mMoveStartDistance, rInfo, "MoveStartDistance");
    al::tryGetArg(&mMoveStartDistancePlessie, rInfo, "MoveStartDistancePlessie");
    al::tryGetArg(&mMoveEndDistanceOffset, rInfo, "MoveEndDistanceOffset");
    al::tryGetArg(&mReverseStartDistance, rInfo, "ReverseStartDistance");

    al::PlacementInfo initPlaceInfo;
    if (al::tryGetLinksInfo(&initPlaceInfo, al::getPlacementInfo(rInfo), "RabbitInitPlacePoint")) {
        mInitPlacePoint = new RabbitInitPlacePoint(initPlaceInfo, mRoute->getStartPoint());
        al::setTrans(this, mInitPlacePoint->getPos());
        mRouteRider->updatePoint(mInitPlacePoint, mRoute->getStartPoint());
        al::setNerve(this, &NrvRabbit.WaitAtInitPoint);
    }

    switch (rc::getItemType(rInfo)) {
    case 14: {
        auto* pGreenStar = new GreenStar("グリーンスター");
        const char* pLinkName = al::calcLinkChildNum(rInfo, "GreenStarMysteryHouse") > 0 ?
                                    "GreenStarMysteryHouse" :
                                    "GreenStar";
        if (al::calcLinkChildNum(rInfo, "GreenStarKinopioBrigade") > 0) {
            pLinkName = "GreenStarKinopioBrigade";
        }

        al::initLinksActor(pGreenStar, rInfo, pLinkName, 0);
        pGreenStar->makeActorDead();
        mGreenStar = pGreenStar;
        break;
    }
    case 18: {
        auto* pParam = new ItemStatePopUpFrontParam();
        mKinokoBigPopUpParam = pParam;
        pParam->_40.y = 17.0f;
        pParam->_40.z = 7.5f;
        for (s32 i = 0; i < mKinokoBigs.capacity(); i++) {
            auto* pKinokoBig = new KinokoBig("巨大キノコ");
            pKinokoBig->init(rInfo);
            pKinokoBig->makeActorDead();
            mKinokoBigs.pushBack(pKinokoBig);
        }

        break;
    }
    case 25: {
        auto* pGenerator = new CoinBlowGenerator("放出コイン");
        mCoinBlowGenerator = pGenerator;
        CoinBlowGenerator::InitParam param;
        param.coinCount = 30;
        param.blowSpeed = 30.0f;
        param.randomSpeed = 8.0f;
        param.radialSpeed = 10.0f;
        pGenerator->initWithParam(param, rInfo);
        break;
    }
    case 28:
        if (mIsSingleMode) {
            mGoalItem = new GoalItem("GoalItem");
            al::initLinksActor(mGoalItem, rInfo, "GoalItem", 0);
            mGoalItem->makeActorDead();
        }

        break;
    default:
        rc::addItemByHostInfo(this, rInfo, nullptr, nullptr);
        break;
    }

    al::tryGetArg(&mIsAppearItemQuickly, rInfo, "IsAppearItemQuickly");
    bool isHide = false;
    al::tryGetArg(&isHide, rInfo, "IsHide");
    bool isBushPlacement = false;
    al::tryGetArg(&isBushPlacement, rInfo, "IsBushPlacementOnRabbit");
    if (isHide || isBushPlacement) {
        al::setNerve(this, &NrvRabbit.Hide);
        if (isBushPlacement) {
            s32 color = 0;
            al::tryGetArg(&color, rInfo, "Color");
            mBush = new Bush("茂み[ウサギ用]");
            mBush->initBushOnRabbit(rInfo, al::getTrans(this), color);
        }
    }

    al::tryGetArg(&mAppearDistance, rInfo, "AppearDistance");
    bool isDashGuide = false;
    al::tryGetArg(&isDashGuide, rInfo, "IsDashGuide");
    if (isDashGuide) {
        mGuideTimer = 360;
    }

    mBeatAnimeController = new BgmBeatAnimeController(this);
    al::listenStageSwitchOn(this, "SwitchGoalComplete", RabbitFunctor(this, &Rabbit::goalComplete));
    al::tryGetArg(&mScenarioId, rInfo, "ScenarioID");
    s32 quadrant = 0;
    al::tryGetArg(&quadrant, rInfo, "Quadrant");
    mIslandId = quadrant != 0 ? IslandDataFunction::getIslandIDFromParam(quadrant) :
                                mPlacementHolder->getZoneNo();

    s32 unlockScenarioId = -1;
    s32 unlockIslandId = 0;
    al::tryGetArg(&unlockIslandId, rInfo, "UnlockIslandID");
    al::tryGetArg(&unlockScenarioId, rInfo, "UnlockScenarioID");
    if (unlockScenarioId >= 0 &&
        SingleModeDataFunction::isScenarioComplete(GameDataHolderAccessor(this), unlockIslandId - 1,
                                                   unlockScenarioId - 1)) {
        appear();
    } else if (al::listenStageSwitchOn(this, "SwitchAppear",
                                       RabbitFunctor(this, &Rabbit::appear))) {
        makeActorDead();
    } else {
        makeActorAppeared();
    }

    al::tryGetArg(&mIsStopOnGroundOnly, rInfo, "StopOnGroundOnly");
    if (mIsSingleMode) {
        mGoalCamera = al::initProgramableCamera_RS(this, rInfo, "RabbitGoalCamera", &mGoalCameraPos,
                                                   &mGoalCameraAt, nullptr);
    }
}

/**
 * @brief Removes the item-available effect once the rabbit's scenario has been completed.
 */
void Rabbit::goalComplete() {
    if (!mIsSingleMode) {
        return;
    }

    if (isScenarioComplete() && getEffectKeeper() != nullptr) {
        al::tryDeleteEffect(this, "ItemAvailable");
        mIsItemAvailableEffect = false;
    }
}

/**
 * @brief Registers the rabbit's shine on the island map.
 */
void Rabbit::initAfterPlacement() {
    al::LiveActor::initAfterPlacement();
    if (!mIsSingleMode || mGoalItem == nullptr) {
        return;
    }

    auto* pIslandMap = al::tryGetSceneObj<IslandMap>(this, SceneObjID_IslandMap);
    if (pIslandMap == nullptr) {
        return;
    }

    s32 islandId = mIslandId - 1;
    s32 scenarioId = mGoalItem->getShineId() - 1;
    pIslandMap->addSpecialShineLocation(this, al::getTrans(this), {islandId, scenarioId});
    pIslandMap->setSpecialShineIconComplete(
        this, SingleModeDataFunction::isScenarioComplete(GameDataHolderAccessor(this), islandId,
                                                         scenarioId));
}

/**
 * @brief Makes the rabbit appear and starts its item effects.
 */
void Rabbit::makeActorAppeared() {
    al::LiveActor::makeActorAppeared();
    if (mGreenStar != nullptr) {
        al::emitEffect(this, "GreenStarStorage", nullptr);
    }

    if (mIsSingleMode && getEffectKeeper() != nullptr && !isScenarioComplete()) {
        mIsItemAvailableEffect = al::tryEmitEffect(this, "ItemAvailable", nullptr);
    }
}

/**
 * @brief Puts the rabbit back to the start of its route (e.g. when the timer is cancelled).
 */
void Rabbit::cancel() {
    if (!mIsSingleMode) {
        return;
    }

    al::setTrans(this, mRoute->getStartPoint()->getPos());
    const IUseRabbitRoutePoint* pStartPoint = mRoute->getStartPoint();
    mRouteRider->updatePoint(pStartPoint, pStartPoint->getNextUniquePoint(0));
    al::showModelIfHide(this);
    al::LiveActor::calcAnim();
    al::setNerve(this, &NrvRabbit.Wait);
}

/**
 * @brief Sends a touch message to NPC sensors.
 * @param pSelf Sensor of this rabbit.
 * @param pOther Sensor that was hit.
 */
void Rabbit::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isNerve(this, &NrvRabbit.Hide)) {
        return;
    }

    if (al::isSensorNpc(pSelf)) {
        al::sendMsgNpcTouch(pOther, pSelf);
    }
}

/**
 * @brief Handles being caught or knocked out by player attacks.
 * @param pMsg Received message.
 * @param pSelf Sensor of this rabbit.
 * @param pOther Sensor of the sender.
 * @return Whether the message was handled.
 */
bool Rabbit::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (mIsCaught) {
        return false;
    }

    if (al::isMsgPlayerCatch(pMsg) || al::isMsgPlayerTrample(pMsg) ||
        al::isMsgPlayerTailAttack(pMsg) || al::isMsgPlayerBodyAttack(pMsg) ||
        al::isMsgPlayerSpinAttack(pMsg) || al::isMsgPlayerGiantAttack(pMsg) ||
        al::isMsgPlayerClimbAttack(pMsg) || al::isMsgPlayerObjHipDropReflectAll(pMsg) ||
        rc::isMsgSkateShoesAttack(pMsg)) {
        rc::requestHitReactionToAttackerNpc(pOther, pSelf);
        mCatchSensor = pSelf;
        mIsCaught = true;
        al::startHitReactionGet(this);
        if (mGreenStar != nullptr) {
            al::deleteEffect(this, "GreenStarStorage");
        }

        if (!al::isNerve(this, &NrvRabbit.Jump) && !al::isNerve(this, &NrvRabbit.SupportFreeze)) {
            al::setNerve(this, &NrvRabbit.Catch);
        }

        return true;
    }

    bool isBoomerang = mIsBig ? al::isMsgPlayerBoomerangReflect(pMsg) :
                                al::isMsgPlayerBoomerangAttack(pMsg);
    if (isBoomerang || al::isMsgPlayerFireBallAttack(pMsg) || al::isMsgBallAttack(pMsg) ||
        al::isMsgBallTrample(pMsg) || al::isMsgPlayerKouraAttack(pMsg) ||
        al::isMsgKickKouraAttack(pMsg) || al::isMsgExplosion(pMsg) ||
        (al::isMsgBlockUpperPunch(pMsg) && !al::isNerve(this, &NrvRabbit.Jump))) {
        if (al::isNerve(this, &NrvRabbit.Swoon) && al::isLessStep(this, 30)) {
            return false;
        }

        rc::requestHitReactionToAttackerNpc(pOther, pSelf);
        startSwoon(pMsg);
        return true;
    }

    return false;
}

/**
 * @brief Checks whether the rabbit is jumping between route points.
 * @return Whether the jump nerve is active.
 */
bool Rabbit::isStateJump() const {
    return al::isNerve(this, &NrvRabbit.Jump);
}

/**
 * @brief Handles touch-screen freezing and knock-outs.
 * @param pMsg Received message.
 * @param pPointer Screen pointer that touched the rabbit.
 * @param pTarget Touched screen point target.
 * @return Whether the message was handled.
 */
bool Rabbit::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                   al::ScreenPointTarget* pTarget) {
    if (mIsCaught) {
        return false;
    }

    if (mStateSupportFreeze->receiveMsgScreenPoint(pMsg, pPointer, pTarget)) {
        if (!al::isNerve(this, &NrvRabbit.Hide) && !al::isNerve(this, &NrvRabbit.Appear) &&
            !al::isNerve(this, &NrvRabbit.SupportFreeze) && !al::isNerve(this, &NrvRabbit.Catch) &&
            !al::isNerve(this, &NrvRabbit.Disappear) &&
            !(al::isNerve(this, &NrvRabbit.Swoon) && al::isLessStep(this, 60))) {
            mNerveAfterSupportFreeze = getNerveKeeper()->getCurrentNerve();
            al::setNerve(this, &NrvRabbit.SupportFreeze);
        }

        return true;
    }

    if (al::isMsgTouchAssistTrigNoPat(pMsg) && !mIsSwoonRequested &&
        !al::isNerve(this, &NrvRabbit.Hide) && !al::isNerve(this, &NrvRabbit.Appear) &&
        !al::isNerve(this, &NrvRabbit.Catch) && !al::isNerve(this, &NrvRabbit.Disappear) &&
        !(al::isNerve(this, &NrvRabbit.Swoon) && al::isLessStep(this, 60))) {
        startSwoon(pMsg);
        return true;
    }

    return false;
}

/**
 * @brief Makes the rabbit appear (stage switch listener).
 */
void Rabbit::appear() {
    al::LiveActor::appear();
}

/**
 * @brief Updates the dash guide, material codes, route reset and item-available effect.
 */
void Rabbit::control() {
    if (mGuideTimer == 0 &&
        rc::tryFindNearestActivePlayerOrKoopaJrActorInCylinder(this, 2000.0f, -500.0f, 500.0f) ==
            nullptr) {
        hideGuide();
    }

    if (!mIsSingleMode || !mIsBig) {
        return;
    }

    sead::Vector3f hitPos;
    sead::Vector3f hitNormal;
    sead::Vector3f arrow;
    sead::Vector3f checkPos = al::getTrans(this);
    checkPos.y += -5.0f;
    arrow = {0.0f, -10.0f, 0.0f};
    if (alCollisionUtil::getHitPosAndNormalOnArrow(this, &hitPos, &hitNormal, al::getTrans(this),
                                                   arrow, nullptr, nullptr) &&
        hitNormal.y > 0.5f) {
        al::tryUpdateEffectMaterialCode(this, "NoCode");
        al::tryUpdateSeMaterialCode(this, "NoCode");
    } else if (rc::isInWaterArea(this, checkPos)) {
        al::tryUpdateEffectMaterialCode(this, "Water");
        al::tryUpdateSeMaterialCode(this, "Water");
    }

    if (rc::calcActivePlayerNum(this) < 1) {
        return;
    }

    al::LiveActor* pPlayer = rc::findNearestActivePlayerActor(this);
    if (!al::isNear(pPlayer, this, 20000.0f) &&
        !al::isNear(pPlayer, mRoute->getStartPoint()->getPos(), 10000.0f) &&
        !al::isNear(mRouteRider->getPos(), mRoute->getStartPoint()->getPos(), 500.0f)) {
        al::tryEmitEffect(this, "Disappear", nullptr);
        al::hideModelIfShow(this);
        al::setTrans(this, mRoute->getStartPoint()->getPos());
        const IUseRabbitRoutePoint* pStartPoint = mRoute->getStartPoint();
        mRouteRider->updatePoint(pStartPoint, pStartPoint->getNextUniquePoint(0));
        al::setNerve(this, &NrvRabbitPoof);
    }

    bool isInTunnel = rc::isInPlessieTunnel(this, al::getTrans(this));
    bool isPlayerInTunnel = rc::isInPlessieTunnel(pPlayer, al::getTrans(pPlayer));
    if (isInTunnel != isPlayerInTunnel) {
        if (!mIsItemAvailableEffect) {
            return;
        }

        if (!SingleModeDataFunction::isScenarioComplete(GameDataHolderAccessor(this), mIslandId - 1,
                                                        mScenarioId - 1) &&
            getEffectKeeper() != nullptr) {
            al::tryDeleteEffect(this, "ItemAvailable");
            mIsItemAvailableEffect = false;
        }
    } else {
        if (mIsItemAvailableEffect) {
            return;
        }

        if (!SingleModeDataFunction::isScenarioComplete(GameDataHolderAccessor(this), mIslandId - 1,
                                                        mScenarioId - 1) &&
            getEffectKeeper() != nullptr) {
            mIsItemAvailableEffect = al::tryEmitEffect(this, "ItemAvailable", nullptr);
        }
    }
}

/**
 * @brief Checks whether the rabbit stands on upward-facing ground.
 * @return Whether ground was found below the rabbit.
 */
bool Rabbit::isRabbitOnGround() {
    sead::Vector3f hitPos;
    sead::Vector3f hitNormal;
    sead::Vector3f arrow = {0.0f, -10.0f, 0.0f};
    if (alCollisionUtil::getHitPosAndNormalOnArrow(this, &hitPos, &hitNormal, al::getTrans(this),
                                                   arrow, nullptr, nullptr) &&
        hitNormal.y > 0.5f) {
        return true;
    }

    return false;
}

/**
 * @brief Hides the dash guide when the rabbit gets clipped.
 */
void Rabbit::startClipped() {
    if (mGuideTimer == 0) {
        hideGuide();
    }

    al::LiveActor::startClipped();
}

/**
 * @brief Moves the rabbit along its route and counts down its timers.
 * @param speed Moving speed.
 */
void Rabbit::moveOnRoute(f32 speed) {
    mRouteRider->setSpeed(speed);
    mRouteRider->move();
    al::setTrans(this, mRouteRider->getPos());
    al::turnDirectionDegree(this, al::getFrontPtr(this), mRouteRider->getFront(), 10.0f);
    if (mReverseCoolTime > 0) {
        mReverseCoolTime--;
    }

    if (mGuideTimer > 0 && --mGuideTimer == 0) {
        rc::appearGuideGameWindow(this, "GuideMessage", "RabbitDash", -1, 0.0f);
    }
}

/**
 * @brief Turns the rabbit to face the camera horizontally.
 */
void Rabbit::faceToCamera() {
    sead::Vector3f dir;
    al::calcCameraDir(&dir, this);
    dir.y = 0.0f;
    if (al::normalizeOrZero(&dir)) {
        dir.e = sead::Vector3f::ez.e;
    }

    al::faceToDirection(this, dir);
}

/**
 * @brief Starts running when a player comes near.
 * @param pNerve Nerve to start.
 * @param isTurn Whether to turn toward the nearest player.
 * @return Whether the nerve was started.
 */
bool Rabbit::tryStartRun(const al::Nerve* pNerve, bool isTurn) {
    if (rc::calcActivePlayerNum(this) < 1) {
        return false;
    }

    al::LiveActor* pPlayer = rc::findNearestActivePlayerOrKoopaJrActor(this);
    if (isTurn) {
        al::turnDirectionToTargetDegree(this, al::getFrontPtr(this), al::getTrans(pPlayer), 8.0f);
    }

    if (!isPlayerNear(pPlayer)) {
        return false;
    }

    mReverseCoolTime = 0;
    al::setNerve(this, pNerve);
    return true;
}

/**
 * @brief Checks whether a player is close enough to make the rabbit run.
 * @param pPlayer Player or Koopa Jr. actor.
 * @return Whether the actor is near.
 */
bool Rabbit::isPlayerNear(const al::LiveActor* pPlayer) {
    if (pPlayer == nullptr) {
        return false;
    }

    if (mIsSingleMode && al::isEqualString(pPlayer->getName(), "KoopaJr")) {
        return al::isNear(this, pPlayer, mMoveStartDistance);
    }

    const auto* pPlayerActor = static_cast<const PlayerActor*>(pPlayer);
    return al::isNear(this, pPlayer,
                      pPlayerActor->isRaidonExist() ? mMoveStartDistancePlessie :
                                                      mMoveStartDistance);
}

/**
 * @brief Checks whether a player is ahead of the rabbit on its route so it should turn back.
 * @param pPlayer Player actor.
 * @return Whether the rabbit can reverse.
 */
bool Rabbit::canStartReverse(const al::LiveActor* pPlayer) const {
    if (mReverseCoolTime > 0) {
        return false;
    }

    if (!al::isNear(this, pPlayer, mReverseStartDistance)) {
        return false;
    }

    if (mInitPlacePoint != nullptr && mRouteRider->getCurrentPoint() == mInitPlacePoint) {
        return false;
    }

    sead::Vector3f playerPosOnRoute;
    mRoute->calcNearestPosOnRoute(&playerPosOnRoute, al::getTrans(pPlayer));
    sead::Vector3f playerFoot;
    al::calcPerpendicFootToLineInside(&playerFoot, playerPosOnRoute,
                                      mRouteRider->getCurrentPointPos(),
                                      mRouteRider->getNextPointPos());
    sead::Vector3f rabbitFoot;
    al::calcPerpendicFootToLineInside(&rabbitFoot, al::getTrans(this),
                                      mRouteRider->getCurrentPointPos(),
                                      mRouteRider->getNextPointPos());
    const sead::Vector3f& rCurrentPos = mRouteRider->getCurrentPointPos();
    return (playerFoot - rCurrentPos).squaredLength() > (rabbitFoot - rCurrentPos).squaredLength();
}

/**
 * @brief Turns back on the route if a player is ahead.
 * @param pPlayer Player actor.
 * @param pNerve Nerve to start after reversing.
 * @return Whether the rabbit started reversing.
 */
bool Rabbit::tryStartReverse(const al::LiveActor* pPlayer, const al::Nerve* pNerve) {
    if (!canStartReverse(pPlayer)) {
        return false;
    }

    startReverse(pNerve);
    return true;
}

/**
 * @brief Pops out one big mushroom per active player at fixed steps of the catch animation.
 * @param step Current step of the catch nerve.
 * @return Whether a mushroom appeared.
 */
bool Rabbit::tryAppearKinokoBig(s32 step) {
    if (mKinokoBigs.size() == 0) {
        return false;
    }

    s32 playerNum = rc::calcActivePlayerNum(this);
    sead::Vector3f basePos = al::getTrans(this) + sKinokoBigOffset;
    for (s32 i = 0; i < playerNum; i++) {
        if (step != 50 + i * 20) {
            continue;
        }

        sead::Vector3f front;
        al::calcFrontDir(&front, this);
        sead::Vector3f dir;
        switch (playerNum) {
        case 1:
            dir = front;
            break;
        case 2:
            dir = front;
            if (i == 0) {
                al::rotateVectorDegreeY(&dir, -20.0f);
            } else if (i == 1) {
                al::rotateVectorDegreeY(&dir, 20.0f);
            }

            break;
        case 3:
            dir = front;
            if (i == 0) {
                al::rotateVectorDegreeY(&dir, -40.0f);
            } else if (i == 2) {
                al::rotateVectorDegreeY(&dir, 40.0f);
            }

            break;
        case 4:
            dir = front;
            switch (i) {
            case 0:
                al::rotateVectorDegreeY(&dir, -60.0f);
                break;
            case 1:
                al::rotateVectorDegreeY(&dir, -20.0f);
                break;
            case 2:
                al::rotateVectorDegreeY(&dir, 20.0f);
                break;
            case 3:
                al::rotateVectorDegreeY(&dir, 60.0f);
                break;
            }

            break;
        }

        al::setTrans(mKinokoBigs[i], basePos + dir * 100.0f);
        al::faceToDirection(mKinokoBigs[i], dir);
        mKinokoBigs[i]->appearPopUpFront(*mKinokoBigPopUpParam);
        return true;
    }

    return false;
}

/**
 * @brief Gets the distance at which a running rabbit stops when no player follows.
 * @return The move end distance.
 */
f32 Rabbit::getMoveEndDistance() const {
    return mMoveStartDistance + mMoveEndDistanceOffset;
}

/**
 * @brief Stays hidden until a player comes near.
 */
void Rabbit::exeHide() {
    if (al::isFirstStep(this)) {
        al::hideModel(this);
    }

    if (rc::calcActivePlayerNum(this) >= 1 &&
        al::isNear(this, rc::findNearestActivePlayerOrKoopaJrActor(this), mAppearDistance)) {
        al::showModel(this);
        al::setNerve(this, &NrvRabbit.Appear);
    }
}

/**
 * @brief Jumps out of hiding (from a bush when placed in one).
 */
void Rabbit::exeAppear() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Appear");
        if (mBush != nullptr) {
            mRouteRider->updatePoint(mRouteRider->getCurrentPoint()->getNextUniquePoint(0),
                                     nullptr);
            mBush->tryStartRabbitAppear();
        }
    }

    faceToCamera();

    if (mBush != nullptr) {
        f32 rate = al::calcNerveRate(this, al::getActionFrameMax(this, "Appear"));
        const IUseRabbitRoutePoint* pStartPoint = mInitPlacePoint;
        if (pStartPoint == nullptr) {
            pStartPoint = mRoute->getStartPoint();
        }

        sead::Vector3f startPos = pStartPoint->getPos();
        al::lerpVec(al::getTransPtr(this), startPos, mRouteRider->getCurrentPointPos(), rate);
    }

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvRabbitAppearWait);
    }
}

/**
 * @brief Teases the player after appearing until they come near.
 */
void Rabbit::exeAppearWait() {
    if (al::isFirstStep(this)) {
        if (mIsBig) {
            al::startAction(this, "WaitTease");
        } else {
            mBeatAnimeController->init("WaitTeaseRhythm", false, false, false);
        }
    }

    if (!mIsBig) {
        mBeatAnimeController->update();
    }

    mIsBeatAnime = true;
    tryStartRun(&NrvRabbitFind, true);
}

/**
 * @brief Waits until a player comes near.
 */
void Rabbit::exeWait() {
    if (al::isFirstStep(this)) {
        al::startActionAtRandomFrame(this, "Wait");
    }

    tryStartRun(&NrvRabbit.FindFirst, true);
}

/**
 * @brief Waits at the initial place point until a player comes near.
 */
void Rabbit::exeWaitAtInitPoint() {
    if (al::isFirstStep(this)) {
        al::startActionAtRandomFrame(this, "Wait");
    }

    if (rc::calcActivePlayerNum(this) >= 1 &&
        al::isNear(this, rc::findNearestActivePlayerOrKoopaJrActor(this), 800.0f)) {
        al::setNerve(this, &NrvRabbitFindAtInitPoint);
    }
}

/**
 * @brief Notices a player at the initial place point and starts running.
 */
void Rabbit::exeFindAtInitPoint() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "FindFirst");
    }

    if (!al::isActionEnd(this)) {
        return;
    }

    const al::Nerve* pNextNerve = &NrvRabbit.Run;
    if (mInitPlacePoint->isActionJump()) {
        pNextNerve = &NrvRabbit.JumpStart;
    }

    if (canStartReverse(rc::findNearestActivePlayerOrKoopaJrActor(this))) {
        startReverseNoStop(pNextNerve);
        return;
    }

    al::setNerve(this, pNextNerve);
}

/**
 * @brief Stops running when no player follows and waits for them again.
 */
void Rabbit::exeRunBreak() {
    if (al::isFirstStep(this)) {
        mIsBeatAnime = false;
        if (mIsBig) {
            al::startAction(this, "WaitTease");
        } else {
            mBeatAnimeController->init("WaitTeaseRhythm", false, false, false);
        }
    }

    if (!mIsBig) {
        mBeatAnimeController->update();
    }

    mIsBeatAnime = true;
    tryStartRun(&NrvRabbitFind, true);
}

/**
 * @brief Notices a player and starts running.
 */
void Rabbit::exeFind() {
    if (al::isFirstStep(this)) {
        al::startAction(this, al::isNerve(this, &NrvRabbit.FindFirst) ? "FindFirst" : "Find");
        if (mIsSingleMode) {
            TimerManager* pTimerManager = TimerManager::tryGetTimerManager(this);
            if (pTimerManager != nullptr) {
                pTimerManager->setCurrentRabbit(this);
            }
        }
    }

    if (!al::isActionEnd(this)) {
        return;
    }

    if (canStartReverse(rc::findNearestActivePlayerOrKoopaJrActor(this))) {
        startReverseNoStop(&NrvRabbit.Run);
        return;
    }

    al::setNerve(this, &NrvRabbit.Run);
}

/**
 * @brief Runs along the route away from the players.
 */
void Rabbit::exeRun() {
    bool isJumpEnd = al::isActionPlaying(this, "JumpEnd");
    if (al::isFirstStep(this) && !isJumpEnd) {
        al::startAction(this, "Run");
    }

    if (isJumpEnd && al::isActionEnd(this)) {
        al::startAction(this, "Run");
    }

    al::LiveActor* pPlayer =
        rc::tryFindNearestActivePlayerOrKoopaJrActorInSphere(this, getMoveEndDistance());
    if (al::isGreaterEqualStep(this, 75) && pPlayer == nullptr) {
        if (!mIsStopOnGroundOnly) {
            al::setNerve(this, &NrvRabbit.RunBreak);
            return;
        }

        if (mIsSingleMode && mIsBig) {
            const IUseRabbitRoutePoint* pNextPoint = mRouteRider->getNextPoint();
            if (pNextPoint != nullptr && pNextPoint->isPointOnLand() &&
                mRouteRider->isNearNextPoint()) {
                al::setNerve(this, &NrvRabbit.RunBreak);
                return;
            }
        } else if (isRabbitOnGround()) {
            al::setNerve(this, &NrvRabbit.RunBreak);
            return;
        }
    }

    moveOnRoute(mSpeed);
    f32 frameRate = al::isActionPlaying(this, "Run") ? (mSpeed - 5.0f) * 0.0f + 1.0f : 1.0f;
    al::setSklAnimFrameRate(this, frameRate, 0);
    if (mRouteRider->isJumping()) {
        al::setSklAnimFrameRate(this, 1.0f, 0);
        al::setNerve(this, &NrvRabbit.JumpStart);
        return;
    }

    if (pPlayer != nullptr && al::isIntervalStep(this, 30, 0) &&
        tryStartReverse(pPlayer, &NrvRabbit.Run)) {
        al::setSklAnimFrameRate(this, 1.0f, 0);
    }
}

/**
 * @brief Prepares a jump between route points.
 */
void Rabbit::exeJumpStart() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "JumpStart");
    }

    if (al::isStep(this, mJumpStartStep)) {
        al::setNerve(this, &NrvRabbit.Jump);
    }
}

/**
 * @brief Jumps between route points.
 */
void Rabbit::exeJump() {
    bool isJumpEnd = al::isActionPlaying(this, "JumpEnd");
    if (al::isFirstStep(this) && !isJumpEnd) {
        al::startAction(this, "JumpLoop");
    }

    if (isJumpEnd && al::isActionEnd(this)) {
        al::startAction(this, "JumpLoop");
    }

    moveOnRoute(mSpeed);
    if (!mRouteRider->isJumping() || mRouteRider->isJumpStart()) {
        if (mIsCaught) {
            al::setNerve(this, &NrvRabbit.Catch);
            return;
        }

        if (mIsSwoonRequested) {
            al::setNerve(this, &NrvRabbit.Swoon);
            return;
        }
    }

    if (!mRouteRider->isJumping()) {
        al::startAction(this, "JumpEnd");
        al::setNerve(this, &NrvRabbit.Run);
        return;
    }

    if (!mRouteRider->isJumpStart()) {
        return;
    }

    al::LiveActor* pPlayer =
        rc::tryFindNearestActivePlayerOrKoopaJrActorInSphere(this, getMoveEndDistance());
    if (pPlayer != nullptr && tryStartReverse(pPlayer, &NrvRabbit.JumpStart)) {
        return;
    }

    al::startAction(this, "JumpEnd");
    al::setNerve(this, &NrvRabbit.Jump);
}

/**
 * @brief Turns back on the route.
 */
void Rabbit::exeReverse() {
    al::updateNerveStateAndNextNerve(this, mNerveAfterReverse);
}

/**
 * @brief Stays knocked out, then resumes running or jumping.
 */
void Rabbit::exeSwoon() {
    if (al::updateNerveState(this)) {
        mIsSwoonRequested = false;
        mReverseCoolTime = 0;
        const al::Nerve* pNextNerve = &NrvRabbit.Run;
        if (mRouteRider->isJumping()) {
            pNextNerve = &NrvRabbit.Jump;
        }

        al::setNerve(this, pNextNerve);
    }
}

/**
 * @brief Gets caught: plays the goal demo and releases the carried item.
 */
void Rabbit::exeCatch() {
    if (al::isFirstStep(this)) {
        if (mGoalItem != nullptr && !rc::requestStartDemoInGameCutscene(this)) {
            al::setNerve(this, &NrvRabbit.Catch);
            return;
        }

        if (mIsSingleMode) {
            rc::setDemoAudioType(this, alSeFunction::DemoType(3));
            TimerManager* pTimerManager = TimerManager::tryGetTimerManager(this);
            if (pTimerManager != nullptr) {
                pTimerManager->resetCurrentRabbit(this);
            }
        }

        rc::addDemoActor(this);
        al::startAction(this, "Catch");
        if (mGuideTimer == 0) {
            rc::disappearGuideGameWindow(this);
        }

        if (mIsSingleMode) {
            al::LiveActor* pPlayer = al::getPlayerActor(getSceneInfo()->playerHolder, 0);
            sead::Vector3f rabbitPos = al::getTrans(this);
            sead::Vector3f playerPos = al::getTrans(pPlayer);
            NpcGoalCameraTarget cameraTarget(&mGoalCameraAt, &mGoalCameraPos);
            cameraTarget.setPlayerMustBeVisible(true);
            sead::Vector3f lookAt = rabbitPos;
            f32 height = rabbitPos.y + 500.0f;
            if (!cameraTarget.calcSafeAngle(rabbitPos, playerPos, this, &lookAt, 2000.0f,
                                            &height)) {
                mGoalCameraAt = al::getCameraAt_RS(this, 0);
                mGoalCameraPos = al::getCameraPos_RS(this, 0);
            }

            al::startCamera_RS(this, mGoalCamera, -1);
        }
    }

    faceToCamera();
    s32 itemStep = mCoinBlowGenerator != nullptr ? 57 : (mIsAppearItemQuickly ? 40 : 50);
    if (al::isStep(this, itemStep)) {
        sead::Vector3f itemPos =
            mActorSceneInfo->isSingleMode ? al::findNearestPlayerPos(this) : al::getTrans(this);
        if (mGreenStar != nullptr) {
            itemPos.y += 250.0f;
            mGreenStar->appearWithPos(itemPos);
        } else if (mCoinBlowGenerator != nullptr) {
            sead::Vector3f up;
            al::calcUpDir(&up, this);
            sead::Vector3f front;
            al::calcFrontDir(&front, this);
            sead::Vector3f side;
            al::calcSideDir(&side, this);
            sead::Vector3f coinPos = al::getTrans(this) + side * sCoinBlowOffset.x +
                                     up * sCoinBlowOffset.y + front * sCoinBlowOffset.z;
            al::setTrans(mCoinBlowGenerator, coinPos);
            mCoinBlowGenerator->appear();
        } else if (mKinokoBigs.size() == 0 && mGoalItem == nullptr) {
            itemPos.y += 150.0f;
            al::appearItem(this, itemPos, al::getFront(this), mCatchSensor);
        }
    }

    tryAppearKinokoBig(al::getNerveStep(this));
    if (mIsSingleMode && mIsBig && al::isStep(this, 90) && mGoalItem != nullptr) {
        al::setNerve(this, &NrvRabbit.DisappearGoalItem);
        return;
    }

    if (al::isActionEnd(this)) {
        if (mGoalItem != nullptr) {
            al::setNerve(this, &NrvRabbit.DisappearGoalItem);
        } else {
            al::setNerve(this, &NrvRabbit.Disappear);
        }
    }
}

/**
 * @brief Disappears after being caught.
 */
void Rabbit::exeDisappear() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Disappear");
    }

    if (al::isActionEnd(this)) {
        kill();
    }
}

/**
 * @brief Hands the goal item to the player and disappears.
 */
void Rabbit::exeDisappearGoalItem() {
    if (al::isFirstStep(this)) {
        al::endCamera_RS(this, mGoalCamera, -1, false);
        mDisappearStep = 0;
        sead::Vector3f itemPos = al::findNearestPlayerPos(this);
        itemPos.y += 50.0f;
        al::setTrans(mGoalItem, itemPos);
        rc::requestEndDemoInGameCutscene(this);
        rc::requestStartDemoInGameCutscene(this);
        rc::addDemoActor(mGoalItem);
        rc::invalidatePlayerInput(this, 4);
        rc::setDemoFullEffectUpdate(this, true);
        mGoalItem->appearCollect(true);
        auto* pIslandMap = al::tryGetSceneObj<IslandMap>(this, SceneObjID_IslandMap);
        if (pIslandMap != nullptr) {
            pIslandMap->setSpecialShineIconComplete(this, true);
        }
    } else if (mDisappearStep == 30) {
        al::startAction(this, "Disappear");
    } else if (mDisappearStep > 30 && al::isActionEnd(this)) {
        al::hideModelIfShow(this);
        kill();
    }

    mDisappearStep++;
}

/**
 * @brief Stays frozen by touch, then resumes the interrupted nerve.
 */
void Rabbit::exeSupportFreeze() {
    if (mIsCaught) {
        mStateSupportFreeze->kill();
        if (mNerveAfterSupportFreeze != &NrvRabbit.Jump) {
            mNerveAfterSupportFreeze = &NrvRabbit.Catch;
        }
    }

    if (al::updateNerveStateAndNextNerve(this, mNerveAfterSupportFreeze) &&
        mNerveAfterSupportFreeze == &NrvRabbit.Swoon) {
        mStateSwoon->setAfterSupportFreeze();
    }
}

/**
 * @brief Reappears at the start of the route in a puff of smoke.
 */
void Rabbit::exePoof() {
    if (al::isFirstStep(this)) {
        al::showModelIfHide(this);
        al::tryEmitEffect(this, "Disappear", nullptr);
        al::setNerve(this, &NrvRabbit.Wait);
    }
}
