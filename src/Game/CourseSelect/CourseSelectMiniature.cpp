#include "CourseSelect/CourseSelectMiniature.hpp"

#include <math/seadQuat.h>
#include <prim/seadEnum.h>

#include "CourseSelect/CourseSelectDirector.hpp"
#include "CourseSelect/CourseSelectFairy.hpp"
#include "CourseSelect/CourseSelectFlag.hpp"
#include "CourseSelect/CourseSelectLock.hpp"
#include "CourseSelect/CourseSelectNode.hpp"
#include "CourseSelect/CourseSelectPuppeteerGroup.hpp"
#include "CourseSelect/CourseSelectRouteDokan.hpp"
#include "CourseSelect/CourseSelectScene.hpp"
#include "CourseSelect/CourseSelectSensor.hpp"
#include "CourseSelect/CourseSelectWorldWarpDokan.hpp"
#include "CourseSelect/MiniatureController.hpp"
#include "Demo/DemoWorldClear.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Library/Joint/JointControllerKeeper.hpp"
#include "Library/Joint/JointSpringController.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorAnimUtil.hpp"
#include "Library/LiveActor/Util/ActorFlagUtil.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorModelUtil.hpp"
#include "Library/LiveActor/Util/ActorMovementUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Movement/RumbleCalculator.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Play/Placement/PlacementInfo.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/StageSwitch/StageSwitchFunc.hpp"
#include "System/CourseInfoHolder.hpp"
#include "System/Data/StageDatabaseInfo.hpp"
#include "System/GameDataFlagFunction.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/GameDataHolderWriter.hpp"

/**
 * Declares a CourseSelectMiniature nerve whose execute function has a different name than the
 * nerve.
 * @param Action The nerve name.
 * @param Func The CourseSelectMiniature::exe* function the nerve runs.
 */
#define COURSE_SELECT_MINIATURE_NERVE(Action, Func)                                                \
    class CourseSelectMiniatureNrv##Action : public al::Nerve {                                    \
    public:                                                                                        \
        void execute(al::NerveKeeper* pKeeper) const override {                                    \
            pKeeper->getParent<CourseSelectMiniature>()->exe##Func();                              \
        }                                                                                          \
    };

namespace {
NERVE_DECL(CourseSelectMiniature, Hide)
NERVE_DECL(CourseSelectMiniature, ClearKinopioBrigade)
COURSE_SELECT_MINIATURE_NERVE(WaitClearDemo, Wait)
NERVE_DECL(CourseSelectMiniature, LockWait)
COURSE_SELECT_MINIATURE_NERVE(LockWaitDisable, LockWait)
COURSE_SELECT_MINIATURE_NERVE(ClearWaitHouse, Wait)
NERVE_DECL(CourseSelectMiniature, ClearHide)
COURSE_SELECT_MINIATURE_NERVE(ClearWait, Wait)
NERVE_DECL(CourseSelectMiniature, Wait)
COURSE_SELECT_MINIATURE_NERVE(CloseWait, Wait)
NERVE_DECL(CourseSelectMiniature, KinopioBrigadeWaitEnter)
NERVE_DECL(CourseSelectMiniature, Enter)
COURSE_SELECT_MINIATURE_NERVE(ClearWithHide, Clear)
NERVE_DECL(CourseSelectMiniature, ClearGateKeeper)
NERVE_DECL(CourseSelectMiniature, Clear)
NERVE_DECL(CourseSelectMiniature, Appear)
NERVE_DECL(CourseSelectMiniature, EnterEnd)
NERVE_DECL(CourseSelectMiniature, KinopioBrigadeEnter)
NERVES_MAKE_NOSTRUCT(CourseSelectMiniature, Hide, ClearKinopioBrigade, WaitClearDemo, LockWait,
                     LockWaitDisable, ClearWaitHouse, ClearHide, ClearWait, Wait, CloseWait,
                     KinopioBrigadeWaitEnter, Enter, ClearWithHide, ClearGateKeeper, Clear, Appear,
                     EnterEnd, KinopioBrigadeEnter)

// clang-format off
SEAD_ENUM(GateKeeperHairJoint, Hair1, Hair2, Hair3)
// clang-format on

/** @brief Spring parameters of a hair joint of the gate keeper miniature. */
struct HairSpringParam {
    f32 stability;
    f32 friction;
    f32 limitDegree;
    sead::Vector3f childLocalPos;
};

const HairSpringParam sHairSpringParams[] = {
    {0.1f, 0.7f, 60.0f, sead::Vector3f(60.0f, 0.0f, 0.0f)},
    {0.1f, 0.7f, 60.0f, sead::Vector3f(100.0f, 0.0f, 0.0f)},
    {0.1f, 0.7f, 60.0f, sead::Vector3f(100.0f, 0.0f, 0.0f)},
};

const sead::Vector3f sFairyOffset(-280.0f, 255.0f, 161.0f);
const sead::Vector3f sFairyOffsetClear(340.0f, 380.0f, 450.0f);
const sead::Vector3f sFairyOffsetW7(-600.0f, 490.0f, -55.0f);
const sead::Vector3f sFairyOffsetW7Clear(300.0f, 700.0f, 475.0f);

}  // namespace

/**
 * @brief Constructs the miniature.
 * @param pName Actor name.
 */
CourseSelectMiniature::CourseSelectMiniature(const char* pName) : al::LiveActor(pName) {}

/**
 * @brief Initializes the model, the course data and the controller, and creates the objects
 * placed around the course: route and world warp dokans, closed course model, clear flag, green
 * star lock, branch node, world clear demo and fairy of Bowser's castles.
 * @param rInfo Actor init info.
 */
void CourseSelectMiniature::init(const al::ActorInitInfo& rInfo) {
    al::initActorChangeModel(this, rInfo);
    mDirector = CourseSelectDirector::getCourseSelectDirector(this);
    mActorInfo = new CourseSelectActorInfo(this, rInfo);
    mController = new MiniatureController(this, mActorInfo);
    mSensor = new CourseSelectSensor(cCourseSelectSensorType_Actor, mController);

    // Flags read from the placement and the stage database.
    {
        const char* pModelName = nullptr;
        alPlacementFunction::tryGetModelName(&pModelName, rInfo);

        bool isAlwaysShow = false;
        al::tryGetArg(&isAlwaysShow, rInfo, "AlwaysShow");
        u32 flags = isAlwaysShow << 4;

        StageDatabaseInfo* pStageInfo = mActorInfo->getStageDatabaseInfo();
        if (!pStageInfo->isNormal() || pStageInfo->isKoopaCastleExpressNormal()) {
            flags |= cFlag_NoRotate;
        }

        if (pStageInfo->isKinopioHouseHide()) {
            flags |= cFlag_HideModel;
        }

        if (pStageInfo->isKoopaCastle()) {
            flags |= cFlag_KoopaCastle;
        }

        if (pStageInfo->isCasinoRoom() || pStageInfo->isKinopioHouseHide() ||
            pStageInfo->isGoldenExpress()) {
            flags |= cFlag_ClearHide;
        }

        if (pStageInfo->isKinopioHouse() || pStageInfo->isFairyHouse()) {
            flags |= cFlag_House;
        }

        mFlags = flags;
    }

    if (getStageInfo()->isGateKeeper()) {
        const char* pName = nullptr;
        if (alPlacementFunction::tryGetModelName(&pName, rInfo) &&
            al::isEqualString(pName, "MiniatureGateKeeperKyuppon")) {
            al::initJointControllerKeeper(this, 3);
            for (auto it = GateKeeperHairJoint::begin(); it != GateKeeperHairJoint::end(); ++it) {
                auto* pController = al::initJointSpringController(this, (*it).text());
                pController->setStability(sHairSpringParams[*it].stability);
                pController->setFriction(sHairSpringParams[*it].friction);
                pController->setLimitDegree(sHairSpringParams[*it].limitDegree);
                pController->setChildLocalPos(sHairSpringParams[*it].childLocalPos);
            }
        }
    }

    al::PlacementInfo linksInfo;
    if (al::tryGetPlacementInfoByKey(&linksInfo, al::getPlacementInfo(rInfo), "Links")) {
        al::PlacementInfo openCourseInfo;
        if (al::tryGetPlacementInfoByKey(&openCourseInfo, linksInfo, "NoDelete_OpenCourse")) {
            s32 num = al::getCountPlacementInfo(openCourseInfo);
            mNextCourseIds = new s32[num];
            s32 count = 0;
            for (s32 i = 0; i < num; i++) {
                al::PlacementInfo courseInfo;
                if (!al::tryGetPlacementInfoByIndex(&courseInfo, openCourseInfo, i)) {
                    continue;
                }

                s32 worldId = 0;
                s32 stageId = 0;
                if (!al::tryGetArg(&worldId, courseInfo, "WorldID") ||
                    !al::tryGetArg(&stageId, courseInfo, "StageID")) {
                    continue;
                }

                s32 courseId =
                    GameDataFunction::calcCourseId(GameDataHolderAccessor(this), worldId, stageId);
                if (courseId == GameDataFunction::getLastKoopaCourseId(GameDataHolderAccessor(this)) &&
                    CourseInfoFunction::isClose(GameDataHolderAccessor(this), courseId)) {
                    continue;
                }

                mNextCourseIds[count] = courseId;
                count++;
            }

            mNextCourseNum = count;
        }
    }

    al::PlacementInfo dokanInfo;
    if (al::tryGetLinksInfo(&dokanInfo, al::getPlacementInfo(rInfo), "CourseSelectRouteDokan")) {
        al::ActorInitInfo dokanInitInfo;
        dokanInitInfo.initViewIdSelf(&dokanInfo, rInfo);
        mRouteDokan = new CourseSelectRouteDokan("コース選択ルート土管");
        mRouteDokan->init(dokanInitInfo);

        if (getStageInfo()->isGateKeeper()) {
            if (!CourseInfoFunction::isClear(GameDataHolderAccessor(this), getCourseId()) ||
                GameDataFunction::isStageLastPlayAndFirstClear(GameDataHolderAccessor(this),
                                                               getCourseId())) {
                mRouteDokan->createStopper(dokanInitInfo);
            }
        } else if (getStageInfo()->isKoopaCastle() && getWorldId() == 7) {
            if (CourseInfoFunction::isClear(GameDataHolderAccessor(this), getCourseId())) {
                mRouteDokan->startAppearEffectW7();
            }
        } else if (GameDataFunction::isStageLastPlayAndFirstClear(GameDataHolderAccessor(this),
                                                                  getCourseId()) ||
                   !CourseInfoFunction::isClear(GameDataHolderAccessor(this), getCourseId())) {
            mRouteDokan->deactive();
        }

        if (al::calcLinkChildNum(rInfo, "NoDelete_NextNodeEnd") >= 1) {
            mHasNextNodeEnd = true;
            al::PlacementInfo nodeEndInfo;
            al::getLinksInfo(&nodeEndInfo, al::getPlacementInfo(rInfo), "NoDelete_NextNodeEnd");
            al::getTrans(&mNextNodeEndTrans, nodeEndInfo);
        }
    }

    if (al::tryGetLinksInfo(&dokanInfo, al::getPlacementInfo(rInfo),
                            "NoDelete_CourseSelectDokan")) {
        mHasDokanLink = true;
        al::getTrans(&mDokanTrans, dokanInfo);
    }

    if (al::tryGetLinksInfo(&dokanInfo, al::getPlacementInfo(rInfo),
                            "CourseSelectWorldWarpDokan")) {
        al::ActorInitInfo dokanInitInfo;
        dokanInitInfo.initViewIdSelf(&dokanInfo, rInfo);
        mWorldWarpDokan = new CourseSelectWorldWarpDokan("コース選択ワールドワープ土管");
        mWorldWarpDokan->init(dokanInitInfo);
    }

    al::initNerve(this, &NrvCourseSelectMiniatureHide, 0);
    makeActorAppeared();

    StageDatabaseInfo* pStageInfo = getStageInfo();
    al::LiveActor* pCloseModel = nullptr;
    if (!pStageInfo->isGateKeeper() && !pStageInfo->isKoopaCastle() &&
        !pStageInfo->isCasinoRoom() && !pStageInfo->isKinopioHouseHide() &&
        !pStageInfo->isGoldenExpress()) {
        if (pStageInfo->isKinopioHouse() || pStageInfo->isFairyHouse()) {
            pCloseModel = new al::LiveActor("コース未開放ミニ");
            al::initActorWithArchiveName(pCloseModel, rInfo, "MiniatureCloseSmall", nullptr);
        } else {
            pCloseModel = new al::LiveActor("コース未開放");
            al::initActorWithArchiveName(pCloseModel, rInfo, "MiniatureClose", nullptr);
        }

        pCloseModel->makeActorDead();
    }

    mCloseModel = pCloseModel;

    pStageInfo = getStageInfo();
    if (pStageInfo->isNormal() || pStageInfo->isKoopaCastle() ||
        pStageInfo->isKinopioBrigade() || pStageInfo->isContinuousMysteryBox() ||
        pStageInfo->isGateKeeperGoalPole()) {
        mFlag = new CourseSelectFlag("コース選択 旗", this);
        al::initCreateActorNoPlacementInfo(mFlag, rInfo);

        sead::Vector3f flagTrans = al::getTrans(this);
        if (pStageInfo->isKoopaCastle()) {
            if (getWorldId() == 7) {
                flagTrans += sead::Vector3f(350.0f, 250.0f, -175.0f);
            } else if (getWorldId() == 8) {
                flagTrans += sead::Vector3f(400.0f, 135.0f, 200.0f);
            } else {
                flagTrans += sead::Vector3f(350.0f, 35.0f, 255.0f);
            }
        } else {
            flagTrans += sead::Vector3f(150.0f, 0.0f, 150.0f);
        }

        al::resetPosition(mFlag, flagTrans, false);
    }

    s32 lockGreenStarNum =
        GameDataFunction::findCourseLockGreenStarNum(GameDataHolderAccessor(this), getCourseId());
    if ((lockGreenStarNum >= 1 &&
         !CourseInfoFunction::isOpen(GameDataHolderAccessor(this), getCourseId())) ||
        CourseInfoFunction::isGreenStarLock(GameDataHolderAccessor(this), getCourseId())) {
        s32 lockNum = lockGreenStarNum < 999 ? lockGreenStarNum : 999;
        mLock = new CourseSelectLock("コース選択 グリーンスターロック", this, lockNum);
        al::initCreateActorNoPlacementInfo(mLock, rInfo);

        sead::Vector3f lockTrans = al::getTrans(this);
        lockTrans.y += pStageInfo->isKoopaCastle() ? 650.0f : 0.0f;
        al::resetPosition(mLock, lockTrans, false);
    }

    mRumble = new al::RumbleCalculatorCosMultLinear(3.5f, 1.5707963705062866f, 0.04f, 30);

    mNode = new CourseSelectNode("コース選択分岐点[ミニチュア]", this);
    al::initCreateActorWithPlacementInfo(mNode, rInfo);
    mNode->setController(mController);

    if (pStageInfo->isKoopaCastle() && getWorldId() < 7) {
        mDemoWorldClear = new DemoWorldClear(this);
        mDemoWorldClear->init(rInfo);
        al::startMclAnimAndSetFrameAndStop(this, "Color", getWorldId());
    }

    if (pStageInfo->isKoopaCastle() && getWorldId() < 8) {
        bool isClear = CourseInfoFunction::isClear(GameDataHolderAccessor(this), getCourseId());
        bool isFirstClear = GameDataFunction::isStageLastPlayAndFirstClear(
            GameDataHolderAccessor(this), getCourseId());
        bool isLeaveFairy = false;
        if (!isFirstClear && isClear) {
            isLeaveFairy =
                GameDataFlagFunction::isLeaveCastleFairyPrincess(GameDataHolderAccessor(this));
        }

        mFairy = new CourseSelectFairy("妖精(クッパ城)", getWorldId(), !isClear);
        al::initCreateActorNoPlacementInfo(mFairy, rInfo);

        sead::Vector3f offset = getWorldId() == 7 ?
                                    (isClear ? sFairyOffsetW7Clear : sFairyOffsetW7) :
                                    (isClear ? sFairyOffsetClear : sFairyOffset);
        sead::Vector3f fairyTrans = offset + al::getTrans(this);
        if (isFirstClear) {
            mFairy->startWorldClearDemo();
        }

        al::resetPosition(mFairy, fairyTrans, false);
        if (isLeaveFairy) {
            mFairy->kill();
        }
    }

    if (pStageInfo->isGateKeeper()) {
        al::tryGetArg(&mGateKeeperMoveDir, rInfo, "GKMoveDir");
        bool isBackInverse = false;
        al::tryGetArg(&isBackInverse, rInfo, "BackInverse");
        if (isBackInverse) {
            mFlags |= cFlag_BackInverse;
        }
    }

    if (pStageInfo->isGoldenExpress() &&
        GameDataFunction::isNeedOpenGoldenExpress(GameDataHolderAccessor(this))) {
        CourseInfoFunction::openGoldenExpress(GameDataHolderAccessor(this), getCourseId());
    }

    if (!(mFlags & cFlag_HideModel) && !pStageInfo->isEvent()) {
        al::HitSensor* pBodySensor = al::getHitSensor(this, "Body");
        mDrcSensor = new CourseSelectSensor(cCourseSelectSensorType_Actor, mController);
        mDrcSensor->setSensor(pBodySensor);
    }

    if (mFlags & cFlag_HideModel) {
        al::hideModelIfShow(this);
    }

    al::tryGetArg(&mColorFrame, rInfo, "ColorFrame");
    if (mColorFrame >= 0) {
        if (al::tryStartMclAnimIfExist(this, "Color")) {
            al::setMclAnimFrameAndStop(this, mColorFrame);
        }

        if (al::tryStartMtpAnimIfExist(this, "Color")) {
            al::setMtpAnimFrameAndStop(this, mColorFrame);
        }
    }
}

/**
 * @brief Gets the stage database entry of the course.
 * @return The entry.
 */
StageDatabaseInfo* CourseSelectMiniature::getStageInfo() const {
    return mActorInfo->getStageDatabaseInfo();
}

/**
 * @brief Gets the course of the miniature.
 * @return The course id.
 */
s32 CourseSelectMiniature::getCourseId() const {
    return mActorInfo->getCourseId();
}

/**
 * @brief Gets the world of the course.
 * @return The world id.
 */
s32 CourseSelectMiniature::getWorldId() const {
    return mActorInfo->getWorldId();
}

/**
 * @brief Reads the open and clear state of the course from the save data and picks the first
 * nerve accordingly (clear demos, locks, waits or hidden while closed).
 */
void CourseSelectMiniature::initAfterPlacement() {
    if (al::isExistAction(this, "Appear")) {
        mFlags |= cFlag_HasAppearAction;
    }

    if ((mDirector->isAllwaysOpenCourse(getCourseId()) || getStageId() == 1) &&
        !getStageInfo()->isGoldenExpress() &&
        getCourseId() != GameDataFunction::getLastKoopaCourseId(GameDataHolderAccessor(this))) {
        mFlags |= cFlag_AlwaysOpen;
    }

    if (getStageInfo()->isKinopioBrigade() &&
        GameDataFunction::isStageLastPlay(GameDataHolderAccessor(this), getCourseId())) {
        al::setNerve(this, &NrvCourseSelectMiniatureClearKinopioBrigade);
        return;
    }

    if (GameDataFunction::isStageLastPlayAndFirstClear(GameDataHolderAccessor(this),
                                                       getCourseId()) &&
        !GameDataFlagFunction::isAfterEnding(GameDataHolderAccessor(this))) {
        al::setNerve(this, &NrvCourseSelectMiniatureWaitClearDemo);
        return;
    }

    if (isGreenStarLock()) {
        al::setNerve(this, &NrvCourseSelectMiniatureLockWait);
        return;
    }

    if (!isCourseOpen() && mLock != nullptr && (mFlags & cFlag_KoopaCastle)) {
        al::setNerve(this, &NrvCourseSelectMiniatureLockWaitDisable);
        return;
    }

    if (CourseInfoFunction::isClear(GameDataHolderAccessor(this), getCourseId())) {
        if (mFlags & cFlag_House) {
            al::setNerve(this, &NrvCourseSelectMiniatureClearWaitHouse);
        } else if (mFlags & cFlag_ClearHide) {
            al::setNerve(this, &NrvCourseSelectMiniatureClearHide);
        } else {
            al::setNerve(this, &NrvCourseSelectMiniatureClearWait);
        }
    } else if ((mFlags & cFlag_AlwaysOpen) ||
               CourseInfoFunction::isOpen(GameDataHolderAccessor(this), getCourseId())) {
        al::setNerve(this, &NrvCourseSelectMiniatureWait);
    } else if (mFlags & cFlag_AlwaysShow) {
        al::setNerve(this, &NrvCourseSelectMiniatureCloseWait);
    } else {
        al::setNerve(this, &NrvCourseSelectMiniatureHide);
        showCloseModel();
    }
}

/**
 * @brief Gets the stage number of the course in its world.
 * @return The stage id.
 */
s32 CourseSelectMiniature::getStageId() const {
    return mActorInfo->getStageNo();
}

/**
 * @brief Gets whether the course is locked by green stars.
 * @return true if a green star lock closes the course.
 */
bool CourseSelectMiniature::isGreenStarLock() const {
    if ((mFlags & cFlag_AlwaysOpen) && mLock != nullptr &&
        !CourseInfoFunction::isOpen(GameDataHolderAccessor(this), getCourseId())) {
        return true;
    }

    return CourseInfoFunction::isGreenStarLock(GameDataHolderAccessor(this), getCourseId());
}

/**
 * @brief Gets whether the course is open.
 * @return true if the course is always open or opened in the save data.
 */
bool CourseSelectMiniature::isCourseOpen() const {
    if (mFlags & cFlag_AlwaysOpen) {
        return true;
    }

    return CourseInfoFunction::isOpen(GameDataHolderAccessor(this), getCourseId());
}

/**
 * @brief Plays the "near" sound while the main player stands on the miniature, and lets the
 * touch screen select it.
 */
void CourseSelectMiniature::control() {
    bool isSelected = mDirector->getSelectedController() == mController;
    if (!isSelected) {
        if (al::isExistSeKeeper(this)) {
            al::setSeSourceVolume(this, 0.4f);
            if (mIsNearSe) {
                al::stopSeByName(this, "PgGetNearClearWait");
                al::stopSeByName(this, "PgGetNearShowWait");
            }
        }
    } else if (al::isExistSeKeeper(this)) {
        al::setSeSourceVolume(this, 1.0f);
        if (!mIsNearSe) {
            if (al::isActionPlaying(this, "ClearWait") &&
                al::isExistSePlayNameInUserInfo(this, "PgGetNearClearWait")) {
                al::tryStartSe(this, "PgGetNearClearWait");
            } else if (al::isActionPlaying(this, "ShowWait") &&
                       al::isExistSePlayNameInUserInfo(this, "PgGetNearShowWait")) {
                al::tryStartSe(this, "PgGetNearShowWait");
            }
        }
    }

    mIsNearSe = isSelected;

    if (mDrcSensor == nullptr) {
        return;
    }

    if (!al::isNerve(this, &NrvCourseSelectMiniatureWait) &&
        !al::isNerve(this, &NrvCourseSelectMiniatureCloseWait) &&
        !al::isNerve(this, &NrvCourseSelectMiniatureClearWait)) {
        return;
    }

    if (getWorldId() != mDirector->getActiveWorldId()) {
        return;
    }

    if (getStageInfo()->isGateKeeperGoalPole() && al::isHideModel(this)) {
        return;
    }

    mDirector->checkDrcTouch(mDrcSensor);
}

/** @brief Appears unless the miniature waits hidden for its clear demo, and shows the flag. */
void CourseSelectMiniature::appear() {
    if (!al::isNerve(this, &NrvCourseSelectMiniatureClearHide)) {
        al::LiveActor::appear();
    }

    if (mFlag != nullptr) {
        mFlag->updateVisibility(true);
    }
}

/** @brief Kills the miniature and hides the flag. */
void CourseSelectMiniature::kill() {
    al::LiveActor::kill();
    if (mFlag != nullptr) {
        mFlag->updateVisibility(false);
    }
}

/**
 * @brief Pushes the players away and reports the players standing on the miniature to the
 * director.
 * @param pSelf Own sensor.
 * @param pOther Sensor of the other actor.
 */
void CourseSelectMiniature::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isNerve(this, &NrvCourseSelectMiniatureWait) ||
        al::isNerve(this, &NrvCourseSelectMiniatureClearWait) ||
        al::isNerve(this, &NrvCourseSelectMiniatureClearWaitHouse) ||
        al::isNerve(this, &NrvCourseSelectMiniatureCloseWait)) {
        if (al::isSensorPlayer(pOther) && al::isSensorMapObj(pSelf)) {
            al::sendMsgPush(pOther, pSelf);
        }
    }

    if ((al::isNerve(this, &NrvCourseSelectMiniatureWait) ||
         al::isNerve(this, &NrvCourseSelectMiniatureClearWait)) &&
        al::isSensorEye(pSelf) && al::isSensorPlayer(pOther)) {
        mSensor->setSensor(pSelf);
        mDirector->touchPlayer(mSensor, al::getSensorHost(pOther));
    }
}

/**
 * @brief Ignores every message.
 * @param pMsg Received message.
 * @param pSelf Own sensor.
 * @param pOther Sensor of the sender.
 * @return Always false.
 */
bool CourseSelectMiniature::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSelf,
                                       al::HitSensor* pOther) {
    return false;
}

/** @brief Starts the enter animation when a player enters the course. */
void CourseSelectMiniature::startEnter() {
    if (getStageInfo()->isKinopioBrigade()) {
        al::setNerve(this, &NrvCourseSelectMiniatureKinopioBrigadeWaitEnter);
        return;
    }

    if (!(mFlags & cFlag_NoEnter)) {
        al::setNerve(this, &NrvCourseSelectMiniatureEnter);
    }
}

/** @brief Starts the demo played after the course was cleared for the first time. */
void CourseSelectMiniature::startClearDemo() {
    al::tryOnStageSwitch(this, "SwitchClearOn");
    if (mFlags & cFlag_ClearHide) {
        al::setNerve(this, &NrvCourseSelectMiniatureClearWithHide);
    }

    if (isGateKeeper()) {
        al::setNerve(this, &NrvCourseSelectMiniatureClearGateKeeper);
    } else {
        al::setNerve(this, &NrvCourseSelectMiniatureClear);
    }
}

/**
 * @brief Gets whether the course is a gate keeper (mini boss) course.
 * @return true for a gate keeper course.
 */
bool CourseSelectMiniature::isGateKeeper() const {
    return GameDataFunction::isStageGateKeeper(GameDataHolderAccessor(this), getCourseId());
}

/** @brief Opens the route dokan leaving from the miniature. */
void CourseSelectMiniature::startOpenRouteDokan() {
    if (isGateKeeper()) {
        mRouteDokan->setEnableInput();
    } else if (mRouteDokan->isDeactive()) {
        mRouteDokan->active();
    }
}

/**
 * @brief Gets whether a course is opened by clearing this one.
 * @param courseId Course to check.
 * @return true if the course is one of the next courses.
 */
bool CourseSelectMiniature::isNextCourse(s32 courseId) const {
    for (s32 i = 0; i < mNextCourseNum; i++) {
        if (mNextCourseIds[i] == courseId) {
            return true;
        }
    }

    return false;
}

/**
 * @brief Finds the node at the end of the route dokan.
 * @return The node, or nullptr without route dokan.
 */
CourseSelectNode* CourseSelectMiniature::tryFindNextNodeEnd() const {
    if (mRouteDokan == nullptr || !mHasNextNodeEnd) {
        return nullptr;
    }

    return CourseSelectDirector::getCourseSelectDirector(this)->tryFindNodeFromTrans(
        mNextNodeEndTrans);
}

/**
 * @brief Remembers the puppeteers playing a demo on the miniature.
 * @param pGroup Puppeteer group.
 */
void CourseSelectMiniature::startPuppetDemo(CourseSelectPuppeteerGroup* pGroup) {
    mPuppeteerGroup = pGroup;
}

/** @brief Forgets the puppeteers playing a demo on the miniature. */
void CourseSelectMiniature::endPuppetDemo() {
    mPuppeteerGroup = nullptr;
}

/**
 * @brief Gets whether a clear demo has to be played when coming back from a course.
 * @return true if a course was cleared for the first time or something new opened.
 */
bool CourseSelectMiniature::isNeedClearDemo() const {
    if (GameDataFunction::isLastPlayCourseFirstClear(GameDataHolderAccessor(this))) {
        return true;
    }

    if (GameDataFunction::isNeedOpenCasinoRoom(GameDataHolderAccessor(this))) {
        return true;
    }

    return GameDataFlagFunction::isExistNewOpenFlag(GameDataHolderAccessor(this), true);
}

/**
 * @brief Gets whether the clear demo is playing.
 * @return true while the clear demo plays.
 */
bool CourseSelectMiniature::isPlayingClearDemo() const {
    if (al::isNerve(this, &NrvCourseSelectMiniatureClearWithHide) ||
        al::isNerve(this, &NrvCourseSelectMiniatureClearGateKeeper)) {
        return true;
    }

    return al::isNerve(this, &NrvCourseSelectMiniatureClear);
}

/**
 * @brief Gets whether the course is a Toad house.
 * @return true for a Toad house.
 */
bool CourseSelectMiniature::isKinopioHouse() const {
    StageDatabaseInfo* pStageInfo = getStageInfo();
    return pStageInfo != nullptr && pStageInfo->isKinopioHouse();
}

/**
 * @brief Collects the controllers of the objects opened by clearing the course, sorted by open
 * priority.
 * @param pControllers Output controllers.
 * @param maxNum Capacity of the output (unused).
 * @return The number of controllers found.
 */
s32 CourseSelectMiniature::tryFindNextNode(ICourseSelectActorController** pControllers,
                                           s32 maxNum) {
    CourseSelectMiniature* nextMiniatures[16];
    s32 num = mDirector->tryFindNextCourse(nextMiniatures, 16, getCourseId());
    for (s32 i = 0; i < num; i++) {
        pControllers[i] = nextMiniatures[i]->mController;
    }

    s32 gk1CourseId = GameDataFunction::getLastKoopaGK1CourseId(GameDataHolderAccessor(this));
    s32 gk2CourseId = GameDataFunction::getLastKoopaGK2CourseId(GameDataHolderAccessor(this));
    if ((getCourseId() == gk1CourseId &&
         CourseInfoFunction::isClear(GameDataHolderAccessor(this), gk2CourseId)) ||
        (getCourseId() == gk2CourseId &&
         CourseInfoFunction::isClear(GameDataHolderAccessor(this), gk1CourseId))) {
        pControllers[num] =
            mDirector
                ->findMiniatureObj(
                    GameDataFunction::getLastKoopaCourseId(GameDataHolderAccessor(this)))
                ->mController;
        num++;
    }

    if (mHasDokanLink) {
        pControllers[num++] = mDirector->tryFindNodeFromTrans(mDokanTrans)->getController();
    }

    for (s32 i = 0; i < num - 1; i++) {
        for (s32 j = i + 1; j < num; j++) {
            if (pControllers[i]->calcOpenNodePriority() > pControllers[j]->calcOpenNodePriority()) {
                ICourseSelectActorController* pTemp = pControllers[i];
                pControllers[i] = pControllers[j];
                pControllers[j] = pTemp;
            }
        }
    }

    return num;
}

/** @brief Opens the course: plays the appear demo or the green star lock appear demo. */
void CourseSelectMiniature::startAppear() {
    if (al::isDead(this)) {
        appear();
    }

    if (mLock != nullptr) {
        startGreenStarLockAppear();
        return;
    }

    if (getStageInfo()->isCasinoRoom()) {
        CourseInfoFunction::openCasinoRoom(GameDataHolderWriter(this), getCourseId());
    }

    if (al::isNerve(this, &NrvCourseSelectMiniatureHide) ||
        al::isNerve(this, &NrvCourseSelectMiniatureClearHide)) {
        al::setNerve(this, &NrvCourseSelectMiniatureAppear);
    } else if (!al::isNerve(this, &NrvCourseSelectMiniatureLockWait) &&
               !al::isNerve(this, &NrvCourseSelectMiniatureClearWait)) {
        CourseInfoFunction::setOpen(GameDataHolderWriter(this), getCourseId());
        al::setNerve(this, &NrvCourseSelectMiniatureWait);
        return;
    }

    CourseInfoFunction::setOpen(GameDataHolderWriter(this), getCourseId());
}

/** @brief Makes the green star lock appear in front of the miniature, on the road to it. */
void CourseSelectMiniature::startGreenStarLockAppear() {
    if (mLock == nullptr) {
        return;
    }

    CourseInfoFunction::setGreenStarLock(GameDataHolderWriter(this), getCourseId());

    if (mLock != nullptr && !(mFlags & cFlag_KoopaCastle)) {
        sead::Vector3f lockTrans = al::getTrans(this);
        sead::Vector3f dir(0.0f, 0.0f, 0.0f);
        if (mNode->getLinkNum() == 0) {
            return;
        }

        CourseSelectNode* pNextNode = mDirector->getNodes()[mNode->getFrontLinkNodeIndex()];
        lockTrans.y = al::getTrans(pNextNode).y;
        const sead::Vector3f& rNextTrans = al::getTrans(pNextNode);
        const sead::Vector3f& rNodeTrans = al::getTrans(mNode);
        dir.x = rNextTrans.x - rNodeTrans.x;
        dir.z = rNextTrans.z - rNodeTrans.z;
        dir.y = 0.0f;
        al::normalize(&dir);

        f32 dist = getStageInfo()->isKoopaCastle() ? 650.0f : 0.0f;
        lockTrans.setScaleAdd(dist, dir, lockTrans);
        al::resetPosition(mLock, lockTrans, false);
        mLock->startAppear();
        CourseSelectDirector::getCourseSelectDirector(this)->getPuppeteerGroup()->startLockAppearDemo(
            mController);
    }

    al::setNerve(this, &NrvCourseSelectMiniatureLockWait);
}

/** @brief Starts the clear demo of the flag. */
void CourseSelectMiniature::startClearFlagDemo() {
    if (mFlag != nullptr) {
        mFlag->startClearDemo();
    }
}

/**
 * @brief Gets whether the miniature appeared and waits open.
 * @return true while waiting open.
 */
bool CourseSelectMiniature::isAppeared() const {
    return al::isNerve(this, &NrvCourseSelectMiniatureWait);
}

/** @brief Starts the world clear demo of Bowser's castle. */
void CourseSelectMiniature::startWorldClearDemo() {
    mDemoWorldClear->startDemo();
}

/**
 * @brief Gets whether the world clear demo ended.
 * @return true once the demo ended.
 */
bool CourseSelectMiniature::isEndWorldClearDemo() const {
    return mDemoWorldClear->isEndDemo();
}

/** @brief Brings the fairy princess back next to Bowser's castle. */
void CourseSelectMiniature::returnCastleFairyPrincess() {
    mFairy->appear();
}

/**
 * @brief Enables the sensors only while the players are in the world of the miniature.
 * @param worldId World the players are in.
 */
void CourseSelectMiniature::changeCurrentWorldId(s32 worldId) {
    if (getWorldId() == worldId) {
        al::validateHitSensors(this);
    } else {
        al::invalidateHitSensors(this);
    }
}

/**
 * @brief Enters the course when the main player decides on the miniature.
 * @param pDirector Course select director.
 * @return Whether the course is entered.
 */
bool CourseSelectMiniature::tryDecide(const CourseSelectDirector* pDirector) {
    if (!pDirector->isEnableEnterSelectedSensor()) {
        return false;
    }

    if (!pDirector->isTriggerDecideMainPlayer()) {
        return false;
    }

    pDirector->getScene()->enterStage(this);
    return true;
}

/**
 * @brief Calculates the direction a gate keeper moves to from its node: the placed direction,
 * or across the road to the next node.
 * @param pDirX Output X component of the direction.
 * @param pDirZ Output Z component of the direction.
 */
inline void CourseSelectMiniature::calcGateKeeperMoveDir(f32* pDirX, f32* pDirZ) const {
    *pDirX = 0.0f;
    *pDirZ = 1.0f;
    switch (mGateKeeperMoveDir) {
    case -1: {
        f32 sign = (mFlags & cFlag_BackInverse) ? 1.0f : -1.0f;
        if (mNode->getLinkNum() != 0) {
            CourseSelectNode* pNextNode = CourseSelectDirector::getCourseSelectDirector(this)
                                              ->getNodes()[mNode->getFrontLinkNodeIndex()];
            if (al::isNearZero(al::getTrans(pNextNode).x - al::getTrans(mNode).x, 0.001f)) {
                *pDirX = sign;
                *pDirZ = 0.0f;
                break;
            }
        }

        *pDirX = 0.0f;
        *pDirZ = sign;
        break;
    }
    case 0:
        *pDirX = 0.0f;
        *pDirZ = 1.0f;
        break;
    case 1:
        *pDirX = 1.0f;
        *pDirZ = 0.0f;
        break;
    case 2:
        *pDirX = 0.0f;
        *pDirZ = -1.0f;
        break;
    case 3:
        *pDirX = -1.0f;
        *pDirZ = 0.0f;
        break;
    }
}

/** @brief Sets the wait nerve played after the clear animation. */
inline void CourseSelectMiniature::setNerveAfterClear() {
    if (mFlags & cFlag_ClearHide) {
        al::setNerve(this, &NrvCourseSelectMiniatureClearHide);
    } else if (mFlags & cFlag_House) {
        al::setNerve(this, &NrvCourseSelectMiniatureClearWaitHouse);
    } else {
        al::setNerve(this, &NrvCourseSelectMiniatureClearWait);
    }
}

/** @brief Plays the clear animation of the miniature. */
void CourseSelectMiniature::exeClear() {
    if (al::isFirstStep(this)) {
        if (getStageInfo()->isCasinoRoom()) {
            CourseInfoFunction::resetClearFlag(GameDataHolderWriter(this), getCourseId());
        }

        if (!al::isExistAction(this, "StageClear") || getStageInfo()->isKinopioBrigade()) {
            setNerveAfterClear();
            return;
        }

        al::startAction(this, "StageClear");
    }

    if (al::isActionEnd(this)) {
        setNerveAfterClear();
    }
}

/** @brief Plays the clear animation of a gate keeper, which moves away from the road. */
void CourseSelectMiniature::exeClearGateKeeper() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "StageClear");

        f32 dirX;
        f32 dirZ;
        calcGateKeeperMoveDir(&dirX, &dirZ);
        al::setVelocity(this, sead::Vector3f(dirX * 200.0f * (1.0f / 60.0f), 0.0f,
                                             dirZ * 200.0f * (1.0f / 60.0f)));
    }

    if (al::isStep(this, 60)) {
        al::setVelocityZero(this);
    }

    if (al::isActionEnd(this) && al::isGreaterEqualStep(this, 60)) {
        al::setNerve(this, &NrvCourseSelectMiniatureClearWait);
    }
}

/** @brief Plays the clear animation of a Captain Toad course and shows the flag. */
void CourseSelectMiniature::exeClearKinopioBrigade() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "StageClear");
        if (!GameDataFunction::isStageLastPlayAndFirstClear(GameDataHolderAccessor(this),
                                                            getCourseId()) &&
            mFlag != nullptr) {
            mFlag->startShow();
        }
    }

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvCourseSelectMiniatureClearWait);
    }
}

/** @brief Shows the closed course model in place of the miniature. */
inline void CourseSelectMiniature::showCloseModel() {
    if (mCloseModel != nullptr) {
        mCloseModel->appear();
        al::showModelIfHide(mCloseModel);
    }

    al::hideModelIfShow(this);
}

/** @brief Hides the miniature while its course is closed. */
void CourseSelectMiniature::exeHide() {
    if (al::isFirstStep(this)) {
        showCloseModel();
    }
}

/** @brief Shows the miniature in place of the closed course model. */
inline void CourseSelectMiniature::hideCloseModel() {
    if (!(mFlags & cFlag_HideModel)) {
        al::showModelIfHide(this);
    }

    if (mCloseModel != nullptr) {
        al::hideModelIfShow(mCloseModel);
        mCloseModel->kill();
    }
}

/** @brief Plays the appear animation of the miniature when its course opens. */
void CourseSelectMiniature::exeAppear() {
    if (al::isFirstStep(this)) {
        if (getStageInfo()->isKoopaCastle()) {
            al::showModelIfHide(this);
            al::setNerve(this, &NrvCourseSelectMiniatureWait);
            return;
        }

        hideCloseModel();
        if (mFlag != nullptr) {
            mFlag->startAppear();
        }

        mAppearTrans = al::getTrans(this);
        if (mFlags & cFlag_HasAppearAction) {
            al::tryStartActionIfNotPlaying(this, "Appear");
        } else {
            if (al::isExistAction(this, "ShowWait")) {
                al::tryStartActionIfNotPlaying(this, "ShowWait");
            }

            al::startHitReactionAppear(this);
        }
    }

    if (al::isStep(this, 30) && isGateKeeper()) {
        CourseSelectDirector::getCourseSelectDirector(this)
            ->getPuppeteerGroup()
            ->startOpenGateKeeperDemo(mController);
    }

    if (mFlags & cFlag_HasAppearAction) {
        if (al::isActionEnd(this)) {
            al::setNerve(this, &NrvCourseSelectMiniatureWait);
        }
        return;
    }

    s32 step = al::getNerveStep(this);
    f32 frame = step;
    f32 scaleXZ;
    f32 scaleY;
    f32 offsetY;
    if (step < 28) {
        f32 rate = frame / 28.0f;
        f32 x = (rate - 0.5f) * 2.0f;
        f32 bounce = 1.0f - x * x;
        scaleXZ = 0.95f * rate + (1.225f - 0.95f) * bounce;
        scaleY = 0.95f * rate + (1.35f - 0.95f) * bounce;
        offsetY = bounce * 65.0f;
    } else if (step < 39) {
        f32 rate = (frame - 28.0f) / 11.0f;
        f32 x = (rate - 0.5f) * 2.0f;
        f32 bounce = 1.0f - x * x;
        scaleXZ = (1.0f - 0.95f) * rate + (1.05f - 1.0f) * bounce + 0.95f;
        scaleY = scaleXZ;
        offsetY = bounce * 25.0f;
    } else if (step < 45) {
        f32 rate = (frame - 39.0f) / 6.0f;
        f32 x = (rate - 0.5f) * 2.0f;
        f32 bounce = 1.0f - x * x;
        scaleXZ = 0.0f * rate + (1.025f - 1.0f) * bounce + 1.0f;
        scaleY = scaleXZ;
        offsetY = bounce * 2.5f;
    } else if (step >= 60) {
        al::setNerve(this, &NrvCourseSelectMiniatureWait);
        return;
    } else {
        offsetY = 0.0f;
        scaleXZ = 1.0f;
        scaleY = scaleXZ;
    }

    sead::Vector3f scale(scaleXZ, scaleY, scaleXZ);
    al::setScale(this, scale);
    al::setTrans(this, mAppearTrans + sead::Vector3f(0.0f, offsetY, 0.0f));
}

/**
 * @brief Waits on the map: places a cleared gate keeper away from the road, starts the wait
 * animations and keeps turning the miniature.
 */
void CourseSelectMiniature::exeWait() {
    if (al::isFirstStep(this)) {
        if (al::isNerve(this, &NrvCourseSelectMiniatureClearWait) ||
            al::isNerve(this, &NrvCourseSelectMiniatureClearWaitHouse) ||
            (getStageInfo()->isKoopaCastle() &&
             CourseInfoFunction::isClear(GameDataHolderAccessor(this), getCourseId()))) {
            al::tryOnStageSwitch(this, "SwitchClearOn");
        }

        if (isGateKeeper() &&
            al::isNerve(this, &NrvCourseSelectMiniatureClearWait)) {
            f32 dirX;
            f32 dirZ;
            calcGateKeeperMoveDir(&dirX, &dirZ);
            al::setTrans(this,
                         al::getTrans(mNode) + sead::Vector3f(dirX * 200.0f, 0.0f, dirZ * 200.0f));
        }

        if (al::isNerve(this, &NrvCourseSelectMiniatureWaitClearDemo) &&
            getStageInfo()->isKinopioBrigade()) {
            al::startAction(this, "StageClear");
        } else if (al::isNerve(this, &NrvCourseSelectMiniatureClearWait) ||
                   al::isNerve(this, &NrvCourseSelectMiniatureClearWaitHouse)) {
            if (al::isExistAction(this, "ClearWait")) {
                al::tryStartActionIfNotPlaying(this, "ClearWait");
            }
        } else if (al::isExistAction(this, "ShowWait")) {
            al::tryStartActionIfNotPlaying(this, "ShowWait");
        }

        hideCloseModel();

        if (al::isNerve(this, &NrvCourseSelectMiniatureWaitClearDemo)) {
            if (mFlag != nullptr) {
                mFlag->startWaitClearDemo();
            }
        } else if (!GameDataFunction::isStageLastPlayAndFirstClear(GameDataHolderAccessor(this),
                                                                   getCourseId()) ||
                   (GameDataFunction::isStageLastPlayAndFirstClear(GameDataHolderAccessor(this),
                                                                   getCourseId()) &&
                    GameDataFlagFunction::isAfterEnding(GameDataHolderAccessor(this)))) {
            if (mFlag != nullptr) {
                mFlag->startShow();
            }
        }

        if (isGateKeeper()) {
            mNode->tryAppearPointObj();
        }
    }

    updatePosture();
}

/** @brief Turns the miniature around its vertical axis. */
void CourseSelectMiniature::updatePosture() {
    if (mFlags & cFlag_NoRotate) {
        return;
    }

    mRotateDegree += 0.5f;
    if (mRotateDegree > 360.0f) {
        mRotateDegree += -360.0f;
    }

    sead::Quatf quat = al::getQuat(this);
    al::rotateQuatRadian(&quat, quat, sead::Vector3f::ey, mRotateDegree * -0.017453292f);
    al::updatePoseQuat(this, quat);
}

/** @brief Plays the enter animation: the miniature shakes, then the course is entered. */
void CourseSelectMiniature::exeEnter() {
    updatePosture();

    if (al::isStep(this, 49) && al::isExistSePlayNameInUserInfo(this, "PgEnterRumble")) {
        al::tryStartSe(this, "PgEnterRumble");
    }

    if (al::isStep(this, 60)) {
        mRumble->start(0);
    }

    if (al::isGreaterEqualStep(this, 60)) {
        mRumble->calc();
        al::setScaleAll(this, mRumble->getValueY() + 1.0f);
        if (mRumble->isEnd()) {
            al::setNerve(this, &NrvCourseSelectMiniatureEnterEnd);
        }
    }
}

/** @brief Keeps turning the miniature after the enter animation. */
void CourseSelectMiniature::exeEnterEnd() {
    updatePosture();
}

/** @brief Speeds up the wait animation of a Captain Toad course before entering it. */
void CourseSelectMiniature::exeKinopioBrigadeWaitEnter() {
    if (al::isFirstStep(this)) {
        al::setActionFrameRate(this, 2.2f);
    }

    // The first frame read is unused in the original code.
    al::getActionFrame(this);
    if (al::getActionFrame(this) < 15.0f) {
        al::setNerve(this, &NrvCourseSelectMiniatureKinopioBrigadeEnter);
    }
}

/** @brief Plays the enter animation of a Captain Toad course. */
void CourseSelectMiniature::exeKinopioBrigadeEnter() {
    if (al::isFirstStep(this)) {
        al::setActionFrameRate(this, 1.0f);
        al::startAction(this, "Entrance");
    }

    // The end of the action is checked, but nothing is done with it.
    al::isActionEnd(this);
}

/** @brief Kills the miniature, its closed course model and its flag after the clear demo. */
void CourseSelectMiniature::exeClearHide() {
    if (al::isFirstStep(this)) {
        if (mCloseModel != nullptr) {
            al::hideModelIfShow(mCloseModel);
            mCloseModel->kill();
        }

        if (mFlag != nullptr) {
            mFlag->kill();
        }

        kill();
    }
}

/** @brief Waits locked by green stars until the lock opens. */
void CourseSelectMiniature::exeLockWait() {
    if (al::isFirstStep(this)) {
        if (al::isNerve(this, &NrvCourseSelectMiniatureLockWaitDisable)) {
            mLock->startDisable();
        } else {
            mLock->startShow();
        }

        if (!getStageInfo()->isKoopaCastle()) {
            showCloseModel();
        }
    }

    if (mLock->isUnlock()) {
        al::setNerve(this, &NrvCourseSelectMiniatureAppear);
        CourseInfoFunction::setOpen(GameDataHolderWriter(this), getCourseId());
    }
}
