#include "CourseSelect/CourseSelectPuppeteer.hpp"

#include "CourseSelect/CourseSelectActorInfo.hpp"
#include "CourseSelect/CourseSelectConst.hpp"
#include "CourseSelect/CourseSelectDirector.hpp"
#include "CourseSelect/CourseSelectPlayerActor.hpp"
#include "CourseSelect/CourseSelectPuppeteerGroup.hpp"
#include "CourseSelect/ICourseSelectActorController.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Rail/RailUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Shadow/Common/ShadowUtil.hpp"
#include "Layout/PlayerEntryFunction.hpp"
#include "MapObj/PlayerCrown.hpp"
#include "MapObj/RouteDokanInOutEffect.hpp"
#include "Player/Normal/PlayerActor.hpp"
#include "Player/PlayerBindEndParam.hpp"
#include "Project/Block/BlockRailRider.hpp"
#include "Project/Collision/CollisionUtil.hpp"
#include "System/GameDataFlagFunction.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/GameDataHolderWriter.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/PlayerPuppetUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ScoreUtil.hpp"

namespace {
NERVE_DECL(CourseSelectPuppeteer, Wait);
NERVE_DECL(CourseSelectPuppeteer, EnterUser);
NERVE_DECL(CourseSelectPuppeteer, LeaveUser);
NERVE_DECL(CourseSelectPuppeteer, BindWait);
NERVE_DECL(CourseSelectPuppeteer, Landing);
NERVE_DECL(CourseSelectPuppeteer, EnterGateKeeper);
NERVE_DECL(CourseSelectPuppeteer, EnterHide);
NERVE_DECL(CourseSelectPuppeteer, EnterPrepare);
NERVE_DECL(CourseSelectPuppeteer, ExitKinopioBrigade);
NERVE_DECL(CourseSelectPuppeteer, PrepareExit);
NERVE_DECL(CourseSelectPuppeteer, EventGateKeeper);
NERVE_DECL(CourseSelectPuppeteer, OpenRoad);
NERVE_DECL(CourseSelectPuppeteer, UnLock);
NERVE_DECL(CourseSelectPuppeteer, LockAppear);
NERVE_DECL(CourseSelectPuppeteer, OpenGateKeeper);
NERVE_DECL(CourseSelectPuppeteer, DokanPrepareIn);
NERVE_DECL(CourseSelectPuppeteer, WorldWarpDokanStart);
NERVE_DECL(CourseSelectPuppeteer, RocketPrepareIn);
NERVE_DECL(CourseSelectPuppeteer, RocketBreakDemo);
NERVE_DECL(CourseSelectPuppeteer, RouteDokanMoveStart);
NERVE_DECL(CourseSelectPuppeteer, WarpToCourse);
NERVE_DECL(CourseSelectPuppeteer, WorldStartDemo);
NERVE_DECL(CourseSelectPuppeteer, WorldStartDemoW8);
NERVE_DECL(CourseSelectPuppeteer, HidePlayerDemo);
NERVE_DECL(CourseSelectPuppeteer, WaitDemo);
NERVE_DECL(CourseSelectPuppeteer, ReviveUserDelay);
NERVE_DECL(CourseSelectPuppeteer, ReviveUser);
NERVE_DECL(CourseSelectPuppeteer, DokanPrepareOut);
NERVE_DECL(CourseSelectPuppeteer, DokanWarpWait);
NERVE_DECL(CourseSelectPuppeteer, DokanEndWait);
NERVE_DECL(CourseSelectPuppeteer, RocketPrepareOut);
NERVE_DECL(CourseSelectPuppeteer, RocketLaunchWait);
NERVE_DECL(CourseSelectPuppeteer, RocketEndWait);
NERVE_DECL(CourseSelectPuppeteer, Enter);
NERVE_DECL(CourseSelectPuppeteer, Exit);
NERVE_DECL(CourseSelectPuppeteer, BindEndLanding);
NERVE_DECL(CourseSelectPuppeteer, DokanIn);
NERVE_DECL(CourseSelectPuppeteer, DokanOut);
NERVE_DECL(CourseSelectPuppeteer, WorldWarpDokanMove);
NERVE_DECL(CourseSelectPuppeteer, WorldWarpDokanOut);
NERVE_DECL(CourseSelectPuppeteer, RocketIn);
NERVE_DECL(CourseSelectPuppeteer, RocketOut);
NERVE_DECL(CourseSelectPuppeteer, RouteDokanMove);
NERVE_DECL(CourseSelectPuppeteer, RouteDokanMoveEnd);
// The nerves below and this bind end param are mutable globals, which the compiler merges into one
// block addressed from a common base, as in the game.

/// How the bind ends when the player jumps out of a route dokan.
PlayerBindEndParam sRouteDokanOutEndParam = {{}, 10, 0, true, false, true, 0, -1.0f, 10};

NERVES_MAKE_NOSTRUCT(CourseSelectPuppeteer, EventGateKeeper, OpenRoad, UnLock, LockAppear,
                     OpenGateKeeper, DokanPrepareIn, WorldWarpDokanStart, RocketPrepareIn,
                     RocketBreakDemo, RouteDokanMoveStart, WarpToCourse, WorldStartDemo,
                     WorldStartDemoW8, HidePlayerDemo, WaitDemo, DokanPrepareOut, DokanWarpWait,
                     RocketPrepareOut, RocketLaunchWait, Enter, Exit, BindEndLanding, DokanIn,
                     DokanOut, WorldWarpDokanMove, WorldWarpDokanOut, RocketIn, RocketOut,
                     RouteDokanMove, RouteDokanMoveEnd)
CourseSelectPuppeteerNrvWait NrvCourseSelectPuppeteerWait;
CourseSelectPuppeteerNrvEnterUser NrvCourseSelectPuppeteerEnterUser;
CourseSelectPuppeteerNrvLeaveUser NrvCourseSelectPuppeteerLeaveUser;
CourseSelectPuppeteerNrvBindWait NrvCourseSelectPuppeteerBindWait;
CourseSelectPuppeteerNrvLanding NrvCourseSelectPuppeteerLanding;
CourseSelectPuppeteerNrvEnterGateKeeper NrvCourseSelectPuppeteerEnterGateKeeper;
CourseSelectPuppeteerNrvEnterHide NrvCourseSelectPuppeteerEnterHide;
CourseSelectPuppeteerNrvEnterPrepare NrvCourseSelectPuppeteerEnterPrepare;
CourseSelectPuppeteerNrvExitKinopioBrigade NrvCourseSelectPuppeteerExitKinopioBrigade;
CourseSelectPuppeteerNrvPrepareExit NrvCourseSelectPuppeteerPrepareExit;
CourseSelectPuppeteerNrvReviveUserDelay NrvCourseSelectPuppeteerReviveUserDelay;
CourseSelectPuppeteerNrvReviveUser NrvCourseSelectPuppeteerReviveUser;
CourseSelectPuppeteerNrvDokanEndWait NrvCourseSelectPuppeteerDokanEndWait;
CourseSelectPuppeteerNrvRocketEndWait NrvCourseSelectPuppeteerRocketEndWait;

constexpr f32 cGravity = -1.6f;
}  // namespace

/**
 * @brief Creates the puppeteer of one control user, with its route-dokan rail rider and effect.
 * @param pGroup Group owning the puppeteer, used as the binder.
 * @param rInfo Actor init info for the effect.
 * @param userId Control user driven by this puppeteer.
 */
CourseSelectPuppeteer::CourseSelectPuppeteer(CourseSelectPuppeteerGroup* pGroup,
                                             const al::ActorInitInfo& rInfo, s32 userId)
    : BindPuppeteer("コース選択バインド操作"), mGroup(pGroup), mUserId(userId) {
    mRailRider = new al::BlockRailRider();
    mDokanEffect = new RouteDokanInOutEffect("ルート土管出入りエフェクト");
    al::initCreateActorNoPlacementInfoNoViewId(mDokanEffect, rInfo);
    initNerve(&NrvCourseSelectPuppeteerWait, 0);
}

/**
 * @brief Sets the player driven by this puppeteer.
 * @param pActor Player actor.
 */
void CourseSelectPuppeteer::setActor(PlayerActor* pActor) {
    mActor = pActor;
}

/**
 * @brief Starts a demo: binds the player if needed, then runs the given nerve.
 * Does nothing while the user is leaving, and goes back to waiting when there is no live player.
 * @param pController Map object the demo is played with, or nullptr.
 * @param pNextNerve Nerve to run once the player is bound.
 */
void CourseSelectPuppeteer::startDemoWithNextNerve(ICourseSelectActorController* pController,
                                                   const al::Nerve* pNextNerve) {
    if (pNextNerve != &NrvCourseSelectPuppeteerEnterUser &&
        al::isNerve(this, &NrvCourseSelectPuppeteerLeaveUser)) {
        return;
    }

    if (mActor == nullptr ||
        (rc::isPlayerDead(mActor) && pNextNerve != &NrvCourseSelectPuppeteerEnterUser)) {
        al::setNerve(this, &NrvCourseSelectPuppeteerWait);
        return;
    }

    mController = pController;
    if (isBind()) {
        if (pController != nullptr) {
            pController->startBind(this);
        }

        al::setNerve(this, pNextNerve);
        return;
    }

    mNextNerve = pNextNerve;
    mActor->requestBind(al::getHitSensor(mGroup, 0), 0.0f, 0);
    al::setNerve(this, &NrvCourseSelectPuppeteerBindWait);
}

/**
 * @brief Plays the landing animation, then runs the given nerve.
 * @param pNextNerve Nerve to run once the landing ends.
 */
void CourseSelectPuppeteer::startLandingWithNextNerve(const al::Nerve* pNextNerve) {
    rc::startPuppetAction(getPlayerPuppet(), "CourseSelectLand");
    al::setNerve(this, &NrvCourseSelectPuppeteerLanding);
    mNextNerve = pNextNerve;
}

/**
 * @brief Starts entering a course.
 * @param pController Course object entered.
 */
void CourseSelectPuppeteer::startEnterDemo(ICourseSelectActorController* pController) {
    const CourseSelectActorInfo* pInfo = pController->getCourseSelectActorInfo();
    if (pInfo->isEnterKinopioBrigade()) {
        startDemoWithNextNerve(pController, &NrvCourseSelectPuppeteerEnterGateKeeper);
    } else if (pInfo->isEnterHide()) {
        startDemoWithNextNerve(pController, &NrvCourseSelectPuppeteerEnterHide);
    } else {
        startDemoWithNextNerve(pController, &NrvCourseSelectPuppeteerEnterPrepare);
    }
}

/**
 * @brief Starts coming out of a course.
 * @param pController Course object left.
 */
void CourseSelectPuppeteer::startExitDemo(ICourseSelectActorController* pController) {
    if (pController->getCourseSelectActorInfo()->isEnterKinopioBrigade()) {
        startDemoWithNextNerve(pController, &NrvCourseSelectPuppeteerExitKinopioBrigade);
    } else {
        startDemoWithNextNerve(pController, &NrvCourseSelectPuppeteerPrepareExit);
    }
}

/**
 * @brief Starts the gate keeper event.
 * @param pController Gate keeper object.
 */
void CourseSelectPuppeteer::startEventGateKeeperDemo(ICourseSelectActorController* pController) {
    startDemoWithNextNerve(pController, &NrvCourseSelectPuppeteerEventGateKeeper);
}

/**
 * @brief Starts the road opening demo.
 * @param pController Object opening the road.
 */
void CourseSelectPuppeteer::startOpenRoadDemo(ICourseSelectActorController* pController) {
    startDemoWithNextNerve(pController, &NrvCourseSelectPuppeteerOpenRoad);
}

/**
 * @brief Starts the unlock demo.
 * @param pController Lock object.
 */
void CourseSelectPuppeteer::startUnLockDemo(ICourseSelectActorController* pController) {
    startDemoWithNextNerve(pController, &NrvCourseSelectPuppeteerUnLock);
}

/**
 * @brief Starts the lock appearance demo.
 * @param pController Lock object.
 */
void CourseSelectPuppeteer::startLockAppearDemo(ICourseSelectActorController* pController) {
    startDemoWithNextNerve(pController, &NrvCourseSelectPuppeteerLockAppear);
}

/**
 * @brief Starts the gate keeper opening demo.
 * @param pController Gate keeper object.
 */
void CourseSelectPuppeteer::startOpenGateKeeperDemo(ICourseSelectActorController* pController) {
    startDemoWithNextNerve(pController, &NrvCourseSelectPuppeteerOpenGateKeeper);
}

/**
 * @brief Starts entering a dokan.
 * @param pController Dokan object.
 */
void CourseSelectPuppeteer::startDokanDemo(ICourseSelectActorController* pController) {
    startDemoWithNextNerve(pController, &NrvCourseSelectPuppeteerDokanPrepareIn);
}

/**
 * @brief Starts travelling through a world warp dokan.
 * @param pController World warp dokan object.
 */
void CourseSelectPuppeteer::startWorldWarpDokanDemo(ICourseSelectActorController* pController) {
    startDemoWithNextNerve(pController, &NrvCourseSelectPuppeteerWorldWarpDokanStart);
}

/**
 * @brief Starts boarding a rocket.
 * @param pController Rocket object.
 */
void CourseSelectPuppeteer::startRocketDemo(ICourseSelectActorController* pController) {
    startDemoWithNextNerve(pController, &NrvCourseSelectPuppeteerRocketPrepareIn);
}

/**
 * @brief Starts the rocket break demo.
 * @param pController Rocket object.
 */
void CourseSelectPuppeteer::startRocketBreakDemo(ICourseSelectActorController* pController) {
    startDemoWithNextNerve(pController, &NrvCourseSelectPuppeteerRocketBreakDemo);
}

/**
 * @brief Starts travelling through a route dokan.
 * @param pController Route dokan object, which places the rail rider.
 */
void CourseSelectPuppeteer::startRouteDokan(ICourseSelectActorController* pController) {
    pController->startRouteDokanRider(mRailRider);
    startDemoWithNextNerve(pController, &NrvCourseSelectPuppeteerRouteDokanMoveStart);
}

/**
 * @brief Starts warping to a course.
 * @param pController Course object warped to.
 */
void CourseSelectPuppeteer::startWarpToCourse(ICourseSelectActorController* pController) {
    startDemoWithNextNerve(pController, &NrvCourseSelectPuppeteerWarpToCourse);
}

/**
 * @brief Starts the world start demo.
 */
void CourseSelectPuppeteer::startWorldStartDemo() {
    startDemoWithNextNerve(nullptr, &NrvCourseSelectPuppeteerWorldStartDemo);
}

/**
 * @brief Starts the world 8 start demo.
 */
void CourseSelectPuppeteer::startWorldStartDemoW8() {
    startDemoWithNextNerve(nullptr, &NrvCourseSelectPuppeteerWorldStartDemoW8);
}

/**
 * @brief Starts hiding the player.
 */
void CourseSelectPuppeteer::startHidePlayerDemo() {
    startDemoWithNextNerve(nullptr, &NrvCourseSelectPuppeteerHidePlayerDemo);
}

/**
 * @brief Starts making the player wait.
 */
void CourseSelectPuppeteer::startWaitDemo() {
    startDemoWithNextNerve(nullptr, &NrvCourseSelectPuppeteerWaitDemo);
}

/**
 * @brief Releases the player on the ground if it is bound.
 */
void CourseSelectPuppeteer::endDemo() {
    if (!isBind()) {
        return;
    }

    endBindOnGround();
    al::setNerve(this, &NrvCourseSelectPuppeteerWait);
}

/**
 * @brief Updates the nerve, and starts a requested leave once the player stands on the ground.
 */
void CourseSelectPuppeteer::update() {
    updateNerve();
    if (!mIsRequestLeave) {
        return;
    }

    CourseSelectDirector* pDirector = CourseSelectDirector::getCourseSelectDirector(mGroup);
    al::LiveActor* pPlayer = rc::findPlayerActorFirstByUserId(pDirector->getMainPlayer(), mUserId);
    if (pPlayer != nullptr && rc::isPlayerOnGround(pPlayer)) {
        mActor = static_cast<PlayerActor*>(pPlayer);
        startDemoWithNextNerve(nullptr, &NrvCourseSelectPuppeteerLeaveUser);
        mIsRequestLeave = false;
    }
}

/**
 * @brief Handles the bind messages of the player.
 * @param pMsg Received message.
 * @param pSender Sender sensor (the player).
 * @param pReceiver Receiver sensor.
 * @return Whether the message was handled.
 */
bool CourseSelectPuppeteer::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSender,
                                       al::HitSensor* pReceiver) {
    if (al::isMsgBindStart(pMsg)) {
        return true;
    }

    if (al::isMsgBindInit(pMsg)) {
        al::setNerve(this, mNextNerve);
        mNextNerve = nullptr;
        startBind(pSender, pReceiver);
        if (mController != nullptr) {
            mController->startBind(this);
        }

        return true;
    }

    if (al::isMsgBindCancel(pMsg)) {
        cancelBind();
        al::setNerve(this, &NrvCourseSelectPuppeteerWait);
        return true;
    }

    return false;
}

/**
 * @brief Checks if no demo is playing.
 * @return Whether the puppeteer waits.
 */
bool CourseSelectPuppeteer::isDemoEnd() const {
    return al::isNerve(this, &NrvCourseSelectPuppeteerWait);
}

/**
 * @brief Activates the player of the user at a position and starts a demo.
 * @param pNerve Demo nerve.
 * @param isHidePlayer Whether the player starts hidden.
 * @param rTrans Position of the player.
 */
void CourseSelectPuppeteer::activateUser(const al::Nerve* pNerve, bool isHidePlayer,
                                         const sead::Vector3f& rTrans) {
    PlayerActor* pActor = mActor;
    s32 port = rc::getControlUserPortNumber(GameDataHolderAccessor(pActor), mUserId);
    if (isHidePlayer) {
        static_cast<CourseSelectPlayerActor*>(pActor)->hidePlayer();
    }

    rc::activatePlayer(pActor, port, &rTrans, nullptr);
    rc::initPlayerFigureType(
        pActor, rc::getControlUserFigureType(GameDataHolderAccessor(pActor), mUserId), false);
    startDemoWithNextNerve(nullptr, pNerve);
}

/**
 * @brief Cancels a leave in progress and releases the player.
 */
void CourseSelectPuppeteer::cancelLeave() {
    endBindOnGround();
    al::setNerve(this, &NrvCourseSelectPuppeteerWait);
}

/**
 * @brief Calculates where a player stands in front of a map object, next to the other users.
 * @param pActor Player placed.
 * @param pController Map object the player stands at, or nullptr.
 * @param pTarget Actor of the map object.
 * @param isLineUpBehindLeader Whether to line up from the first living user instead of the
 * object.
 * @return The position of the player.
 */
static sead::Vector3f calcLineUpTrans(PlayerActor* pActor,
                                      ICourseSelectActorController* pController,
                                      const al::LiveActor* pTarget, bool isLineUpBehindLeader) {
    s32 userId = rc::findControlUserId(pActor);
    s32 order = rc::calcActiveUserNumInOrder(GameDataHolderAccessor(pActor), userId);
    s32 activeNum = rc::getActiveControlUserNum(GameDataHolderAccessor(pActor));
    f32 frontOffset = CourseSelectConst::getPlayerOffsetMiniature();
    if (pController != nullptr) {
        const CourseSelectActorInfo* pInfo = pController->getCourseSelectActorInfo();
        if (pInfo != nullptr && GameDataFunction::isStageKoopaCastle(GameDataHolderAccessor(pActor),
                                                                      pInfo->getCourseId())) {
            frontOffset = CourseSelectConst::getPlayerOffsetKoopa();
        }
    }

    sead::Vector3f sideDir(1.0f, 0.0f, -0.0f);
    sideDir.normalize();
    f32 sideOffset = CourseSelectConst::getPlayerOffsetSide();

    sead::Vector3f trans;
    f32 lineOffset;
    if (isLineUpBehindLeader) {
        s32 leaderId = 0;
        for (s32 i = 0; i < rc::getControlUserNumMax(); i++) {
            if (rc::isActiveControlUser(GameDataHolderAccessor(pActor), i) &&
                !rc::isDeadControlUserInStage(GameDataHolderAccessor(pActor), i)) {
                leaderId = i;
                break;
            }
        }

        s32 port = rc::getControlUserPortNumber(GameDataHolderAccessor(pActor), leaderId);
        trans = al::getTrans(rc::tryFindPlayerFromInputPort(pActor, port, false));
        lineOffset =
            rc::calcActiveUserNumInOrder(GameDataHolderAccessor(pActor), leaderId) *
            CourseSelectConst::getPlayerOffsetSide();
    } else {
        lineOffset = (activeNum - 1) * 0.5f * sideOffset;
        trans = al::getTrans(pTarget) + sead::Vector3f(0.0f, 0.0f, 1.0f) * frontOffset;
    }

    sead::Vector3f hitPos;
    sead::Vector3f arrowStart(trans.x, trans.y + 100.0f, trans.z);
    if (alCollisionUtil::getFirstPolyOnArrow(pActor, &hitPos, nullptr, arrowStart,
                                             sead::Vector3f(0.0f, -200.0f, 0.0f), nullptr,
                                             nullptr)) {
        trans.y = hitPos.y;
    }

    trans -= sideDir * lineOffset;
    trans += sideDir * (order * CourseSelectConst::getPlayerOffsetSide());
    return trans;
}

/**
 * @brief Revives the player of the user next to a map object, delayed by the dead users before it.
 * @param pController Map object the player revives at.
 * @param isLineUpBehindLeader Whether to line up from the first living user.
 */
void CourseSelectPuppeteer::reviveUser(ICourseSelectActorController* pController,
                                       bool isLineUpBehindLeader) {
    mController = pController;
    sead::Vector3f trans = calcLineUpTrans(mActor, pController, pController->getActor(),
                                           isLineUpBehindLeader);
    activateUser(&NrvCourseSelectPuppeteerReviveUserDelay, true, trans);

    mReviveDelay = 0;
    s32 deadNum = 0;
    for (s32 i = 0; i < rc::getControlUserNumMax(); i++) {
        if (i == mUserId) {
            mReviveDelay = deadNum * 120;
            return;
        }

        deadNum += rc::isDeadControlUserInStage(GameDataHolderAccessor(mActor), i);
    }
}

/**
 * @brief Checks if a control user plays and is alive in the stage.
 * @param accessor Game data accessor.
 * @param userId Control user checked.
 * @return Whether the user is active and alive.
 */
static bool isAliveControlUser(GameDataHolderAccessor accessor, s32 userId) {
    return rc::isActiveControlUser(accessor, userId) &&
           !rc::isDeadControlUserInStage(accessor, userId);
}

/**
 * @brief Makes the player of a joining user appear next to a living player, on a free side.
 */
void CourseSelectPuppeteer::enterUser() {
    CourseSelectDirector* pDirector = CourseSelectDirector::getCourseSelectDirector(mGroup);
    al::LiveActor* pNearPlayer = nullptr;
    bool isNextUser = false;
    for (s32 i = 0; i < rc::getControlUserNumMax(); i++) {
        if (pNearPlayer != nullptr && mUserId <= i) {
            isNextUser = i == mUserId;
            break;
        }

        al::LiveActor* pPlayer =
            rc::tryFindActivePlayerActorFirstByUserId(pDirector->getMainPlayer(), i);
        if (pPlayer != nullptr && isAliveControlUser(GameDataHolderAccessor(pPlayer), i)) {
            pNearPlayer = pPlayer;
        }
    }

    const sead::Vector3f& nearPlayerTrans = al::getTrans(pNearPlayer);
    sead::Vector3f trans = nearPlayerTrans;
    {
        sead::Vector3f arrowStart = nearPlayerTrans;
        arrowStart.y += 50.0f;
        sead::Vector3f hitPos;
        if (alCollisionUtil::getFirstPolyOnArrow(pNearPlayer, &hitPos, nullptr, arrowStart,
                                                 sead::Vector3f(0.0f, -200.0f, 0.0f), nullptr,
                                                 nullptr)) {
            trans.y = hitPos.y;
        }
    }

    const sead::Vector3f& nearestTrans = al::getTrans(pDirector->tryFindNearestActor());
    sead::Vector3f toNearest(nearestTrans.x - trans.x, 0.0f, nearestTrans.z - trans.z);
    f32 distanceSq = toNearest.x * toNearest.x + toNearest.z * toNearest.z;
    f32 side = CourseSelectConst::getPlayerOffsetSide();
    if (!isNextUser) {
        side = -side;
    }

    sead::Vector3f start = trans;
    sead::Vector3f offsets[4] = {{side, 0.0f, 0.0f},
                                 {0.0f, 0.0f, -side},
                                 {0.0f, 0.0f, side},
                                 {-side, 0.0f, 0.0f}};

    s32 nearIndex = -1;
    if (distanceSq < 360000.0f) {
        f32 maxDot = -1.0f;
        for (s32 i = 0; i < 4; i++) {
            f32 dot = toNearest.dot(offsets[i]);
            if (maxDot < dot) {
                maxDot = dot;
                nearIndex = i;
            }
        }
    }

    start.y += 70.0f;
    bool isFound = false;
    for (s32 i = 0; i < 4; i++) {
        if (i == nearIndex) {
            continue;
        }

        sead::Vector3f end = start + offsets[i];
        if (alCollisionUtil::getFirstPolyOnArrow(pNearPlayer, nullptr, nullptr, start, offsets[i],
                                                 nullptr, nullptr)) {
            continue;
        }

        if (alCollisionUtil::getFirstPolyOnArrow(pNearPlayer, nullptr, nullptr, end, -offsets[i],
                                                 nullptr, nullptr)) {
            continue;
        }

        if (alCollisionUtil::checkStrikeSphere(pNearPlayer, end, 30.0f, nullptr, nullptr) != 0) {
            continue;
        }

        trans += offsets[i];
        isFound = true;
        break;
    }

    if (!isFound) {
        trans.z += -10.0f;
    }

    activateUser(&NrvCourseSelectPuppeteerEnterUser, false, trans);
}

/**
 * @brief Requests the player of the user to leave once it stands on the ground.
 */
void CourseSelectPuppeteer::leaveUser() {
    mIsRequestLeave = true;
}

/**
 * @brief Checks if the user's player is joining, reviving or leaving.
 * @return Whether an entry demo is playing.
 */
bool CourseSelectPuppeteer::isPlayEntryDemo() const {
    return al::isNerve(this, &NrvCourseSelectPuppeteerReviveUserDelay) ||
           al::isNerve(this, &NrvCourseSelectPuppeteerReviveUser) ||
           al::isNerve(this, &NrvCourseSelectPuppeteerEnterUser) ||
           al::isNerve(this, &NrvCourseSelectPuppeteerLeaveUser);
}

/**
 * @brief Checks if the user's player is leaving.
 * @return Whether the leave demo is playing.
 */
bool CourseSelectPuppeteer::isPlayLeaveDemo() const {
    return al::isNerve(this, &NrvCourseSelectPuppeteerLeaveUser);
}

/**
 * @brief Sets the warp start and end actors, and tells the player a warp starts.
 * @param pWarpStart Actor the player warps from.
 * @param pWarpEnd Actor the player warps to.
 */
void CourseSelectPuppeteer::startBindWarpActor(const al::LiveActor* pWarpStart,
                                               const al::LiveActor* pWarpEnd) {
    mWarpStartActor = pWarpStart;
    mWarpEndActor = pWarpEnd;
    al::sendMsgWarpStart(rc::getPuppetSensor(getPlayerPuppet()), al::getHitSensor(pWarpStart, 0));
}

/**
 * @brief Moves the player to the exit dokan and starts coming out of it.
 */
void CourseSelectPuppeteer::startDokanWarp() {
    rc::setPuppetTrans(getPlayerPuppet(), al::getTrans(mWarpEndActor));
    al::setNerve(this, &NrvCourseSelectPuppeteerDokanPrepareOut);
}

/**
 * @brief Ends the dokan demo and releases the player.
 */
void CourseSelectPuppeteer::startDokanEnd() {
    endBindOnGround();
    al::setNerve(this, &NrvCourseSelectPuppeteerWait);
}

/**
 * @brief Checks if the player waits inside the dokan long enough to warp.
 * @return Whether the warp can start.
 */
bool CourseSelectPuppeteer::isDokanDemoWarpWait() const {
    return al::isNerve(this, &NrvCourseSelectPuppeteerDokanWarpWait) &&
           al::isGreaterEqualStep(this, 45);
}

/**
 * @brief Checks if the player came out of the dokan.
 * @return Whether the dokan demo can end.
 */
bool CourseSelectPuppeteer::isDokanDemoEndWait() const {
    return al::isNerve(this, &NrvCourseSelectPuppeteerDokanEndWait);
}

/**
 * @brief Moves the player to the landing rocket.
 */
void CourseSelectPuppeteer::startRocketDemoWarp() {
    rc::setPuppetTrans(getPlayerPuppet(), al::getTrans(mWarpEndActor));
}

/**
 * @brief Starts getting out of the rocket.
 */
void CourseSelectPuppeteer::startRocketDemoOut() {
    al::setNerve(this, &NrvCourseSelectPuppeteerRocketPrepareOut);
}

/**
 * @brief Ends the rocket demo and releases the player.
 */
void CourseSelectPuppeteer::startRocketDemoEnd() {
    endBindOnGround();
    al::setNerve(this, &NrvCourseSelectPuppeteerWait);
}

/**
 * @brief Checks if the player is inside the rocket.
 * @return Whether the rocket can launch.
 */
bool CourseSelectPuppeteer::isRocketDemoWarpWait() const {
    return al::isNerve(this, &NrvCourseSelectPuppeteerRocketLaunchWait);
}

/**
 * @brief Checks if the player got out of the rocket.
 * @return Whether the rocket demo can end.
 */
bool CourseSelectPuppeteer::isRocketDemoEndWait() const {
    return al::isNerve(this, &NrvCourseSelectPuppeteerRocketEndWait);
}

/**
 * @brief Requests the player to be bound until the bind message arrives.
 */
void CourseSelectPuppeteer::exeBindWait() {
    mActor->requestBind(al::getHitSensor(mGroup, 0), 0.0f, 0);
}

/**
 * @brief Runs the pending nerve once the landing animation ends.
 */
void CourseSelectPuppeteer::exeLanding() {
    if (rc::isPuppetActionEnd(getPlayerPuppet())) {
        al::setNerve(this, mNextNerve);
        mNextNerve = nullptr;
    }
}

/**
 * @brief Applies gravity to the player and keeps it on the floor it lands on.
 * @param pPuppeteer Puppeteer of the player.
 */
static void updateGravity(const BindPuppeteer* pPuppeteer) {
    IUsePlayerPuppet* pPuppet = pPuppeteer->getPlayerPuppet();
    sead::Vector3f velocity(0.0f, 0.0f, 0.0f);
    velocity.y = rc::getPuppetVelocity(pPuppet).y + cGravity;
    rc::setPuppetVelocity(pPuppet, velocity);
    rc::solveAirPuppet(pPuppet);
    if (rc::isOnFloorPuppet(pPuppet) && rc::getPuppetVelocity(pPuppet).y <= 0.0f) {
        velocity.y = cGravity;
        rc::setPuppetVelocity(pPuppet, velocity);
    }
}

/**
 * @brief Runs toward the course, the users one after another, then jumps in.
 */
void CourseSelectPuppeteer::exeEnterPrepare() {
    if (al::isFirstStep(this)) {
        setupJumpPrepare(mController->getActor());
    }

    s32 userId = rc::findControlUserId(mActor);
    s32 order = rc::calcActiveUserNumInOrder(GameDataHolderAccessor(mActor), userId);
    updateGravity(this);
    if (al::isGreaterEqualStep(this, order * 15)) {
        al::setNerve(this, &NrvCourseSelectPuppeteerEnter);
    }
}

/**
 * @brief Starts running toward an actor.
 * @param pTarget Actor run to.
 */
void CourseSelectPuppeteer::setupJumpPrepare(const al::LiveActor* pTarget) {
    rc::startPuppetAction(getPlayerPuppet(), "CourseSelectMoveRun");
    rc::invalidatePuppetSensors(getPlayerPuppet());
    const sead::Vector3f& targetTrans = al::getTrans(pTarget);
    sead::Vector3f front = targetTrans - rc::getPuppetTrans(getPlayerPuppet());
    front.y = 0.0f;
    front.normalize();
    rc::setPuppetFrontVec(getPlayerPuppet(), front);
}

/**
 * @brief Jumps into the course and hides the player once it falls into it.
 */
void CourseSelectPuppeteer::exeEnter() {
    if (al::isFirstStep(this)) {
        const CourseSelectActorInfo* pInfo = mController->getCourseSelectActorInfo();
        f32 height = 100.0f;
        if (pInfo != nullptr && GameDataFunction::isStageKoopaCastle(GameDataHolderAccessor(mActor),
                                                                      pInfo->getCourseId())) {
            height = pInfo->getWorldId() == 8 ? 50.0f : 100.0f;
        }

        setupJumpEnter(mController->getActor(), height);
        rc::hidePuppetSilhouette(getPlayerPuppet());
    }

    updateJumpParamEnter();
    if (al::isGreaterEqualStep(this, 80) ||
        (rc::getPuppetTrans(getPlayerPuppet()).y < mJumpEndY &&
         rc::getPuppetVelocity(getPlayerPuppet()).y < 0.0f)) {
        rc::hidePuppet(getPlayerPuppet());
        s32 userId = rc::findControlUserId(mActor);
        s32 order = rc::calcActiveUserNumInOrder(GameDataHolderAccessor(mActor), userId);
        if (order != 0) {
            al::startHitReaction(mActor, "コースインサブ");
        } else {
            al::startHitReaction(mActor, "コースイン");
        }

        al::setNerve(this, &NrvCourseSelectPuppeteerWait);
    }
}

/**
 * @brief Calculates the velocity of a jump reaching a target.
 * @param pPuppet Player jumping.
 * @param rTarget Landing position.
 * @param height Height of the jump above the lower of the start and the target.
 */
static void setJumpVelocity(IUsePlayerPuppet* pPuppet, const sead::Vector3f& rTarget,
                            f32 height) {
    f32 startY = rc::getPuppetTrans(pPuppet).y;
    f32 topY = (startY < rTarget.y ? startY : rTarget.y) + height;
    f32 riseSq = (topY - rc::getPuppetTrans(pPuppet).y) / 0.175f;
    f32 speedY;
    if (riseSq < 0.0f) {
        speedY = 40.0f;
    } else {
        speedY = sead::Mathf::sqrt(riseSq);
    }

    const sead::Vector3f& trans = rc::getPuppetTrans(pPuppet);
    sead::Vector3f front(rTarget.x - trans.x, 0.0f, rTarget.z - trans.z);
    f32 distance = sead::Mathf::sqrt(front.x * front.x + front.z * front.z);
    if (al::isNearZero(distance, 0.001f)) {
        rc::setPuppetVelocity(pPuppet, sead::Vector3f(0.0f, speedY, 0.0f));
        return;
    }

    f32 fallSq = speedY * speedY + (rTarget.y - trans.y) * (2.0f * cGravity);
    f32 speedH = 0.0f;
    if (fallSq > 0.0f) {
        f32 fallSpeed = sead::Mathf::sqrt(fallSq);
        if (!al::isNearZero(distance, 0.001f)) {
            speedH = distance / ((-speedY - fallSpeed) / cGravity);
        }
    }

    f32 invDistance = 1.0f / distance;
    front.x = invDistance * front.x;
    front.z = invDistance * front.z;
    rc::setPuppetFrontVec(pPuppet, front);
    rc::setPuppetVelocity(pPuppet, sead::Vector3f(speedH * front.x, speedY, speedH * front.z));
}

/**
 * @brief Starts jumping into an actor.
 * @param pTarget Actor jumped into.
 * @param height Height above the actor where the jump ends.
 */
void CourseSelectPuppeteer::setupJumpEnter(const al::LiveActor* pTarget, f32 height) {
    rc::startPuppetAction(getPlayerPuppet(), "DemoStageEnter");
    rc::invalidatePuppetSensors(getPlayerPuppet());
    sead::Vector3f target = al::getTrans(pTarget);
    target.y += height;
    mJumpEndY = target.y;

    f32 jumpHeight = 300.0f;
    if (mController != nullptr) {
        const CourseSelectActorInfo* pInfo = mController->getCourseSelectActorInfo();
        if (pInfo != nullptr && GameDataFunction::isStageKoopaCastle(GameDataHolderAccessor(mActor),
                                                                      pInfo->getCourseId())) {
            jumpHeight = pInfo->getWorldId() == 8 ? 200.0f : 300.0f;
        }
    }

    setJumpVelocity(getPlayerPuppet(), target, jumpHeight);
}

/**
 * @brief Moves the player along its jump into an actor.
 */
void CourseSelectPuppeteer::updateJumpParamEnter() {
    sead::Vector3f velocity = rc::getPuppetVelocity(getPlayerPuppet());
    velocity.y += cGravity;
    rc::setPuppetVelocity(getPlayerPuppet(), velocity);
    rc::moveSimplePuppet(getPlayerPuppet());
}

/**
 * @brief Waits in front of a Toad Brigade course.
 */
void CourseSelectPuppeteer::exeEnterGateKeeper() {
    if (al::isFirstStep(this)) {
        rc::startPuppetAction(getPlayerPuppet(), "CourseSelectWait");
    }

    updateGravity(this);
    if (al::isGreaterEqualStep(this, 80)) {
        al::setNerve(this, &NrvCourseSelectPuppeteerWait);
    }
}

/**
 * @brief Waits in front of a hidden course.
 */
void CourseSelectPuppeteer::exeEnterHide() {
    if (al::isFirstStep(this)) {
        rc::startPuppetAction(getPlayerPuppet(), "Wait");
    }

    if (al::isGreaterEqualStep(this, 80)) {
        al::setNerve(this, &NrvCourseSelectPuppeteerWait);
    }
}

/**
 * @brief Waits during the gate keeper event.
 */
void CourseSelectPuppeteer::exeEventGateKeeper() {
    if (al::isFirstStep(this)) {
        rc::startPuppetAction(getPlayerPuppet(), "Wait");
    }
}

/**
 * @brief Keeps the player hidden inside the course before it jumps out.
 */
void CourseSelectPuppeteer::exePrepareExit() {
    if (al::isFirstStep(this)) {
        rc::startPuppetAction(getPlayerPuppet(), "CourseSelectWait");
        rc::hidePuppet(getPlayerPuppet());
        rc::hidePuppetSilhouette(getPlayerPuppet());
    }

    if (al::isGreaterEqualStep(this, 60)) {
        al::setNerve(this, &NrvCourseSelectPuppeteerExit);
    }
}

/**
 * @brief Jumps out of the course and lands next to it.
 */
void CourseSelectPuppeteer::exeExit() {
    if (al::isFirstStep(this)) {
        rc::showPuppet(getPlayerPuppet());
        const CourseSelectActorInfo* pInfo = mController->getCourseSelectActorInfo();
        f32 height = 100.0f;
        if (pInfo != nullptr && GameDataFunction::isStageKoopaCastle(GameDataHolderAccessor(mActor),
                                                                      pInfo->getCourseId())) {
            if (pInfo->getWorldId() == 7) {
                height = 100.0f;
            } else {
                height = pInfo->getWorldId() == 8 ? 50.0f : 100.0f;
            }
        }

        setupJumpExit(mController->getActor(), height);
    }

    if (al::isStep(this, 2)) {
        al::startHitReaction(mActor, "コースアウト");
    }

    sead::Vector3f velocity = updateJumpParamExit();
    if (al::isGreaterEqualStep(this, 60) ||
        (rc::isOnFloorPuppet(getPlayerPuppet()) && velocity.y < 0.0f)) {
        rc::setPuppetFrontVec(getPlayerPuppet(), sead::Vector3f::ez);
        rc::showPuppetSilhouette(getPlayerPuppet());
        sead::Vector3f trans =
            calcLineUpTrans(mActor, mController, mController->getActor(), false);
        rc::setPuppetTrans(getPlayerPuppet(), trans);
        al::setNerve(this, &NrvCourseSelectPuppeteerBindEndLanding);
    }
}

/**
 * @brief Starts jumping out of an actor toward the player's line-up position.
 * @param pTarget Actor jumped out of.
 * @param height Height above the actor where the jump starts.
 */
void CourseSelectPuppeteer::setupJumpExit(const al::LiveActor* pTarget, f32 height) {
    const CourseSelectActorInfo* pInfo = mController->getCourseSelectActorInfo();
    f32 jumpHeight = 300.0f;
    if (pInfo != nullptr && GameDataFunction::isStageKoopaCastle(GameDataHolderAccessor(mActor),
                                                                  pInfo->getCourseId())) {
        if (pInfo->getWorldId() == 7) {
            jumpHeight = 400.0f;
        } else {
            jumpHeight = pInfo->getWorldId() == 8 ? 200.0f : 400.0f;
        }
    }

    sead::Vector3f target = calcLineUpTrans(mActor, mController, pTarget, false);
    rc::invalidatePuppetSensors(getPlayerPuppet());
    rc::setPuppetUpVec(getPlayerPuppet(), sead::Vector3f::ey);
    sead::Vector3f start = al::getTrans(pTarget);
    start.y += height;
    rc::setPuppetTrans(getPlayerPuppet(), start);
    rc::startPuppetAction(getPlayerPuppet(), "DemoStageExit");
    setJumpVelocity(getPlayerPuppet(), target, jumpHeight);
}

/**
 * @brief Moves the player along its jump out of an actor.
 * @return The velocity of the player.
 */
sead::Vector3f CourseSelectPuppeteer::updateJumpParamExit() {
    sead::Vector3f velocity = rc::getPuppetVelocity(getPlayerPuppet());
    velocity.y += cGravity;
    rc::setPuppetVelocity(getPlayerPuppet(), velocity);
    rc::solveAirPuppet(getPlayerPuppet());
    return velocity;
}

/**
 * @brief Places the player in front of a Toad Brigade course, facing it, then releases it.
 */
void CourseSelectPuppeteer::exeExitKinopioBrigade() {
    if (al::isFirstStep(this)) {
        sead::Vector3f trans =
            calcLineUpTrans(mActor, mController, mController->getActor(), false);
        const sead::Vector3f& courseTrans = al::getTrans(mController->getActor());
        sead::Vector3f front(courseTrans.x - trans.x, 0.0f, courseTrans.z - trans.z);
        al::normalizeOrDirZ(&front);
        rc::setPuppetTrans(getPlayerPuppet(), trans);
        rc::setPuppetFrontVec(getPlayerPuppet(), front);
        rc::startPuppetAction(getPlayerPuppet(), "CourseSelectWait");
    }

    if (al::isGreaterEqualStep(this, 180)) {
        endBindOnGround();
        al::setNerve(this, &NrvCourseSelectPuppeteerWait);
    }
}

/**
 * @brief Waits while a road opens.
 */
void CourseSelectPuppeteer::exeOpenRoad() {
    if (al::isFirstStep(this)) {
        rc::startPuppetAction(getPlayerPuppet(), "CourseSelectWait");
    }
}

/**
 * @brief Opens a lock: the main player shows the lock actor, then the player is released once
 * both animations end.
 */
void CourseSelectPuppeteer::exeUnLock() {
    CourseSelectDirector* pDirector = CourseSelectDirector::getCourseSelectDirector(mGroup);
    al::LiveActor* pLock = pDirector->getUnLockActor();
    if (al::isFirstStep(this)) {
        if (mActor == pDirector->getMainPlayer()) {
            pLock->appear();
            al::hideShadow(pLock);
            al::setTrans(pLock, al::getTrans(mActor));
            al::startAction(pLock, "CourseSelectOpenLock");
        }

        rc::startPuppetAction(getPlayerPuppet(), "CourseSelectOpenLock");
        rc::setPuppetVelocity(getPlayerPuppet(), sead::Vector3f::zero);
    }

    updateGravity(this);
    if (rc::isPuppetActionEnd(getPlayerPuppet()) &&
        (al::isDead(pLock) || al::isActionEnd(pLock))) {
        if (!al::isDead(pLock)) {
            pLock->kill();
        }

        endBindOnGround();
        al::setNerve(this, &NrvCourseSelectPuppeteerWait);
    }
}

/**
 * @brief Gets surprised by an appearing lock.
 */
void CourseSelectPuppeteer::exeLockAppear() {
    if (al::isStep(this, 24)) {
        rc::startPuppetAction(getPlayerPuppet(), "CourseSelectSurprise");
    }

    if (rc::isPuppetActionEnd(getPlayerPuppet())) {
        al::setNerve(this, &NrvCourseSelectPuppeteerWaitDemo);
    }
}

/**
 * @brief Gets surprised by an opening gate keeper.
 */
void CourseSelectPuppeteer::exeOpenGateKeeper() {
    if (al::isFirstStep(this)) {
        rc::startPuppetAction(getPlayerPuppet(), "CourseSelectSurprise");
    }

    if (rc::isPuppetActionEnd(getPlayerPuppet())) {
        al::setNerve(this, &NrvCourseSelectPuppeteerWaitDemo);
    }
}

/**
 * @brief Runs toward the dokan, the users one after another.
 */
void CourseSelectPuppeteer::exeDokanPrepareIn() {
    if (al::isFirstStep(this)) {
        setupJumpPrepare(mWarpStartActor);
    }

    s32 userId = rc::findControlUserId(mActor);
    s32 order = rc::calcActiveUserNumInOrder(GameDataHolderAccessor(mActor), userId);
    updateGravity(this);
    if (al::isGreaterEqualStep(this, order * 15)) {
        al::setNerve(this, &NrvCourseSelectPuppeteerDokanIn);
    }
}

/**
 * @brief Jumps into the dokan.
 */
void CourseSelectPuppeteer::exeDokanIn() {
    if (al::isFirstStep(this)) {
        setupJumpEnter(mWarpStartActor, 70.0f);
        rc::hidePuppetSilhouette(getPlayerPuppet());
    }

    updateJumpParamEnter();
    if (al::isGreaterEqualStep(this, 80) ||
        (rc::getPuppetTrans(getPlayerPuppet()).y < mJumpEndY &&
         rc::getPuppetVelocity(getPlayerPuppet()).y < 0.0f)) {
        rc::hidePuppet(getPlayerPuppet());
        al::setNerve(this, &NrvCourseSelectPuppeteerDokanWarpWait);
        rc::startPuppetSe(getPlayerPuppet(), "PgCourseSelectDokanIn");
    }
}

/**
 * @brief Waits inside the dokan until the warp starts.
 */
void CourseSelectPuppeteer::exeDokanWarpWait() {}

/**
 * @brief Waits inside the exit dokan, the users one after another.
 */
void CourseSelectPuppeteer::exeDokanPrepareOut() {
    s32 userId = rc::findControlUserId(mActor);
    s32 order = rc::calcActiveUserNumInOrder(GameDataHolderAccessor(mActor), userId);
    if (al::isStep(this, 50)) {
        rc::startPuppetSe(getPlayerPuppet(), "PgCourseSelectDokanOut");
    }

    if (al::isGreaterEqualStep(this, order * 15 + 90)) {
        al::setNerve(this, &NrvCourseSelectPuppeteerDokanOut);
        rc::startPuppetSe(getPlayerPuppet(), "PgCourseSelectEachDokanOut");
    }
}

/**
 * @brief Jumps out of the exit dokan and lands.
 */
void CourseSelectPuppeteer::exeDokanOut() {
    if (al::isFirstStep(this)) {
        rc::showPuppet(getPlayerPuppet());
        rc::hidePuppetSilhouette(getPlayerPuppet());
        setupJumpExit(mWarpEndActor, 30.0f);
    }

    if (al::isStep(this, 2)) {
        al::startHitReaction(mActor, "コースアウト");
    }

    sead::Vector3f velocity = updateJumpParamExit();
    if (al::isGreaterEqualStep(this, 60) ||
        (rc::isOnFloorPuppet(getPlayerPuppet()) && velocity.y < 0.0f)) {
        rc::setPuppetFrontVec(getPlayerPuppet(), sead::Vector3f::ez);
        rc::showPuppetSilhouette(getPlayerPuppet());
        al::sendMsgWarpEnd(rc::getPuppetSensor(getPlayerPuppet()),
                           al::getHitSensor(mWarpStartActor, 0));
        startLandingWithNextNerve(&NrvCourseSelectPuppeteerDokanEndWait);
    }
}

/**
 * @brief Waits until the dokan demo ends.
 */
void CourseSelectPuppeteer::exeDokanEndWait() {}

/**
 * @brief Places the player on a rail, facing along it.
 * @param pPuppet Player placed.
 * @param rTrans Position on the rail.
 * @param rDir Direction of the rail.
 */
static void setPuppetPoseOnRail(IUsePlayerPuppet* pPuppet, const sead::Vector3f& rTrans,
                                const sead::Vector3f& rDir) {
    sead::Vector3f front;
    front.setRotated(sead::Quatf::unit, sead::Vector3f::ez);
    sead::Vector3f dir;
    sead::Quatf rotation;
    sead::Quatf quat(1.0f, 0.0f, 0.0f, 0.0f);
    if (al::isReverseDirection(front, rDir, 0.01f)) {
        quat.setAxisAngle(sead::Vector3f::ex, 180.0f);
    } else {
        al::normalizeOrZero(&front);
        al::normalizeOrZero(&dir, rDir);
        al::makeQuatRotationRate(&rotation, front, dir, 1.0f);
        quat = rotation * sead::Quatf::unit;
        quat.normalize();
    }

    rc::setPuppetQuat(pPuppet, quat);
    rc::setPuppetTrans(pPuppet, rTrans);
}

/**
 * @brief Enters the world warp dokan, the users one after another.
 */
void CourseSelectPuppeteer::exeWorldWarpDokanStart() {
    if (al::isFirstStep(this)) {
        rc::startPuppetSe(getPlayerPuppet(), "PgCourseSelectRouteDokanIn");
        rc::startPuppetAction(getPlayerPuppet(), "RouteDokanMove");
        rc::invalidatePuppetSensors(getPlayerPuppet());
        al::LiveActor* pRailActor = mController->getActor();
        al::setRailPosToStart(pRailActor);
        const sead::Vector3f& railTrans = al::getRailPos(pRailActor);
        const sead::Vector3f& railDir = al::getRailDir(pRailActor);
        setPuppetPoseOnRail(getPlayerPuppet(), railTrans, railDir);
    }

    s32 userId = rc::findControlUserId(mActor);
    s32 userIds[4];
    s32 userNum = rc::findActiveUserIdList(userIds, GameDataHolderAccessor(mActor));
    s32 aliveNum = 0;
    for (s32 i = 0; i < userNum; i++) {
        if (userIds[i] == userId) {
            break;
        }

        aliveNum += !rc::isDeadControlUserInStage(GameDataHolderAccessor(mActor), userIds[i]);
    }

    if (al::isGreaterEqualStep(this, aliveNum * 15)) {
        al::setNerve(this, &NrvCourseSelectPuppeteerWorldWarpDokanMove);
    }
}

/**
 * @brief Moves the player along the world warp dokan rail.
 */
void CourseSelectPuppeteer::exeWorldWarpDokanMove() {
    al::LiveActor* pRailActor = mController->getActor();
    f32 coord = al::getNerveStep(this) * 20.0f;
    bool isEnd = false;
    if (al::getRailTotalLength(pRailActor) <= coord) {
        coord = al::getRailTotalLength(pRailActor);
        isEnd = true;
    }

    al::setRailPosToCoord(pRailActor, coord);
    const sead::Vector3f& railTrans = al::getRailPos(pRailActor);
    const sead::Vector3f& railDir = al::getRailDir(pRailActor);
    setPuppetPoseOnRail(getPlayerPuppet(), railTrans, railDir);
    if (isEnd) {
        al::setNerve(this, &NrvCourseSelectPuppeteerWorldWarpDokanOut);
        rc::startPuppetSe(getPlayerPuppet(), "PgCourseSelectDokanOut");
        rc::startPuppetSe(getPlayerPuppet(), "PgCourseSelectEachDokanOut");
    }
}

/**
 * @brief Jumps out of the world warp dokan like out of a course.
 */
void CourseSelectPuppeteer::exeWorldWarpDokanOut() {
    exeExit();
}

/**
 * @brief Waits while the rocket breaks.
 */
void CourseSelectPuppeteer::exeRocketBreakDemo() {
    if (al::isFirstStep(this)) {
        rc::startPuppetAction(getPlayerPuppet(), "CourseSelectWait");
    }

    updateGravity(this);
}

/**
 * @brief Runs toward the rocket, the users one after another.
 */
void CourseSelectPuppeteer::exeRocketPrepareIn() {
    if (al::isFirstStep(this)) {
        setupJumpPrepare(mWarpStartActor);
    }

    s32 userId = rc::findControlUserId(mActor);
    s32 order = rc::calcActiveUserNumInOrder(GameDataHolderAccessor(mActor), userId);
    updateGravity(this);
    if (al::isGreaterEqualStep(this, order * 15)) {
        al::setNerve(this, &NrvCourseSelectPuppeteerRocketIn);
    }
}

/**
 * @brief Jumps into the rocket.
 */
void CourseSelectPuppeteer::exeRocketIn() {
    if (al::isFirstStep(this)) {
        setupJumpEnter(mWarpStartActor, 100.0f);
        rc::hidePuppetSilhouette(getPlayerPuppet());
    }

    updateJumpParamEnter();
    if (al::isGreaterEqualStep(this, 80) ||
        (rc::getPuppetTrans(getPlayerPuppet()).y < mJumpEndY &&
         rc::getPuppetVelocity(getPlayerPuppet()).y < 0.0f)) {
        rc::hidePuppet(getPlayerPuppet());
        al::setNerve(this, &NrvCourseSelectPuppeteerRocketLaunchWait);
    }
}

/**
 * @brief Waits inside the rocket until it launches.
 */
void CourseSelectPuppeteer::exeRocketLaunchWait() {}

/**
 * @brief Waits inside the landed rocket, the users one after another.
 */
void CourseSelectPuppeteer::exeRocketPrepareOut() {
    s32 userId = rc::findControlUserId(mActor);
    s32 order = rc::calcActiveUserNumInOrder(GameDataHolderAccessor(mActor), userId);
    if (al::isGreaterEqualStep(this, order * 15 + 45)) {
        al::setNerve(this, &NrvCourseSelectPuppeteerRocketOut);
    }
}

/**
 * @brief Jumps out of the rocket and lands.
 */
void CourseSelectPuppeteer::exeRocketOut() {
    if (al::isFirstStep(this)) {
        rc::showPuppet(getPlayerPuppet());
        setupJumpExit(mWarpEndActor, 100.0f);
        rc::hidePuppetSilhouette(getPlayerPuppet());
    }

    if (al::isStep(this, 2)) {
        al::startHitReaction(mActor, "コースアウト");
    }

    sead::Vector3f velocity = updateJumpParamExit();
    if (al::isGreaterEqualStep(this, 60) ||
        (rc::isOnFloorPuppet(getPlayerPuppet()) && velocity.y < 0.0f)) {
        rc::setPuppetFrontVec(getPlayerPuppet(), sead::Vector3f::ez);
        rc::showPuppetSilhouette(getPlayerPuppet());
        al::sendMsgWarpEnd(rc::getPuppetSensor(getPlayerPuppet()),
                           al::getHitSensor(mWarpStartActor, 0));
        startLandingWithNextNerve(&NrvCourseSelectPuppeteerRocketEndWait);
    }
}

/**
 * @brief Waits until the rocket demo ends.
 */
void CourseSelectPuppeteer::exeRocketEndWait() {}

/**
 * @brief Moves the player into the route dokan entrance, the users one after another.
 */
void CourseSelectPuppeteer::exeRouteDokanMoveStart() {
    if (al::isFirstStep(this)) {
        rc::calcPuppetQuat(&mRouteStartQuat, getPlayerPuppet());
        mRouteStartTrans = rc::getPuppetTrans(getPlayerPuppet());
        sead::Vector3f dir;
        mRailRider->calcPosAndDir(&mRouteEndTrans, &dir);
        al::makeQuatFrontUp(&mRouteEndQuat, dir, sead::Vector3f::ey);
    }

    s32 userId = rc::findControlUserId(mActor);
    s32 frame = rc::calcActiveUserNumInOrder(GameDataHolderAccessor(mActor), userId) * 13;
    f32 rate = al::calcNerveRate(this, frame - 8, frame);
    sead::Vector3f trans;
    al::lerpVec(&trans, mRouteStartTrans, mRouteEndTrans, rate);
    sead::Quatf quat;
    al::slerpQuat(&quat, mRouteStartQuat, mRouteEndQuat, rate);
    rc::setPuppetQuat(getPlayerPuppet(), quat);
    rc::setPuppetTrans(getPlayerPuppet(), trans);
    if (al::isGreaterEqualStep(this, frame)) {
        al::setNerve(this, &NrvCourseSelectPuppeteerRouteDokanMove);
    }
}

/**
 * @brief Moves the player along the route dokan rail until its end.
 */
void CourseSelectPuppeteer::exeRouteDokanMove() {
    if (al::isFirstStep(this)) {
        rc::startPuppetSe(getPlayerPuppet(), "PgCourseSelectRouteDokanIn");
        rc::startPuppetAction(getPlayerPuppet(), "RouteDokanMove");
        sead::Vector3f trans;
        sead::Vector3f dir;
        mRailRider->calcPosAndDir(&trans, &dir);
        mDokanEffect->startIn(trans, dir);
    }

    sead::Quatf quat;
    rc::calcPuppetQuat(&quat, getPlayerPuppet());
    sead::Vector3f trans;
    sead::Vector3f dir;
    mRailRider->move(20.0f, &trans, &dir);
    if (!al::isParallelDirection(dir, sead::Vector3f::ey, 0.01f)) {
        sead::Vector3f up;
        al::calcQuatUp(&up, quat);
        if (al::isReverseDirection(up, sead::Vector3f::ey, 0.01f)) {
            al::rotateQuatRadian(&quat, quat, dir, 0.17453292f);
        } else {
            al::turnQuatYDirRadian(&quat, quat, sead::Vector3f::ey, 0.17453292f);
        }
    }

    al::turnQuatZDirRate(&quat, quat, dir, 1.0f);
    rc::setPuppetQuat(getPlayerPuppet(), quat);
    rc::setPuppetTrans(getPlayerPuppet(), trans);
    if (mRailRider->isReachEnd()) {
        mDokanEffect->startOut(trans, dir);
        al::setNerve(this, &NrvCourseSelectPuppeteerRouteDokanMoveEnd);
    }
}

/**
 * @brief Pushes the player out of the route dokan with a jump and releases it.
 */
void CourseSelectPuppeteer::exeRouteDokanMoveEnd() {
    sead::Vector3f front = rc::getPuppetFrontVec(getPlayerPuppet());
    sead::Vector3f trans = rc::getPuppetTrans(getPlayerPuppet());
    trans += front * 20.0f;
    rc::setPuppetTrans(getPlayerPuppet(), trans);
    if (!al::isGreaterEqualStep(this, 1)) {
        return;
    }

    s32 userId = rc::findControlUserId(mActor);
    s32 order = rc::calcActiveUserNumInOrder(GameDataHolderAccessor(mActor), userId);
    s32 activeNum = rc::getActiveControlUserNum(GameDataHolderAccessor(mActor));
    f32 followerNum = activeNum - order - 1;
    CourseSelectDirector* pDirector = CourseSelectDirector::getCourseSelectDirector(mGroup);
    f32 speed = followerNum * 9.0f + 2.0f;
    if (pDirector->getActiveWorldId() == 8 &&
        !GameDataFlagFunction::isShowWorldStartDemo(GameDataHolderAccessor(mActor), 8)) {
        speed = ((activeNum - 1) * 0.5f - order) * 12.0f + 22.0f;
    }

    sead::Vector3f velocity = front * speed;
    rc::setPuppetVelocity(getPlayerPuppet(), velocity);
    rc::startPuppetSe(getPlayerPuppet(), "RouteDokanOut");
    rc::startPuppetAction(getPlayerPuppet(), "Jump");
    endBind(&sRouteDokanOutEndParam);
    al::setNerve(this, &NrvCourseSelectPuppeteerWait);
}

/**
 * @brief Warps the player straight to its line-up position at a course.
 */
void CourseSelectPuppeteer::exeWarpToCourse() {
    al::requestCancelInterpole(mActor);
    if (al::isFirstStep(this)) {
        al::sendMsgWarpStart(rc::getPuppetSensor(getPlayerPuppet()),
                             al::getHitSensor(mController->getActor(), 0));
        sead::Vector3f trans =
            calcLineUpTrans(mActor, mController, mController->getActor(), false);
        rc::setPuppetTrans(getPlayerPuppet(), trans);
        rc::setPuppetFrontVec(getPlayerPuppet(), sead::Vector3f::ez);
        rc::setPuppetUpVec(getPlayerPuppet(), sead::Vector3f::ey);
    } else if (al::isStep(this, 1)) {
        al::sendMsgWarpEnd(rc::getPuppetSensor(getPlayerPuppet()),
                           al::getHitSensor(mController->getActor(), 0));
        endBindOnGround();
        al::setNerve(this, &NrvCourseSelectPuppeteerWait);
    }
}

/**
 * @brief Plays the world start animation, then releases the player.
 */
void CourseSelectPuppeteer::exeWorldStartDemo() {
    if (al::isFirstStep(this)) {
        rc::startPuppetAction(getPlayerPuppet(), "DemoWorldStart");
    }

    if (rc::isPuppetActionEnd(getPlayerPuppet())) {
        endBindOnGround();
        al::setNerve(this, &NrvCourseSelectPuppeteerWait);
    }
}

/**
 * @brief Plays the world 8 start animation, turning the player at first, then releases it.
 */
void CourseSelectPuppeteer::exeWorldStartDemoW8() {
    if (al::isFirstStep(this)) {
        rc::startPuppetAction(getPlayerPuppet(), "CourseSelectW8StartDemo");
    }

    if (al::isLessEqualStep(this, 30)) {
        sead::Quatf quat;
        rc::calcPuppetQuat(&quat, getPlayerPuppet());
        al::rotateQuatRadian(&quat, quat, sead::Vector3f::ey, 0.05235988f);
        rc::setPuppetQuat(getPlayerPuppet(), quat);
    }

    if (rc::isPuppetActionEnd(getPlayerPuppet())) {
        endBindOnGround();
        al::setNerve(this, &NrvCourseSelectPuppeteerWait);
    }
}

/**
 * @brief Hides the player.
 */
void CourseSelectPuppeteer::exeHidePlayerDemo() {
    if (al::isFirstStep(this)) {
        rc::hidePuppet(getPlayerPuppet());
    }
}

/**
 * @brief Makes the player wait.
 */
void CourseSelectPuppeteer::exeWaitDemo() {
    if (al::isFirstStep(this)) {
        rc::startPuppetAction(getPlayerPuppet(), "CourseSelectWait");
    }
}

/**
 * @brief Waits for the dead users before this one, then starts the revival.
 */
void CourseSelectPuppeteer::exeReviveUserDelay() {
    // The first step of a delayed revival does nothing (likely stripped debug code).
    if (mReviveDelay != 0 && al::isFirstStep(this)) {
    }

    if (al::isStep(this, mReviveDelay)) {
        al::startHitReaction(mActor, "コース選択復活");
        return;
    }

    if (al::isGreaterEqualStep(this, mReviveDelay + 60)) {
        al::setNerve(this, &NrvCourseSelectPuppeteerReviveUser);
    }
}

/**
 * @brief Makes the revived player pop up, then releases it.
 */
void CourseSelectPuppeteer::exeReviveUser() {
    if (al::isFirstStep(this)) {
        auto* pPlayer = static_cast<CourseSelectPlayerActor*>(
            rc::findPlayerActorFirstByUserId(mActor, mUserId));
        pPlayer->showPlayer();
        al::startSe(pPlayer, "PgCsComeBack");
        rc::setPuppetVelocity(getPlayerPuppet(), sead::Vector3f(0.0f, 20.0f, 0.0f));
        rc::startPuppetAction(getPlayerPuppet(), "CourseSelectAppear");
        PlayerCrown* pCrown =
            CourseSelectDirector::getCourseSelectDirector(mGroup)->getPlayerCrown();
        s32 bestScoreUserId =
            GameDataFunction::tryGetLastStageBestScoreUserID(GameDataHolderAccessor(mActor));
        if (mUserId == bestScoreUserId) {
            pCrown->changeHost(mActor);
            pCrown->appear();
        }
    }

    updateGravity(this);
    if (al::isStep(this, 8)) {
        ScoreFunction::popUpPlayerUp(mActor, 5, 100.0f);
    }

    if (rc::isPuppetActionEnd(getPlayerPuppet()) && al::isGreaterEqualStep(this, 8) &&
        al::isGreaterEqualStep(this, 70)) {
        endBindOnGround();
        al::setNerve(this, &NrvCourseSelectPuppeteerWait);
    }
}

/**
 * @brief Makes the joining player pop up, then releases it.
 */
void CourseSelectPuppeteer::exeEnterUser() {
    if (al::isFirstStep(this)) {
        if (rc::isPuppetHidden(getPlayerPuppet())) {
            rc::showPuppet(getPlayerPuppet());
            rc::showPuppetSilhouette(getPlayerPuppet());
        }

        rc::setPuppetVelocity(getPlayerPuppet(), sead::Vector3f(0.0f, 20.0f, 0.0f));
        rc::startPuppetAction(getPlayerPuppet(), "CourseSelectAppear");
        PlayerCrown* pCrown =
            CourseSelectDirector::getCourseSelectDirector(mGroup)->getPlayerCrown();
        s32 bestScoreUserId =
            GameDataFunction::tryGetLastStageBestScoreUserID(GameDataHolderAccessor(mActor));
        if (mUserId == bestScoreUserId) {
            pCrown->changeHost(mActor);
            pCrown->appear();
        }
    }

    updateGravity(this);
    if (al::isNerve(this, &NrvCourseSelectPuppeteerReviveUser) && al::isStep(this, 8)) {
        ScoreFunction::popUpPlayerUp(mActor, 5, 100.0f);
    }

    if (!rc::isPuppetActionEnd(getPlayerPuppet())) {
        return;
    }

    if (al::isNerve(this, &NrvCourseSelectPuppeteerReviveUser) && al::isLessEqualStep(this, 8)) {
        return;
    }

    endBindOnGround();
    al::setNerve(this, &NrvCourseSelectPuppeteerWait);
}

namespace Local {
/**
 * @brief Turns a direction toward another one by at most an angle.
 * @param pOut Turned direction.
 * @param rVec Direction turned.
 * @param rTarget Direction turned to.
 * @param degree Maximum angle in degrees.
 * @return Whether a rotation could be made.
 */
bool turnVecToVecDegree(sead::Vector3f* pOut, const sead::Vector3f& rVec,
                        const sead::Vector3f& rTarget, f32 degree) {
    sead::Quatf quat;
    bool isRotated = sead::QuatCalcCommon<f32>::makeVectorRotationLimit(
        quat, rVec, rTarget, sead::Mathf::deg2rad(degree));
    pOut->setRotated(quat, rVec);
    al::normalize(pOut);
    return isRotated;
}
}  // namespace Local

/**
 * @brief Turns the leaving player to the back, then retires its user (or only stops the main
 * player when it is the last user).
 */
void CourseSelectPuppeteer::exeLeaveUser() {
    if (al::isFirstStep(this)) {
        rc::startPuppetAction(getPlayerPuppet(), "CourseSelectLeave");
    }

    sead::Vector3f front = rc::getPuppetFrontVec(getPlayerPuppet());
    if (al::isNearZero(front.dot(sead::Vector3f::ez) + 1.0f, 0.001f)) {
        Local::turnVecToVecDegree(&front, front, sead::Vector3f::ex, 25.0f);
    } else {
        Local::turnVecToVecDegree(&front, front, sead::Vector3f::ez, 25.0f);
    }

    rc::setPuppetFrontVec(getPlayerPuppet(), front);
    if (!rc::isPuppetActionEnd(getPlayerPuppet())) {
        return;
    }

    endBindOnGround();
    PlayerCrown* pCrown = CourseSelectDirector::getCourseSelectDirector(mGroup)->getPlayerCrown();
    if (pCrown->getHost() == mActor) {
        pCrown->kill();
    }

    if (rc::getActiveControlUserNum(GameDataHolderAccessor(mActor)) >= 2) {
        PlayerEntryFunction::retirePlayer(GameDataHolderWriter(mActor), mUserId);
        al::LiveActor* pPlayer = rc::findPlayerActorFirstByUserId(mActor, mUserId);
        al::startHitReaction(pPlayer, "退出");
        rc::deactivatePlayer(static_cast<PlayerActor*>(pPlayer));
    } else {
        al::onAreaTarget(rc::findPlayerActorFirstByUserId(mActor, mUserId));
    }

    mActor = nullptr;
    al::setNerve(this, &NrvCourseSelectPuppeteerWait);
}

/**
 * @brief Plays the landing animation after a bind, then releases the player.
 */
void CourseSelectPuppeteer::exeBindEndLanding() {
    if (al::isFirstStep(this)) {
        rc::startPuppetAction(getPlayerPuppet(), "CourseSelectLand");
    }

    if (rc::isPuppetActionEnd(getPlayerPuppet())) {
        endBindOnGround();
        al::setNerve(this, &NrvCourseSelectPuppeteerWait);
    }
}

/**
 * @brief Does nothing once the bind ended.
 */
void CourseSelectPuppeteer::exeBindEnd() {}

/**
 * @brief Destroys the puppeteer.
 */
CourseSelectPuppeteer::~CourseSelectPuppeteer() = default;
