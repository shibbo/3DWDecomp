#include "MapObj/GoalPole.hpp"

#include <math/seadMathCalcCommon.h>

#include "Library/Audio/System/AudioKeeperFunction.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/Collision/CollisionPartsKeeperUtil.hpp"
#include "Library/LiveActor/Common/LiveActorGroup.hpp"
#include "Library/LiveActor/SubActorUtil.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorClippingUtil.hpp"
#include "Library/LiveActor/Util/ActorFlagUtil.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorModelUtil.hpp"
#include "Library/LiveActor/Util/ActorMovementUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorSceneUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Play/Layout/SimpleLayoutAppearWait.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Play/Placement/PlacementId.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Shadow/Common/ShadowUtil.hpp"
#include "Library/StageSwitch/StageSwitchFunc.hpp"
#include "Library/Thread/Functor.hpp"
#include "Project/AreaObj/AreaObjUtil.hpp"
#include "Project/Bgm/BgmPlayingRequest.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Collision/CollisionPartsTriangle.hpp"
#include "Project/Collision/CollisionUtil.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"

#include "MapObj/ActorStateDemoCamera.hpp"
#include "MapObj/BindPuppeteerGroup.hpp"
#include "MapObj/GoalPoleBindPuppeteer.hpp"
#include "MapObj/GoalPoleFlag.hpp"
#include "MapObj/GoalPoleStateRunaway.hpp"
#include "NPC/FairyPrincess.hpp"
#include "NPC/GhostPlayerRecorder.hpp"
#include "NPC/RosettaNpc.hpp"
#include "Scene/PlayerStocker.hpp"
#include "System/CourseInfoHolder.hpp"
#include "System/GameDataFlagFunction.hpp"
#include "System/GameDataFunction.hpp"
#include "System/StageTimer.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/DemoUtil.hpp"
#include "Util/GhostPlayerUtil.hpp"
#include "Util/LayoutUtil.hpp"
#include "Util/PlayerPuppetUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ScoreUtil.hpp"

namespace {
NERVE_DECL(GoalPole, Runaway)
NERVE_DECL(GoalPole, Wait)
NERVE_DECL(GoalPole, FairyFocusDemo)
NERVE_DECL(GoalPole, WaitGoalDemo)
NERVE_DECL(GoalPole, GoalDemoCatch)
NERVE_DECL(GoalPole, GoalDemoFall)
NERVE_DECL(GoalPole, GoalDemoJump)
NERVE_DECL(GoalPole, GoalDemoFireworks)
NERVE_DECL(GoalPole, EndingPrevDemoRequest)
NERVE_DECL(GoalPole, EndingPrevDemo)
NERVES_MAKE_NOSTRUCT(GoalPole, EndingPrevDemoRequest, EndingPrevDemo)
NERVES_MAKE_STRUCT(GoalPole, Runaway, Wait, FairyFocusDemo, WaitGoalDemo, GoalDemoCatch,
                   GoalDemoFall, GoalDemoJump, GoalDemoFireworks)

/// Sprixie princess of one world: the world it belongs to and its model archive.
struct FairyInfo {
    s32 worldId;
    const char* archiveName;
};

/// Fireworks launched at the end of the goal demo, picked by the last digit of the timer.
struct FireworksInfo {
    const char* effectSuffix;
    s32 stepTableIndex;
    s32 timerLastDigit;
};

constexpr s32 cBindSensorNum = 10;
constexpr s32 cFairyNum = 7;
constexpr s32 cFireworksInfoNum = 3;
constexpr s32 cFireworksLaunchNum = 5;

const char* const cBindSensorName[cBindSensorNum] = {"Bind1", "Bind2", "Bind3", "Bind4",
                                                     "Bind5", "Bind6", "Bind7", "Bind8",
                                                     "Bind9", "Bind10"};

const FairyInfo cFairyInfo[cFairyNum] = {
    {1, "FairyPrincess01"}, {2, "FairyPrincess02"}, {3, "FairyPrincess03"},
    {4, "FairyPrincess04"}, {5, "FairyPrincess05"}, {6, "FairyPrincess06"},
    {7, "FairyPrincess07"},
};

const char* const cJumpCameraName[] = {"DemoPoleGoalJump1", "DemoPoleGoalJump2",
                                       "DemoPoleGoalJump3", "DemoPoleGoalJump4"};

const FireworksInfo cFireworksInfo[cFireworksInfoNum] = {
    {"01", 0, 1},
    {"03", 1, 3},
    {"05", 2, 6},
};

const s32 cFireworksLaunchStep[cFireworksInfoNum][cFireworksLaunchNum] = {
    {0, 0, 0, 0, 0},
    {0, 30, 60, 0, 0},
    {0, 20, 40, 60, 80},
};

const f32 cBindPosOffsetDoubleMario[cBindSensorNum] = {1000.0f, 690.0f, 610.0f, 530.0f, 450.0f,
                                                       370.0f,  310.0f, 230.0f, 150.0f, 120.0f};

const sead::Vector3f cFallArrowOffset(200.0f, 100.0f, 0.0f);
const sead::Vector3f cFairyFocusLookAtOffset(400.0f, 500.0f, 1250.0f);
const sead::Vector3f cFairyFocusCameraOffset(400.0f, 450.0f, 500.0f);
const sead::Vector3f cFireworksPos(0.0f, 0.0f, 0.0f);
const sead::Vector3f cRosettaOffset(-3750.0f, 0.0f, -500.0f);

/**
 * @brief Find the sprixie princess of a world.
 * @param worldId World to look for.
 * @return The princess info, or nullptr if the world has none.
 */
const FairyInfo* findFairyInfo(s32 worldId) {
    for (s32 i = 0; i < cFairyNum; i++) {
        if (cFairyInfo[i].worldId == worldId) {
            return &cFairyInfo[i];
        }
    }

    return nullptr;
}

/**
 * @brief Find the fireworks launched for a timer value.
 * @param lastDigit Last digit of the remaining time.
 * @return The fireworks info, or nullptr if no fireworks are launched.
 */
const FireworksInfo* findFireworksInfo(s32 lastDigit) {
    for (s32 i = 0; i < cFireworksInfoNum; i++) {
        if (cFireworksInfo[i].timerLastDigit == lastDigit) {
            return &cFireworksInfo[i];
        }
    }

    return nullptr;
}

/**
 * @brief Check whether the course being played was never cleared.
 * @param pHolder Scene object holder of the caller.
 * @return Whether the course is valid and not cleared yet.
 */
bool isNotClearPlayingCourse(const al::IUseSceneObjHolder* pHolder) {
    s32 courseId = GameDataFunction::getPlayingCourseId(pHolder);
    if (GameDataFunction::isInvalidCourseId(courseId)) {
        return false;
    }

    return !CourseInfoFunction::isClear(pHolder, courseId);
}

/**
 * @brief Check whether a front direction is parallel to the horizontal camera direction.
 * @param pActor Actor whose camera is checked.
 * @param rFront Front direction to compare.
 * @return Whether the directions are parallel.
 */
bool isParallelToCameraDir(const al::LiveActor* pActor, const sead::Vector3f& rFront) {
    sead::Vector3f frontH = rFront;
    frontH.y = 0.0f;
    if (al::normalizeOrZero(&frontH)) {
        return false;
    }

    const sead::Vector3f& lookAt = al::getCameraLookAt(pActor);
    const sead::Vector3f& cameraPos = al::getCameraPos(pActor);
    sead::Vector3f cameraDir(lookAt.x - cameraPos.x, 0.0f, lookAt.z - cameraPos.z);
    if (al::normalizeOrZero(&cameraDir)) {
        return false;
    }

    return al::isParallelDirection(frontH, cameraDir, 0.2f);
}
}  // namespace

/**
 * @brief Construct the goal pole.
 * @param pName Name of the actor.
 */
GoalPole::GoalPole(const char* pName) : al::LiveActor(pName) {}

/**
 * @brief Initialize the pole, its flag, the bound player puppeteers and the demo NPCs.
 * @param rInfo Placement info of the pole.
 */
void GoalPole::init(const al::ActorInitInfo& rInfo) {
    if (al::isObjectName(rInfo, "GoalPoleSuper")) {
        mType = Type::Super;
    } else if (al::isObjectName(rInfo, "GoalPoleLast")) {
        mType = Type::Last;
    }

    const char* archiveName;
    switch (mType) {
    case Type::Super:
        archiveName = "GoalPoleSuper";
        break;
    case Type::Last:
        archiveName = "GoalPoleLast";
        break;
    case Type::Normal:
    default:
        archiveName = "GoalPole";
        break;
    }

    al::initActorWithArchiveName(this, rInfo, archiveName, nullptr);
    al::tryGetArg(&mGoalSe, rInfo, "GoalSe");
    al::invalidateHitSensor(this, "Eye");
    mCatchCamera = al::initProgramableCamera(this, rInfo, "CatchCamera");
    al::makeMtxRT(&mBaseMtx, this);
    mAnimCamera = al::initAnimCamera(this, rInfo);

    s32 userNumMax = rc::getControlUserNumMax();
    mBindPos = new sead::Vector3f[cBindSensorNum];
    for (s32 i = 0; i < cBindSensorNum; i++) {
        mBindPos[i].set(al::getTrans(this));
        al::setHitSensorPosPtr(this, cBindSensorName[i], &mBindPos[i]);
        al::invalidateHitSensor(this, cBindSensorName[i]);
    }

    mPuppeteerGroup = new BindPuppeteerGroup("ゴールポールバインド操作グループ", userNumMax);
    for (s32 i = 0; i < mPuppeteerGroup->getPuppeteerNumMax(); i++) {
        mPuppeteerGroup->registerPuppeteer(
            new GoalPoleBindPuppeteer("ゴールポールバインド操作", this, rInfo, &mBaseMtx));
    }

    tryCreateNpc(rInfo);
    mFlag = new GoalPoleFlag(this);
    al::initCreateActorNoPlacementInfo(mFlag, rInfo);
    mClearLayout = new al::SimpleLayoutAppearWait("クリアレイアウト", "HeadCourseClear",
                                                  al::getLayoutInitInfo(rInfo), nullptr);

    if (al::isObjectName(rInfo, "GoalPoleRunaway")) {
        al::initNerve(this, &NrvGoalPole.Runaway, 1);
        mRunawayState = new GoalPoleStateRunaway(this, rInfo, &mBaseMtx);
        al::initNerveState(this, mRunawayState, &NrvGoalPole.Runaway, "逃げる");

        bool isInvalidClipping = false;
        if (al::tryGetArg(&isInvalidClipping, rInfo, "IsInvalidClipping") && isInvalidClipping) {
            al::invalidateClipping(this);
            al::invalidateClipping(mFlag);
        }
    } else if (isSuperWithFairyBottle()) {
        al::initNerve(this, &NrvGoalPole.Wait, 1);
        al::calcTransLocalOffset(&mFairyFocusLookAt, this, cFairyFocusLookAtOffset);
        auto* param = new ActorStateDemoCameraParam(180, 0, -1, &mFairyFocusLookAt,
                                                    al::getTransPtr(this));
        param->mCameraOffset.set(cFairyFocusCameraOffset);
        mFairyFocusState = new ActorStateDemoCamera(this, rInfo, "FairyFocusDemo", param, true);
        al::initNerveState(this, mFairyFocusState, &NrvGoalPole.FairyFocusDemo,
                           "妖精プリンセス着目");
        al::listenStageSwitchOn(
            this, "SwitchFairyPrincessFocusOn",
            al::FunctorV0M<GoalPole*, void (GoalPole::*)()>(this, &GoalPole::startDemoFairyFocus));
    } else {
        al::initNerve(this, &NrvGoalPole.Wait, 0);
        if (isLast()) {
            al::tryGetLinksQT(&mEndingPrevPlayerQuat, &mEndingPrevPlayerTrans, rInfo,
                              "EndingPrevDemoPlayerReturnPos");
            al::listenStageSwitchOn(this, "SwitchEndingPrevDemoStart",
                                    al::FunctorV0M<GoalPole*, void (GoalPole::*)()>(
                                        this, &GoalPole::startDemoEndingPrev));
        }
    }

    if (isLast()) {
        al::PlacementId placementId;
        al::tryGetPlacementID(&placementId, rInfo);
        mEndingPrevDemoStartId = new sead::FixedSafeString<32>();
        mEndingPrevDemoStartId->format("%s%sS", placementId.mPlacementID,
                                       placementId.mZoneID != nullptr ? placementId.mZoneID : "");
        mEndingPrevDemoEndId = new sead::FixedSafeString<32>();
        mEndingPrevDemoEndId->format("%s%sE", placementId.mPlacementID,
                                     placementId.mZoneID != nullptr ? placementId.mZoneID : "");
    }

    if (mRunawayState != nullptr) {
        al::invalidateShadowIntensityAll(this);
    }

    makeActorAppeared();
}

/**
 * @brief Create the sprixie princesses or Rosalina waiting at the pole, depending on the course.
 * @param rInfo Placement info of the pole.
 */
void GoalPole::tryCreateNpc(const al::ActorInitInfo& rInfo) {
    s32 courseId = GameDataFunction::getPlayingCourseId(this);
    if (GameDataFunction::isInvalidCourseId(courseId)) {
        return;
    }

    bool isClear = CourseInfoFunction::isClear(this, courseId);
    s32 worldId = -1;
    s32 stageId = -1;
    GameDataFunction::calcWorldAndStageId(this, &worldId, &stageId, courseId);

    if (courseId == GameDataFunction::calcRosettaAppearanceCourseId(this)) {
        if (isClear) {
            al::tryOnStageSwitch(this, "SwitchRosettaNpcUnexistOn");
            return;
        }

        mRosetta = new RosettaNpc("ロゼッタNPC", rInfo);
        al::calcTransLocalOffset(al::getTransPtr(mRosetta), this, cRosettaOffset);
        return;
    }

    if (GameDataFlagFunction::isLeaveCastleFairyPrincess(this) && !isLast() && isClear) {
        return;
    }

    if (!GameDataFunction::isStageKoopaCastle(this, courseId)) {
        return;
    }

    if (isLast() || (worldId == 7 && !isClear)) {
        mFairyGroup = new FairyGroup("妖精プリンセスグループ", cFairyNum);
        for (s32 i = 0; i < cFairyNum; i++) {
            const FairyInfo& info = cFairyInfo[i];
            if (info.worldId != 7 &&
                !GameDataFlagFunction::isEnableDemoLeaveCastleFairyPrincess(this, info.worldId)) {
                continue;
            }

            auto* fairy = new FairyPrincess("妖精プリンセス");
            fairy->initFairyWithArchiveName(
                rInfo, info.archiveName,
                isLast()   ? "WaitGoalPoleLast" :
                isClear    ? "WaitHappyGoalPole" :
                worldId != 7 ? "WaitGoalPole" :
                info.worldId == 7 ? "WaitGoalPole" :
                                    "WaitGoalPoleW7");
            if (!isClear && info.worldId == 7 && worldId == 7) {
                fairy->createBottle(rInfo);
                fairy->setEnableBottleReaction(false);
            }

            mFairyGroup->registerActor(fairy);
        }

        return;
    }

    const FairyInfo* info = findFairyInfo(worldId);
    if (info == nullptr) {
        return;
    }

    mFairyGroup = new FairyGroup("妖精プリンセスグループ", 1);
    auto* fairy = new FairyPrincess("妖精プリンセス");
    fairy->initFairyWithArchiveName(rInfo, info->archiveName,
                                    isClear ? "WaitHappyGoalPole" :
                                    worldId == 7 ? (info->worldId == 7 ? "WaitGoalPole" :
                                                                         "WaitGoalPoleW7") :
                                                   "WaitGoalPole");
    if (!isClear) {
        fairy->createBottle(rInfo);
    }

    mFairyGroup->registerActor(fairy);
}

/**
 * @brief Check whether this is a super goal pole (the one with a sprixie princess).
 * @return Whether the pole is a super goal pole.
 */
bool GoalPole::isSuper() const {
    return mType == Type::Super;
}

/// Start the demo focusing on the sprixie princess.
void GoalPole::startDemoFairyFocus() {
    al::setNerve(this, &NrvGoalPole.FairyFocusDemo);
}

/**
 * @brief Check whether this is the last goal pole of the game.
 * @return Whether the pole is the last goal pole.
 */
bool GoalPole::isLast() const {
    return mType == Type::Last;
}

/// Request the demo played before the ending.
void GoalPole::startDemoEndingPrev() {
    al::setNerve(this, &NrvGoalPoleEndingPrevDemoRequest);
}

/// Spread the bind sensors upwards when double Marios exist, and check the fur-off area.
void GoalPole::initAfterPlacement() {
    if (PlayerStockerFunction::getDoubleMarioCreateNum(this) >= 1) {
        for (s32 i = 0; i < cBindSensorNum; i++) {
            mBindPos[i] += sead::Vector3f(0.0f, cBindPosOffsetDoubleMario[i], 0.0f);
            al::validateHitSensor(this, cBindSensorName[i]);
        }
    }

    if (al::isInAreaObj(this, "PlayerFurOffArea")) {
        mIsEnableFur = false;
    }
}

/// Update every bound player puppeteer.
void GoalPole::control() {
    for (s32 i = 0; i < mPuppeteerGroup->getPuppeteerNum(); i++) {
        getPuppeteer(i)->update();
    }
}

/**
 * @brief Get a bound player puppeteer.
 * @param index Index of the puppeteer.
 * @return The puppeteer.
 */
GoalPoleBindPuppeteer* GoalPole::getPuppeteer(s32 index) const {
    return mPuppeteerGroup->getPuppeteer<GoalPoleBindPuppeteer>(index);
}

/**
 * @brief Kill the enemies around the pole during the goal demo.
 * @param pSelf Sensor of the pole.
 * @param pOther Sensor that was hit.
 */
void GoalPole::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (!al::isSensorName(pSelf, "Eye")) {
        return;
    }

    if (al::isNerve(this, &NrvGoalPole.WaitGoalDemo) || al::isNerve(this, &NrvGoalPole.GoalDemoCatch)) {
        for (s32 i = 0; i < mPuppeteerGroup->getPuppeteerNum(); i++) {
            if (getPuppeteer(i)->isCatchJust()) {
                al::sendMsgGoalKill(pOther, pSelf);
                return;
            }
        }
    }

    if (al::isNerve(this, &NrvGoalPole.GoalDemoFall) || al::isNerve(this, &NrvGoalPole.GoalDemoJump) ||
        al::isNerve(this, &NrvGoalPole.GoalDemoFireworks)) {
        al::sendMsgGoalKill(pOther, pSelf);
    }

    if (al::isNerve(this, &NrvGoalPole.GoalDemoFall) &&
        (al::isNewNerve(this) || al::isFirstStep(this))) {
        rc::sendMsgStartGoalDemoPole(pOther, pSelf);
        if (mRunawayState != nullptr) {
            rc::sendMsgGoalKillRunaway(pOther, pSelf);
        }
    }
}

/**
 * @brief Handle the players catching the pole.
 * @param pMsg Received message.
 * @param pOther Sensor that sent the message.
 * @param pSelf Sensor of the pole.
 * @return Whether the message was handled.
 */
bool GoalPole::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                          al::HitSensor* pSelf) {
    if (al::isSensorMapObj(pSelf) &&
        al::isHitCylinderSensor(pOther, pSelf, sead::Vector3f::ey, 10.0f) &&
        al::isMsgPlayerBoomerangReflect(pMsg)) {
        return true;
    }

    if (al::isNerve(this, &NrvGoalPole.WaitGoalDemo) ||
        (al::isNerve(this, &NrvGoalPole.GoalDemoFall) &&
         (al::isNewNerve(this) || al::isFirstStep(this)))) {
        for (s32 i = 0; i < mPuppeteerGroup->getPuppeteerNum(); i++) {
            if (getPuppeteer(i)->isCatchSuccess() && getPuppeteer(i)->isBind() &&
                rc::isMsgAskControlUserId(
                    pMsg, rc::getPuppetSensor(getPuppeteer(i)->getPlayerPuppet()))) {
                return true;
            }
        }
    }

    if (al::isMsgBindSteal(pMsg)) {
        return true;
    }

    if (!al::isSensorBindableGoal(pSelf) || !al::isSensorPlayer(pOther)) {
        return false;
    }

    if (al::isMsgBindCancel(pMsg)) {
        // The result of this check is unused in the game.
        al::isNerve(this, &NrvGoalPole.Wait);
        return true;
    }

    if (rc::isPlayerDead(al::getSensorHost(pOther)) || mIsInvalidBind) {
        return false;
    }

    if (al::isMsgBindInit(pMsg)) {
        rc::notifyGoalKillGhostPlayerRecorder(this);
        GoalPoleBindPuppeteer* puppeteer = getPuppeteer(rc::findControlUserId(pOther));
        puppeteer->startBind(pOther, pSelf, mCatchNum);
        rc::forceEndSubActionPuppet(puppeteer->getPlayerPuppet());
        al::sendMsgHoldCancel(pOther, pSelf);
        if (puppeteer->isCatchSuccess()) {
            mCatchNum++;
        }

        return true;
    }

    bool isFirstCatch = false;
    bool isCheckCatch = false;
    if (al::isNerve(this, &NrvGoalPole.Wait) || al::isNerve(this, &NrvGoalPole.Runaway)) {
        isFirstCatch = true;
        isCheckCatch = true;
    } else if (al::isNerve(this, &NrvGoalPole.WaitGoalDemo) && al::isLessEqualStep(this, 180)) {
        isCheckCatch = true;
    }

    if (isCheckCatch) {
        if (al::getActorTrans(pOther).y < al::getTrans(this).y + 250.0f) {
            f32 distance = (al::getSensorPos(pSelf) - al::getSensorPos(pOther)).length();
            if (al::getSensorRadius(pOther) + 80.0f < distance) {
                return false;
            }
        }

        sead::Vector3f front = {0.0f, 0.0f, 0.0f};
        al::calcFrontDir(&front, this);
        if (isParallelToCameraDir(this, front) &&
            !al::isHitCylinderSensor(pOther, pSelf, front, al::getSensorRadius(pSelf) * 0.5f)) {
            return false;
        }
    }

    if (!al::isMsgBindStart(pMsg)) {
        return false;
    }

    if (!isFirstCatch && !al::isNerve(this, &NrvGoalPole.WaitGoalDemo)) {
        return false;
    }

    if (mPuppeteerGroup->isBindingSameUserId(pOther)) {
        return false;
    }

    if (!isFirstCatch) {
        return true;
    }

    mBaseMtx.setTranslation(al::getTrans(this));
    mPoleTopTrans.set(getPoleTrans());
    if (!rc::isPlayerClimbOrClimbSpecial(pOther)) {
        fixCamera(al::getSensorPos(pOther));
    }

    al::validateHitSensor(this, "Eye");
    mIsGoal = true;
    al::setNerve(this, &NrvGoalPole.WaitGoalDemo);
    return true;
}

/**
 * @brief Get the position of the top of the pole.
 * @return Translation of the pole top.
 */
const sead::Vector3f& GoalPole::getPoleTrans() const {
    return al::getTrans(al::getSubActor(this, "ポール先端"));
}

/**
 * @brief Keep the camera still while several players are playing.
 * @param rLookAt Position the camera looks at.
 */
void GoalPole::fixCamera(const sead::Vector3f& rLookAt) {
    if (rc::calcActivePlayerNum(this) < 2) {
        return;
    }

    sead::Vector3f offset = rLookAt - al::getCameraLookAt(this);
    al::setCameraPos(mCatchCamera, offset + al::getCameraPos(this));
    al::setCameraLookAtPos(mCatchCamera, rLookAt);
    al::setCameraUpDir(mCatchCamera, al::getCameraUpDir(this));
    al::startCamera(this, mCatchCamera, 300);
}

/**
 * @brief Check whether the goal demo is playing.
 * @return Whether the players are catching, falling, jumping or watching the fireworks.
 */
bool GoalPole::isGoalDemoPlaying() const {
    return al::isNerve(this, &NrvGoalPole.GoalDemoCatch) ||
           al::isNerve(this, &NrvGoalPole.GoalDemoFall) ||
           al::isNerve(this, &NrvGoalPole.GoalDemoJump) ||
           al::isNerve(this, &NrvGoalPole.GoalDemoFireworks);
}

/**
 * @brief Check whether the super pole holds a sprixie princess in a bottle (course not cleared).
 * @return Whether the princess is still trapped.
 */
bool GoalPole::isSuperWithFairyBottle() const {
    return isSuper() && mFairyGroup != nullptr && isNotClearPlayingCourse(this);
}

/**
 * @brief Check whether a player caught the pole.
 * @param index Index of the puppeteer.
 * @return Whether the catch succeeded.
 */
bool GoalPole::isCatchSuccess(s32 index) const {
    return getPuppeteer(index)->isCatchSuccess();
}

/**
 * @brief Compute the height rate at which a player caught the pole.
 * @param index Index of the puppeteer.
 * @return Catch height rate (0 at the bottom, 1 at the top).
 */
f32 GoalPole::calcCatchHeightRate(s32 index) const {
    return getPuppeteer(index)->calcCatchHeightRate();
}

/**
 * @brief Find the player who caught the pole the highest.
 * @return Index of the puppeteer of that player.
 */
s32 GoalPole::calcHighestUserId() const {
    for (s32 i = 0; i < mPuppeteerGroup->getPuppeteerNum(); i++) {
        if (!getPuppeteer(i)->isCatchSuccess()) {
            continue;
        }

        if (calcHeightOrder(i) < 1) {
            return i;
        }
    }

    return 0;
}

/**
 * @brief Count the players who caught the pole higher than a player.
 * @param index Index of the puppeteer of the player.
 * @return Number of higher players (ties are broken by catch order).
 */
s32 GoalPole::calcHeightOrder(s32 index) const {
    GoalPoleBindPuppeteer* self = getPuppeteer(index);
    s32 order = 0;
    for (s32 i = 0; i < mPuppeteerGroup->getPuppeteerNum(); i++) {
        GoalPoleBindPuppeteer* other = getPuppeteer(i);
        if (other == self || !other->isCatchSuccess()) {
            continue;
        }

        if (self->getCatchHeight() < other->getCatchHeight() ||
            (self->getCatchHeight() == other->getCatchHeight() &&
             other->getCatchIndex() < self->getCatchIndex())) {
            order++;
        }
    }

    return order;
}

/**
 * @brief Get the height order stored in a puppeteer.
 * @param index Index of the puppeteer.
 * @return Height order of the player.
 */
s32 GoalPole::getHeightOrder(s32 index) const {
    return getPuppeteer(index)->getHeightOrder();
}

/// Hide the players, the flag, the pole and the princesses before the World 7 castle clear demo.
void GoalPole::prepareDemoW7KoopaCastleClear() {
    for (s32 i = 0; i < mPuppeteerGroup->getPuppeteerNum(); i++) {
        GoalPoleBindPuppeteer* puppeteer = getPuppeteer(i);
        if (puppeteer->isBind()) {
            rc::hidePuppet(puppeteer->getPlayerPuppet());
            rc::hidePuppetFur(puppeteer->getPlayerPuppet());
        }
    }

    al::hideModel(mFlag);
    al::hideModel(al::getSubActor(this, 0));
    if (mFairyGroup != nullptr) {
        for (s32 i = 0; i < mFairyGroup->getActorCount(); i++) {
            mFairyGroup->getActor(i)->kill();
        }
    }

    mClearLayout->kill();
}

/// Start the World 7 castle clear demo where Bowser catches the princess again.
void GoalPole::startDemoW7KoopaCastleClear() {
    al::startAction(this, "DemoKoopaCatchFairyAgain");
}

/// Make the princesses pose with the players at the end of the goal demo.
void GoalPole::startGoalPoseNpcAction() {
    if (mFairyGroup == nullptr) {
        return;
    }

    s32 worldId = GameDataFunction::calcPlayingWorldId(this);
    al::StringTmp<32> actionName(isLast() ? "DemoBeforeEnding" : "GoalPoseSuper");
    if (mCatchNum >= 2) {
        actionName.appendWithFormat("%d", mCatchNum);
    }

    for (s32 i = 0; i < mFairyGroup->getActorCount(); i++) {
        if (worldId == 7 && isNotClearPlayingCourse(this) &&
            !mFairyGroup->getDeriveActor(i)->isExistBottle()) {
            mFairyGroup->getDeriveActor(i)->startDemoAction("GoalPoseSuperW7", this);
        } else {
            mFairyGroup->getDeriveActor(i)->startDemoAction(actionName.cstr(), this);
        }
    }
}

/**
 * @brief Release the princesses from their bottles at the end of the goal demo.
 * @return Whether a princess was released.
 */
bool GoalPole::tryReleaseFairy() {
    if (isLast()) {
        for (s32 i = 0; i < mFairyGroup->getActorCount(); i++) {
            mFairyGroup->getDeriveActor(i)->startDemoAction("DemoBeforeEndingStart", this);
        }

        return true;
    }

    if (isNotClearPlayingCourse(this) && isSuper() && mFairyGroup != nullptr) {
        bool isReleased = false;
        for (s32 i = 0; i < mFairyGroup->getActorCount(); i++) {
            if (mFairyGroup->getDeriveActor(i)->tryBreakBottle()) {
                isReleased = true;
            } else {
                mFairyGroup->getDeriveActor(i)->startDemoAction("ReleaseW7", this);
            }
        }

        return isReleased;
    }

    return false;
}

/// Turn the flag back to its default direction.
void GoalPole::startLerpFlag() {
    mFlag->startLerp();
}

/// Move the bind sensors to the nearest point of the pole for each player and resize them.
void GoalPole::updateBindSensor() {
    sead::Vector3f bottom = al::getTrans(this);
    sead::Vector3f top = al::getTrans(this);
    bottom.y += 120.0f;
    top.y += 1000.0f;

    sead::Vector3f front = {0.0f, 0.0f, 0.0f};
    al::calcFrontDir(&front, this);
    bool isParallel = isParallelToCameraDir(this, front);
    if (PlayerStockerFunction::getDoubleMarioCreateNum(this) == 0) {
        for (s32 i = 0; i < rc::getControlUserNumMax(); i++) {
            al::LiveActor* player = rc::tryFindNearestPlayerActorByUserId(this, i, -1.0f);
            if (player != nullptr) {
                al::validateHitSensor(this, cBindSensorName[i]);
                al::calcPerpendicFootToLineInside(&mBindPos[i], al::getTrans(player), bottom, top);
            } else {
                al::invalidateHitSensor(this, cBindSensorName[i]);
            }
        }
    }

    // Sensors high above the pole base get a larger radius. The middle band keeps the lower
    // radius too (the game still fetches the translation for its check).
    if (isParallel) {
        for (s32 i = 0; i < cBindSensorNum; i++) {
            f32 radius;
            if (al::getTrans(this).y + 900.0f < mBindPos[i].y) {
                radius = 280.0f;
            } else if (al::getTrans(this).y + 120.0f < mBindPos[i].y) {
                radius = 160.0f;
            } else {
                radius = 160.0f;
            }

            al::setSensorRadius(this, cBindSensorName[i], radius);
        }
    } else {
        for (s32 i = 0; i < cBindSensorNum; i++) {
            f32 radius;
            if (al::getTrans(this).y + 900.0f < mBindPos[i].y) {
                radius = 140.0f;
            } else if (al::getTrans(this).y + 120.0f < mBindPos[i].y) {
                radius = 80.0f;
            } else {
                radius = 80.0f;
            }

            al::setSensorRadius(this, cBindSensorName[i], radius);
        }
    }
}

/// Kill the flag as soon as a player finished catching the pole.
void GoalPole::tryKillFlag() {
    if (al::isDead(mFlag)) {
        return;
    }

    for (s32 i = 0; i < mPuppeteerGroup->getPuppeteerNum(); i++) {
        if (getPuppeteer(i)->isEndCatchMove()) {
            mFlag->kill();
            return;
        }
    }
}

/// Make the flag appear at the bottom of the pole once every player allows it.
void GoalPole::tryAppearFlagBottom() {
    if (al::isAlive(mFlag)) {
        return;
    }

    for (s32 i = 0; i < mPuppeteerGroup->getPuppeteerNum(); i++) {
        if (!getPuppeteer(i)->isEnableAppearFlag()) {
            return;
        }
    }

    if (!isSuper() && !isLast()) {
        sead::Vector3f poleSide;
        mBaseMtx.getBase(poleSide, 0);

        bool isFacingSide = false;
        for (s32 i = 0; i < mPuppeteerGroup->getPuppeteerNum(); i++) {
            if (!getPuppeteer(i)->isCatchSuccess()) {
                continue;
            }

            const sead::Vector3f& front =
                rc::getPuppetFrontVec(getPuppeteer(i)->getPlayerPuppet());
            if (al::calcAngleDegree(poleSide, -front) < 40.0f) {
                isFacingSide = true;
                break;
            }
        }

        if (isFacingSide) {
            mFlag->setRotateY(45.0f);
        }
    }

    mFlag->appearBottom(calcHighestUserId());
}

/**
 * @brief Find the player right under another one on the pole.
 * @param heightOrder Height order of the upper player.
 * @return Puppeteer of the player below, or nullptr if none.
 */
GoalPoleBindPuppeteer* GoalPole::findUnderHeightPuppeteer(s32 heightOrder) const {
    for (s32 i = 0; i < mPuppeteerGroup->getPuppeteerNum(); i++) {
        GoalPoleBindPuppeteer* puppeteer = getPuppeteer(i);
        if (!puppeteer->isCatchSuccess()) {
            continue;
        }

        if (heightOrder + 1 == calcHeightOrder(i)) {
            return puppeteer;
        }
    }

    return nullptr;
}

/**
 * @brief Find the player who caught the pole first.
 * @return Puppeteer of that player, or nullptr if none.
 */
GoalPoleBindPuppeteer* GoalPole::findFirstCatchPuppeteer() const {
    for (s32 i = 0; i < mPuppeteerGroup->getPuppeteerNum(); i++) {
        GoalPoleBindPuppeteer* puppeteer = getPuppeteer(i);
        if (puppeteer->isCatchSuccess() && puppeteer->getCatchIndex() == 0) {
            return puppeteer;
        }
    }

    return nullptr;
}

/// Keep the flag and the princesses updated during demos.
void GoalPole::addDemoActorNpc() {
    rc::addDemoActor(mFlag);
    for (s32 i = 0; i < mFairyGroup->getActorCount(); i++) {
        rc::addDemoActor(mFairyGroup->getDeriveActor(i));
        rc::addDemoActor(mFairyGroup->getDeriveActor(i)->getWingActor());
        if (mFairyGroup->getDeriveActor(i)->isExistBottle()) {
            rc::addDemoActor(mFairyGroup->getDeriveActor(i)->getBottleActor());
            rc::addDemoActor(mFairyGroup->getDeriveActor(i)->getBottleInnerActor());
            rc::addDemoActor(mFairyGroup->getDeriveActor(i)->getBottleCapActor());
        }
    }
}

/// Wait for the players.
void GoalPole::exeWait() {
    updateBindSensor();
}

/// Run away from the players.
void GoalPole::exeRunaway() {
    al::updateNerveState(this);
    updateBindSensor();
}

/// Show the trapped sprixie princess with a camera demo.
void GoalPole::exeFairyFocusDemo() {
    if (al::isFirstStep(this)) {
        addDemoActorNpc();
        rc::disappearCameraChangeLayout(this);
    }

    if (al::isStep(this, 75)) {
        al::startSe(this, "FairyCloseUp");
    }

    if (al::updateNerveState(this)) {
        for (s32 i = 0; i < mFairyGroup->getActorCount(); i++) {
            FairyPrincess* fairy = mFairyGroup->getDeriveActor(i);
            if (fairy->isExistBottle()) {
                fairy->setEnableBottleReaction(true);
            }
        }

        rc::appearCameraChangeLayout(this);
        al::setNerve(this, &NrvGoalPole.Wait);
    }
}

/// Wait until the players can be used for the demo before the ending.
void GoalPole::exeEndingPrevDemoRequest() {
    if (rc::requestStartDemoPlayer(this)) {
        al::requestCaptureScreenCover(this, 2);
        al::setNerve(this, &NrvGoalPoleEndingPrevDemo);
    }
}

/// Play the demo before the ending, then put the players back.
void GoalPole::exeEndingPrevDemo() {
    if (al::isFirstStep(this)) {
        al::startAnimCamera(this, mAnimCamera, "DemoBeforeEnding", 0);
        rc::hideDemoPlayerAll(this);
        rc::stopSklAnimAndDeleteEffectDemoPlayerAll(this);
        rc::stopAndHideGhostPlayerAll(this);
        rc::setPlacementIdObjGhostPlayerRecorder(this, mEndingPrevDemoStartId->cstr(), true);
        rc::disappearCameraChangeLayout(this);
        al::startSe(this, "PgFairyCloseUpLast");
    }

    if (al::isStep(this, 210)) {
        al::startSe(this, "PgFairyCloseUpLastFocused");
    }

    if (al::isEndAnimCamera(mAnimCamera)) {
        al::onStageSwitch(this, "SwitchEndingPrevDemoEndOn");
        al::endCamera(this, mAnimCamera, 0);
        rc::showDemoPlayerAll(this);
        rc::replaceDemoPlayerAll(this, mEndingPrevPlayerTrans, mEndingPrevPlayerQuat, 150.0f);
        rc::startActionDemoPlayerAll(this, "Wait");
        rc::requestEndDemoPlayer(this);
        rc::appearCameraChangeLayout(this);
        rc::restartGhostPlayerAll(this);
        rc::setPlacementIdObjGhostPlayerRecorder(this, mEndingPrevDemoEndId->cstr(), false);
        rc::tryStartFromObjGhostPlayerRecorder(this, mEndingPrevDemoEndId->cstr());
        al::setNerve(this, &NrvGoalPole.Wait);
    }
}

/// Wait for the other players to catch the pole.
void GoalPole::exeWaitGoalDemo() {
    if (al::isFirstStep(this)) {
        al::invalidateClipping(this);
        al::invalidateClipping(mFlag);
    }

    updateBindSensor();
    tryKillFlag();
    if (al::isGreaterEqualStep(this, 180)) {
        if (al::isStep(this, 180)) {
            for (s32 i = 0; i < mPuppeteerGroup->getPuppeteerNum(); i++) {
                GoalPoleBindPuppeteer* puppeteer = getPuppeteer(i);
                if (!puppeteer->isBind()) {
                    puppeteer->onCatchFailure();
                }
            }
        }

        rc::requestBindAllPlayerAcceptReviveBubble(this, al::getHitSensor(this, "Bind1"));
    }

    if (rc::isAllPlayerBinded(this) && al::isDead(mFlag)) {
        al::setNerve(this, &NrvGoalPole.GoalDemoCatch);
    }
}

/// Start the goal music and wait for every player to be ready to slide down.
void GoalPole::exeGoalDemoCatch() {
    if (al::isFirstStep(this)) {
        if (mGoalSe == 1) {
            al::startSe(this, "GoalSircus");
            if (al::isEnableRhythmAnim(this, "Stage")) {
                f32 beat = al::getCurBeat(this);
                if (sead::Mathf::abs(beat - 165.95f) <= 0.2f ||
                    sead::Mathf::abs(beat - 166.95f) <= 0.2f ||
                    sead::Mathf::abs(beat - 167.95f) <= 0.2f) {
                    al::startSe(this, "PerfectEnd");
                }
            }
        }

        alSeFunction::setIsStateAfterGoal(this, true);
        al::stopAllBgm(this, -1);
        switch (mType) {
        case Type::Super:
            al::prepareSequenceBgm(this, al::BgmPlayingRequest("WorldClear"));
            break;
        case Type::Last:
            al::prepareSequenceBgm(this, al::BgmPlayingRequest("KoopaLastClear"));
            break;
        case Type::Normal:
        default:
            al::prepareSequenceBgm(this, al::BgmPlayingRequest("Clear"));
            break;
        }

        al::changeAudioEffect(this, nullptr);
    }

    tryAppearFlagBottom();
    for (s32 i = 0; i < mPuppeteerGroup->getPuppeteerNum(); i++) {
        if (!getPuppeteer(i)->isEnableStartFall()) {
            return;
        }
    }

    al::setNerve(this, &NrvGoalPole.GoalDemoFall);
}

/// Slide the players down the pole and raise the flag.
void GoalPole::exeGoalDemoFall() {
    if (al::isFirstStep(this)) {
        mTimerDisplayCount = mStageTimer->calcDisplayCount();
        for (s32 i = 0; i < mPuppeteerGroup->getPuppeteerNum(); i++) {
            GoalPoleBindPuppeteer* puppeteer = getPuppeteer(i);
            if (puppeteer->isCatchSuccess()) {
                s32 heightOrder = calcHeightOrder(i);
                puppeteer->startFall(heightOrder, mCatchNum, findUnderHeightPuppeteer(heightOrder));
            }
        }

        al::startAnimCamera(this, mAnimCamera, "DemoPoleGoalFall", &mBaseMtx, 60);
        rc::endRecordGhostPlayerRecorder(this, false);
        mFlag->startUpward(calcCatchHeightRate(calcHighestUserId()));
        if (mRunawayState != nullptr) {
            mRunawayState->appearStep();
        }

        al::startSe(this, "Down");

        const char* materialName = nullptr;
        if (mRunawayState != nullptr) {
            materialName = "Cloud";
        } else {
            al::Triangle triangle;
            sead::Vector3f arrowStart = {0.0f, 0.0f, 0.0f};
            al::calcTransLocalOffset(&arrowStart, this, cFallArrowOffset);
            alCollisionUtil::getFirstPolyOnArrow(this, nullptr, &triangle, arrowStart,
                                                 sead::Vector3f(0.0f, -200.0f, 0.0f), nullptr,
                                                 nullptr);
            if (triangle.isValid()) {
                materialName = al::getMaterialCodeName(triangle);
            }
        }

        if (materialName != nullptr) {
            for (s32 i = 0; i < mPuppeteerGroup->getPuppeteerNum(); i++) {
                if (getPuppeteer(i)->isCatchSuccess()) {
                    rc::updateMaterialGoalPole(
                        rc::getPuppetSensor(getPuppeteer(i)->getPlayerPuppet()), materialName);
                }
            }
        }
    }

    bool isEndFallingAll = true;
    for (s32 i = 0; i < mPuppeteerGroup->getPuppeteerNum(); i++) {
        if (!getPuppeteer(i)->isEndFalling()) {
            isEndFallingAll = false;
            break;
        }
    }

    if (isEndFallingAll) {
        al::stopSeByName(this, "Down");
    }

    if (!al::isGreaterStep(this, 60)) {
        return;
    }

    for (s32 i = 0; i < mPuppeteerGroup->getPuppeteerNum(); i++) {
        if (!getPuppeteer(i)->isEnableStartJump()) {
            return;
        }
    }

    al::setNerve(this, &NrvGoalPole.GoalDemoJump);
}

/// Make the players jump off the pole and start the course clear music and layout.
void GoalPole::exeGoalDemoJump() {
    if (al::isFirstStep(this)) {
        for (s32 i = 0; i < mPuppeteerGroup->getPuppeteerNum(); i++) {
            GoalPoleBindPuppeteer* puppeteer = getPuppeteer(i);
            if (puppeteer->isCatchSuccess()) {
                puppeteer->startJump();
            }
        }

        al::endCamera(this, mAnimCamera, -1);
        al::startAnimCamera(this, mAnimCamera, cJumpCameraName[mCatchNum - 1], &mBaseMtx, 0);
        al::tryOnStageSwitch(this, "SwitchDemoJumpStartOn");
        if (mRosetta != nullptr) {
            al::resetPosition(mRosetta, al::getTrans(this), false);
            mRosetta->startGoalPose(mCatchNum);
        }
    }

    s32 delay = GoalPoleBindPuppeteer::getJumpStartDelayFrame() * (mCatchNum - 1);
    if (al::isStep(this, delay + 19)) {
        if (isSuper()) {
            al::startSequenceBgm(this, "WorldClear", -1, 0);
        } else if (isLast()) {
            al::startSequenceBgm(this, "KoopaLastClear", -1, 0);
        } else {
            al::startSequenceBgm(this, "Clear", -1, 0);
        }
    }

    if (al::isStep(this, delay + (isLast() ? 942 : isSuper() ? 339 : 192))) {
        mClearLayout->appear();
    }

    bool isFireworks = !isLast() && findFireworksInfo(mTimerDisplayCount % 10) != nullptr;
    if (isFireworks && al::isStep(this, delay + (isSuper() ? 380 : 240))) {
        al::setNerve(this, &NrvGoalPole.GoalDemoFireworks);
        return;
    }

    if (al::isStep(this, delay + (isSuper() ? 400 : isLast() ? 1200 : 270))) {
        if (isLast() || findFireworksInfo(mTimerDisplayCount % 10) == nullptr) {
            mIsEndGoalDemo = true;
        }
    }
}

/// Launch the fireworks matching the last digit of the remaining time.
void GoalPole::exeGoalDemoFireworks() {
    if (al::isFirstStep(this)) {
        const FireworksInfo* info = findFireworksInfo(mTimerDisplayCount % 10);
        GoalPoleBindPuppeteer* puppeteer = findFirstCatchPuppeteer();
        const char* characterName = rc::getPlayerCharacterName(
            al::getSensorHost(rc::getPuppetSensor(puppeteer->getPlayerPuppet())));
        al::StringTmp<64> effectName("%s%s", characterName, info->effectSuffix);
        al::setEffectFollowPosPtr(this, effectName.cstr(), &cFireworksPos);
        al::emitEffect(this, effectName.cstr(), nullptr);
    }

    s32 stepTableIndex = findFireworksInfo(mTimerDisplayCount % 10)->stepTableIndex;
    for (s32 i = 0; i < cFireworksLaunchNum; i++) {
        if (al::isStep(this, cFireworksLaunchStep[stepTableIndex][i])) {
            rc::addScoreByFactor(this,
                                 rc::getPuppetSensor(findFirstCatchPuppeteer()->getPlayerPuppet()),
                                 "花火", 0.0f, 0);
            al::startSe(this, "FireWork");
            break;
        }
    }

    if (al::isStep(this, 120)) {
        mIsEndGoalDemo = true;
    }
}
