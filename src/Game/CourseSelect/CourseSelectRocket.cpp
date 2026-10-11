#include "CourseSelect/CourseSelectRocket.hpp"

#include "CourseSelect/CourseSelectDirector.hpp"
#include "CourseSelect/CourseSelectMiniature.hpp"
#include "CourseSelect/CourseSelectPuppeteer.hpp"
#include "CourseSelect/CourseSelectPuppeteerGroup.hpp"
#include "CourseSelect/CourseSelectScene.hpp"
#include "CourseSelect/CourseSelectSensor.hpp"
#include "CourseSelect/CourseSelectWindowHolder.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/Draw/GraphicsSystemInfo.hpp"
#include "Library/LiveActor/SubActorUtil.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorAnimUtil.hpp"
#include "Library/LiveActor/Util/ActorClippingUtil.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorModelUtil.hpp"
#include "Library/LiveActor/Util/ActorMovementUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorSceneInfo.hpp"
#include "Library/LiveActor/Util/ActorSceneUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Play/Placement/PlacementInfo.hpp"
#include "Library/Resource/ResourceFunction.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/StageSwitch/StageSwitchFunc.hpp"
#include "MapObj/BindPuppeteer.hpp"
#include "Project/Base/StringUtil.hpp"
#include "System/GameDataFlagFunction.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/GameDataHolderWriter.hpp"
#include "Util/PlayerPuppetUtil.hpp"

namespace {
NERVE_DECL(CourseSelectRocket, Wait)
NERVE_DECL(CourseSelectRocket, Breakable)
NERVE_DECL(CourseSelectRocket, Rock)
NERVE_DECL(CourseSelectRocket, DemoOpen)
NERVE_DECL(CourseSelectRocket, DemoBreak)
NERVE_DECL(CourseSelectRocket, DemoBreakFade)
NERVE_DECL(CourseSelectRocket, DemoBreakFadeEnd)
NERVE_DECL(CourseSelectRocket, DemoStart)
NERVE_DECL(CourseSelectRocket, DemoExit)
NERVE_DECL(CourseSelectRocket, DemoLaunch)
NERVE_DECL(CourseSelectRocket, DemoLaunchFirst)
NERVE_DECL(CourseSelectRocket, DemoLand)
NERVE_DECL(CourseSelectRocket, DemoBreakFadeWait)
NERVE_DECL(CourseSelectRocket, DemoBreakInfo)
NERVE_DECL(CourseSelectRocket, DemoBeforeSave)
NERVE_DECL(CourseSelectRocket, DemoSave)
NERVES_MAKE_NOSTRUCT(CourseSelectRocket, Wait, Breakable, Rock, DemoOpen, DemoBreak, DemoBreakFade,
                     DemoBreakFadeEnd, DemoStart, DemoExit, DemoLaunch, DemoLaunchFirst, DemoLand,
                     DemoBreakFadeWait, DemoBreakInfo, DemoBeforeSave, DemoSave)

/** @brief Maximum number of fairy princesses (one per Bowser's castle) shown in the demos. */
constexpr s32 cFairyNumMax = 7;

/**
 * @brief Sets what the graphics areas of the scene follow.
 * @param pActor Actor of the scene.
 * @param target Graphics area target.
 */
void setGraphicsAreaTarget(const al::LiveActor* pActor, al::GraphicsAreaTarget::ValueType target) {
    pActor->getSceneInfo()->graphicsSystemInfo->setAreaTarget(target);
}
}  // namespace

/**
 * @brief Constructs the rocket.
 * @param pName Actor name.
 */
CourseSelectRocket::CourseSelectRocket(const char* pName) : al::LiveActor(pName) {}

/**
 * @brief Gets whether the world the rocket leads to is open.
 * @return true if it is open.
 */
inline bool CourseSelectRocket::isOpenWorld() const {
    if (mRocketType == cRocketType_SpecialWorld) {
        return GameDataFlagFunction::isAlreadyOpenCourseSelectRocket(GameDataHolderAccessor(this));
    }

    if (mRocketType == cRocketType_ChampionshipWorld) {
        return GameDataFlagFunction::isAlreadyOpenChampionshipWorld(GameDataHolderAccessor(this));
    }

    if (mRocketType == cRocketType_ArrangeWorld) {
        return GameDataFlagFunction::isAlreadyOpenArrangeWorld(GameDataHolderAccessor(this));
    }

    return false;
}

/**
 * @brief Initializes the rocket and its landing rocket. The rocket of the special worlds also
 * creates the fairy princesses and their tools shown when its rock breaks, the demo cameras and
 * the waves around its rock and its base. The starting nerve depends on whether the world the
 * rocket leads to is open.
 * @param rInfo Actor init info.
 */
void CourseSelectRocket::init(const al::ActorInitInfo& rInfo) {
    al::initActorSuffix(this, rInfo, mIsLandRocket ? "Linked" : nullptr);
    al::initNerve(this, &NrvCourseSelectRocketWait, 0);
    al::tryGetArg(reinterpret_cast<s32*>(&mRocketType), rInfo, "RocketType");
    al::startMclAnimAndSetFrameAndStop(this, "Color", mRocketType);
    mRock = al::tryGetSubActor(this, "ロケット岩");
    mRockBroken = al::tryGetSubActor(this, "ロケット岩[壊れ]");
    mBase = al::tryGetSubActor(this, "ロケット土台");
    mController = new RocketController(this);
    mDirector = CourseSelectDirector::getCourseSelectDirector(this);
    mSensor = new CourseSelectSensor(cCourseSelectSensorType_Rocket, mController);

    al::PlacementInfo landInfo;
    if (al::tryGetLinksInfo(&landInfo, al::getPlacementInfo(rInfo), "LandRocket")) {
        al::ActorInitInfo landInitInfo;
        landInitInfo.initViewIdSelf(&landInfo, rInfo);
        auto* pLandRocket = new CourseSelectRocket("コース選択ロケット[着陸地点]");
        mPairRocket = pLandRocket;
        pLandRocket->mPairRocket = this;
        pLandRocket->mIsLandRocket = true;
        pLandRocket->init(landInitInfo);
    }

    al::invalidateHitSensor(this, "OpenDemo");
    if (mIsLandRocket) {
        makeActorAppeared();
        return;
    }

    mIsOpen = isOpenWorld();

    if (mRocketType == cRocketType_SpecialWorld) {
        mFairies.allocBuffer(cFairyNumMax, nullptr);
        mFairyTools.allocBuffer(cFairyNumMax, nullptr);

        s32 fairyNum = mFairies.capacity();
        for (s32 i = 0; i < fairyNum; i++) {
            s32 worldId = i + 1;
            al::StringTmp<128> fairyName("FairyPrincess%02d", worldId);
            auto* pFairy = new al::LiveActor("妖精(ロケットデモ)");
            al::initActorWithArchiveNameNoPlacementInfo(pFairy, rInfo, fairyName, "Demo");
            al::resetPosition(pFairy, al::getTrans(this), false);
            pFairy->makeActorDead();

            al::StringTmp<128> toolName("");
            if (worldId <= 3) {
                toolName.format("FairyHummer");
            } else {
                toolName.format("FairyWrench");
            }

            auto* pTool = new al::LiveActor("妖精工具(ロケットデモ)");
            al::initActorWithArchiveNameNoPlacementInfo(pTool, rInfo, toolName, "DemoRocket");
            al::resetPosition(pTool, al::getTrans(this), false);
            pTool->makeActorDead();

            if (GameDataFlagFunction::isEnableDemoLeaveCastleFairyPrincess(
                    GameDataHolderAccessor(this), worldId)) {
                mFairies.pushBack(pFairy);
                mFairyTools.pushBack(pTool);
            } else {
                mFairyTools.pushBack(nullptr);
            }
        }

        CourseSelectDirector::getCourseSelectDirector(this)->setRocket(this);

        al::Resource* pCameraResource = al::findOrCreateResource("ObjectData/DemoCamera", nullptr);
        mCameraAfterEnding =
            al::initAnimCamera(this, rInfo, pCameraResource, "DemoAfterEnding", false);
        mCameraAppearance =
            al::initAnimCamera(this, rInfo, pCameraResource, "DemoRocketAppearance", false);
        mCameraShot = al::initAnimCamera(this, rInfo, pCameraResource, "DemoRocketShot", false);

        mWaveBase = new al::LiveActor("ロケット土台波");
        al::initActorWithArchiveNameNoPlacementInfo(mWaveBase, rInfo, "CourseSelectWaveRocketBase",
                                                    nullptr);
        al::resetPosition(mWaveBase, al::getTrans(this), false);
        mWaveBase->makeActorDead();

        mWaveRock = new al::LiveActor("ロケット岩波");
        al::initActorWithArchiveNameNoPlacementInfo(mWaveRock, rInfo, "CourseSelectWaveRocketRock",
                                                    nullptr);
        al::resetPosition(mWaveRock, al::getTrans(this), false);
        mWaveRock->makeActorDead();
    }

    bool isNewOpen = false;
    switch (mRocketType) {
    case cRocketType_SpecialWorld:
        isNewOpen = GameDataFunction::isEnableSpecialWorld(GameDataHolderAccessor(this)) &&
                    !GameDataFlagFunction::isAlreadyOpenCourseSelectRocket(
                        GameDataHolderAccessor(this));
        break;
    case cRocketType_ChampionshipWorld:
        isNewOpen = GameDataFlagFunction::isNewOpenChampionshipWorld(GameDataHolderAccessor(this));
        break;
    case cRocketType_ArrangeWorld:
        isNewOpen = GameDataFlagFunction::isNewOpenArrangeWorld(GameDataHolderAccessor(this));
        break;
    default:
        break;
    }

    if (mRocketType == cRocketType_SpecialWorld) {
        if (isNewOpen) {
            al::validateHitSensor(this, "OpenDemo");
            al::invalidateHitSensor(this, "Region");
            al::hideModel(this);
            mRock->appear();
            mWaveRock->appear();
            al::startAction(mWaveRock, "Wait");
            al::setNerve(this, &NrvCourseSelectRocketBreakable);
        } else if (mIsOpen) {
            mBase->appear();
            mWaveBase->appear();
            al::startAction(mWaveBase, "Wait");
            al::setNerve(this, &NrvCourseSelectRocketWait);
        } else {
            al::hideModel(this);
            mRock->appear();
            mWaveRock->appear();
            al::startAction(mWaveRock, "Wait");
            al::setNerve(this, &NrvCourseSelectRocketRock);
        }
    } else if (isNewOpen || mIsOpen) {
        mBase->appear();
        al::setNerve(this, &NrvCourseSelectRocketWait);
    } else {
        al::hideModel(this);
        mRock->appear();
        if (mRocketType == cRocketType_ArrangeWorld) {
            al::startAction(mRock, "ChampionshipSign");
            if (!GameDataFlagFunction::isEnableShowCourseSelectRocketRock(
                    GameDataHolderAccessor(this))) {
                al::hideModel(mRock);
            }
        } else {
            al::hideModel(mRock);
        }

        al::setNerve(this, &NrvCourseSelectRocketRock);
    }

    makeActorAppeared();
}

/**
 * @brief Kills the rocket, or delays the kill until the end of the puppet demo while a demo
 * plays.
 */
void CourseSelectRocket::kill() {
    if (isDemo()) {
        mIsKillReserved = true;
        return;
    }

    al::LiveActor::kill();
}

/**
 * @brief Gets whether the rocket plays a demo.
 * @return true while a demo plays.
 */
bool CourseSelectRocket::isDemo() const {
    return al::isNerve(this, &NrvCourseSelectRocketDemoOpen) ||
           al::isNerve(this, &NrvCourseSelectRocketDemoBreak) ||
           al::isNerve(this, &NrvCourseSelectRocketDemoBreakFade) ||
           al::isNerve(this, &NrvCourseSelectRocketDemoBreakFadeEnd) ||
           al::isNerve(this, &NrvCourseSelectRocketDemoStart) ||
           al::isNerve(this, &NrvCourseSelectRocketDemoExit) ||
           al::isNerve(this, &NrvCourseSelectRocketDemoLaunch) ||
           al::isNerve(this, &NrvCourseSelectRocketDemoLaunchFirst) ||
           al::isNerve(this, &NrvCourseSelectRocketDemoLand);
}

/**
 * @brief Pushes the players away from the map object sensor, and lets the players touching the
 * bind sensor select the rocket, or break its rock when it is breakable.
 * @param pSelf Sensor of the rocket.
 * @param pOther Sensor of the other actor.
 */
void CourseSelectRocket::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (!al::isSensorPlayer(pOther)) {
        return;
    }

    if (al::isSensorMapObj(pSelf)) {
        al::sendMsgPush(pOther, pSelf);
        return;
    }

    if (!al::isNerve(this, &NrvCourseSelectRocketWait) &&
        !al::isNerve(this, &NrvCourseSelectRocketBreakable)) {
        return;
    }

    if (!al::isSensorBindable(pSelf)) {
        return;
    }

    if (al::isNerve(this, &NrvCourseSelectRocketBreakable)) {
        mDirector->getPuppeteerGroup()->startRocketBreakDemo(mController);
        return;
    }

    mSensor->setSensor(pSelf);
    mDirector->touchPlayer(mSensor, al::getSensorHost(pOther));
}

/**
 * @brief Receives no message.
 * @param pMsg Message.
 * @param pSelf Sensor of the rocket.
 * @param pOther Sensor of the sender.
 * @return Always false.
 */
bool CourseSelectRocket::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSelf,
                                    al::HitSensor* pOther) {
    return false;
}

/** @brief Does nothing on its own: only checks whether the rock is in front of the rocket. */
void CourseSelectRocket::control() {
    if (al::isNerve(this, &NrvCourseSelectRocketRock)) {
        return;
    }
}

/** @brief Starts the demo shown after the ending, with the fairy princesses around the rocket. */
void CourseSelectRocket::startOpenDemo() {
    al::setNerve(this, &NrvCourseSelectRocketDemoOpen);
}

/**
 * @brief Starts a puppet demo: breaks the rock if it is breakable, otherwise launches the rocket.
 * @param pGroup Puppeteer group playing the demo.
 */
void CourseSelectRocket::startPuppetDemo(CourseSelectPuppeteerGroup* pGroup) {
    mDirector->getScene()->setNameplatesVisible(false);
    mPuppeteerGroup = pGroup;
    al::invalidateClipping(this);
    if (al::isNerve(this, &NrvCourseSelectRocketBreakable)) {
        al::setNerve(this, &NrvCourseSelectRocketDemoBreak);
        return;
    }

    al::setNerve(this, &NrvCourseSelectRocketDemoStart);
}

/** @brief Ends the puppet demo, and kills the rocket if a kill was requested during the demo. */
void CourseSelectRocket::endPuppetDemo() {
    mDirector->getScene()->setNameplatesVisible(true);
    mPuppeteerGroup = nullptr;
    al::validateClipping(this);
    al::setNerve(this, &NrvCourseSelectRocketWait);
    if (mIsKillReserved) {
        mIsKillReserved = false;
        kill();
    }
}

/**
 * @brief Binds a player to the rocket, warping it to the landing rocket.
 * @param pPuppeteer Puppeteer of the player.
 */
void CourseSelectRocket::startBind(CourseSelectPuppeteer* pPuppeteer) {
    if (al::isNerve(this, &NrvCourseSelectRocketBreakable)) {
        return;
    }

    pPuppeteer->startBindWarpActor(this, mPairRocket);
}

/**
 * @brief Starts the rocket demo when the main player decides on the rocket.
 * @param pDirector Course select director.
 * @return Whether the rocket demo started.
 */
bool CourseSelectRocket::tryDecide(const CourseSelectDirector* pDirector) {
    if (!pDirector->isEnableEnterSelectedSensor()) {
        return false;
    }

    if (!pDirector->isTriggerDecideMainPlayer()) {
        return false;
    }

    mDirector->getPuppeteerGroup()->startRocketDemo(mController);
    return true;
}

/**
 * @brief Makes the fairy princesses appear and starts an action on them.
 * @param pActionName Action name.
 */
inline void CourseSelectRocket::appearFairies(const char* pActionName) {
    s32 fairyNum = mFairies.size();
    for (s32 i = 0; i < fairyNum; i++) {
        mFairies(i)->appear();
        al::startAction(mFairies[i], pActionName);
    }
}

/** @brief Makes the tools of the fairy princesses appear with their appearance demo action. */
inline void CourseSelectRocket::appearFairyTools() {
    s32 toolNum = mFairyTools.size();
    for (s32 i = 0; i < toolNum; i++) {
        if (mFairyTools[i] == nullptr) {
            continue;
        }

        al::StringTmp<128> actionName("DemoRocketAppearance%02d", i + 1);
        mFairyTools(i)->appear();
        al::startAction(mFairyTools[i], actionName.cstr());
    }
}

/** @brief Ends the rocket demo of the puppeteers still playing it. */
inline void CourseSelectRocket::startPuppeteerRocketDemoEnd() {
    CourseSelectPuppeteerGroup* pGroup = mPuppeteerGroup;
    s32 puppeteerNum = pGroup->getPuppeteerNum();
    for (s32 i = 0; i < puppeteerNum; i++) {
        CourseSelectPuppeteer* pPuppeteer = pGroup->getPuppeteer(i);
        if (!pPuppeteer->isDemoEnd()) {
            pPuppeteer->startRocketDemoEnd();
        }
    }
}

/** @brief Waits with the rock in front of the rocket. */
void CourseSelectRocket::exeRock() {}

/** @brief Waits with the breakable rock in front of the rocket, the fairy princesses around it. */
void CourseSelectRocket::exeBreakable() {
    if (al::isFirstStep(this)) {
        appearFairies("WaitDemoAfterEnding");
    }
}

/** @brief Waits for the players. */
void CourseSelectRocket::exeWait() {}

/** @brief Plays the camera demo shown after the ending, with the fairy princesses. */
void CourseSelectRocket::exeDemoOpen() {
    if (al::isFirstStep(this)) {
        al::tryOnStageSwitch(this, "SwitchDemoAfterEndingOn");
        setGraphicsAreaTarget(this, al::GraphicsAreaTarget::CameraLookAt);
        al::makeMtxRT(&mDemoMtx, this);
        al::requestCancelInterpole(this);
        al::startAnimCamera(this, mCameraAfterEnding, "DemoAfterEnding", &mDemoMtx, -1);
        appearFairies("WaitDemoAfterEnding");
    }

    if (al::isEndAnimCamera(mCameraAfterEnding)) {
        al::setNerve(this, &NrvCourseSelectRocketWait);
    }
}

/** @brief Plays the demo of the fairy princesses breaking the rock, hiding the players. */
void CourseSelectRocket::exeDemoBreak() {
    if (al::isFirstStep(this)) {
        al::requestCaptureScreenCover(this, 4);
        al::tryOnStageSwitch(this, "SwitchDemoAppearanceOn");
        setGraphicsAreaTarget(this, al::GraphicsAreaTarget::CameraLookAt);
        al::startSequenceBgm(this, "DemoRocketOpen", -1, 0);
        return;
    }

    if (al::isStep(this, 1)) {
        al::makeMtxRT(&mDemoMtx, this);
        al::startAnimCamera(this, mCameraAppearance, "DemoRocketAppearance", &mDemoMtx, 0);
        appearFairies("DemoRocketAppearance");
        appearFairyTools();

        s32 puppeteerNum = mPuppeteerGroup->getPuppeteerNum();
        for (s32 i = 0; i < puppeteerNum; i++) {
            CourseSelectPuppeteer* pPuppeteer = mPuppeteerGroup->getPuppeteer(i);
            if (!pPuppeteer->isDemoEnd()) {
                rc::hidePuppet(pPuppeteer->getPlayerPuppet());
                rc::hidePuppetSilhouette(pPuppeteer->getPlayerPuppet());
                rc::hidePuppetShadow(pPuppeteer->getPlayerPuppet());
            }
        }
    }

    if (al::isEndAnimCamera(mCameraAppearance)) {
        al::setNerve(this, &NrvCourseSelectRocketDemoBreakFade);
    }
}

/** @brief Fades the screen to white at the end of the break demo. */
void CourseSelectRocket::exeDemoBreakFade() {
    CourseSelectWindowHolder* pWindowHolder = mDirector->getWindowHolder();
    if (al::isFirstStep(this)) {
        pWindowHolder->startFadeWhite(60);
        al::startSe(this, "PgWipeFadeWhite");
    }

    if (pWindowHolder->isCloseFadeWhite()) {
        al::setNerve(this, &NrvCourseSelectRocketDemoBreakFadeWait);
    }
}

/** @brief Waits while the screen is white. */
void CourseSelectRocket::exeDemoBreakFadeWait() {
    if (al::isStep(this, 0)) {
        al::startSe(this, "PgDemoBreakFadeCreate");
    }

    if (!al::isLessStep(this, 120)) {
        al::setNerve(this, &NrvCourseSelectRocketDemoBreakFadeEnd);
    }
}

/**
 * @brief Fades the screen back in on the rebuilt rocket: the fairy princesses go back to their
 * castles and the broken rock falls apart.
 */
void CourseSelectRocket::exeDemoBreakFadeEnd() {
    CourseSelectWindowHolder* pWindowHolder = mDirector->getWindowHolder();
    if (al::isFirstStep(this)) {
        pWindowHolder->endFadeWhite(15);

        s32 fairyNum = mFairies.size();
        for (s32 i = 0; i < fairyNum; i++) {
            mFairies(i)->kill();
            mDirector->findKoopaCastle(i + 1)->returnCastleFairyPrincess();
        }

        s32 toolNum = mFairyTools.size();
        for (s32 i = 0; i < toolNum; i++) {
            al::LiveActor* pTool = mFairyTools[i];
            if (pTool != nullptr) {
                pTool->kill();
            }
        }

        al::showModelIfHide(this);
        al::startAction(this, "Appear");
        mRock->kill();
        mWaveRock->kill();
        mRockBroken->appear();
        al::startAction(mRockBroken, "Break");
        mBase->appear();
        mWaveBase->appear();
        al::startAction(mWaveBase, "Wait");
        al::startAction(mBase, "Appear");
        al::startSequenceBgm(this, "MessageOpenWorld", -1, 0);
    }

    if (!al::isLessStep(this, 90) && al::isActionEnd(mRockBroken)) {
        mRockBroken->kill();
        al::invalidateHitSensor(this, "OpenDemo");
        al::validateHitSensor(this, "Region");
        al::setNerve(this, &NrvCourseSelectRocketDemoBreakInfo);
    }
}

/** @brief Ends the break demo: shows the players again and opens the rocket for good. */
void CourseSelectRocket::exeDemoBreakInfo() {
    // The first step check is kept by the original code although nothing is done on it.
    al::isFirstStep(this);
    if (al::isLessStep(this, 90)) {
        return;
    }

    al::startSequenceBgmWithAreaCheck(this, false, -1, 0, -1);

    s32 puppeteerNum = mPuppeteerGroup->getPuppeteerNum();
    for (s32 i = 0; i < puppeteerNum; i++) {
        CourseSelectPuppeteer* pPuppeteer = mPuppeteerGroup->getPuppeteer(i);
        if (!pPuppeteer->isDemoEnd()) {
            rc::showPuppet(pPuppeteer->getPlayerPuppet());
            rc::showPuppetSilhouette(pPuppeteer->getPlayerPuppet());
            rc::showPuppetShadow(pPuppeteer->getPlayerPuppet());
            pPuppeteer->startRocketDemoEnd();
        }
    }

    al::tryOffStageSwitch(this, "SwitchDemoAppearanceOn");
    setGraphicsAreaTarget(this, al::GraphicsAreaTarget::Player);
    al::endCamera(this, mCameraAppearance, -1);
    al::requestCaptureScreenCover(this, 2);
    al::changeBgmSituation(this, "CourseSelectPlay");
    GameDataFlagFunction::setOpenCourseSelectRocket(GameDataHolderWriter(this));
    al::setNerve(this, &NrvCourseSelectRocketWait);
}

/**
 * @brief Waits until all the players got in the rocket, then launches it. The first launch to the
 * special worlds plays a camera demo.
 */
void CourseSelectRocket::exeDemoStart() {
    if (al::isFirstStep(this)) {
        mRidePlayerFlags.makeAllZero();
        al::stopAllSequenceBgm(this, 180);
        al::startSe(this, "PgDecideEnter");
    }

    if (mPuppeteerGroup == nullptr) {
        return;
    }

    s32 puppeteerNum = mPuppeteerGroup->getPuppeteerNum();
    for (s32 i = 0; i < puppeteerNum; i++) {
        CourseSelectPuppeteer* pPuppeteer = mPuppeteerGroup->getPuppeteer(i);
        if (pPuppeteer->isDemoEnd()) {
            continue;
        }

        if (!pPuppeteer->isRocketDemoWarpWait()) {
            return;
        }

        if (mRidePlayerFlags.isOffBit(i)) {
            al::startAction(this, "Ride");
            mRidePlayerFlags.setBit(i);
        }
    }

    if (mRidePlayerFlags.isZero() || !al::isActionEnd(this)) {
        return;
    }

    bool isAllDemoEnd = true;
    for (s32 i = 0; i < puppeteerNum; i++) {
        if (!mPuppeteerGroup->getPuppeteer(i)->isDemoEnd()) {
            isAllDemoEnd = false;
            break;
        }
    }

    if (isAllDemoEnd) {
        return;
    }

    if (mRocketType == cRocketType_SpecialWorld && !mIsLandRocket &&
        !GameDataFlagFunction::isAlreadyFirstLaunchCourseSelectRocket(
            GameDataHolderAccessor(this))) {
        al::requestCaptureScreenCover(this, 4);
        al::tryOnStageSwitch(this, "SwitchDemoLaunchFirstOn");
        setGraphicsAreaTarget(this, al::GraphicsAreaTarget::CameraLookAt);
        al::setNerve(this, &NrvCourseSelectRocketDemoLaunchFirst);
        return;
    }

    al::setNerve(this, &NrvCourseSelectRocketDemoLaunch);
}

/** @brief Plays the camera demo of the first launch to the special worlds. */
void CourseSelectRocket::exeDemoLaunchFirst() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "DemoRocketShot");
        al::makeMtxRT(&mDemoMtx, this);
        al::startAnimCamera(this, mCameraShot, "DemoRocketShot", &mDemoMtx, 0);
        al::startSequenceBgm(this, "DemoRocketLaunch", -1, 0);
    }

    if (al::isEndAnimCamera(mCameraShot)) {
        al::endCamera(this, mCameraShot, 0);
        al::requestCaptureScreenCover(this, 4);
        al::tryOffStageSwitch(this, "SwitchDemoLaunchFirstOn");
        setGraphicsAreaTarget(this, al::GraphicsAreaTarget::Player);
        al::setNerve(this, &NrvCourseSelectRocketDemoLand);
    }
}

/** @brief Launches the rocket. */
void CourseSelectRocket::exeDemoLaunch() {
    if (al::isFirstStep(this)) {
        al::startAction(this, mIsLandRocket ? "DownStart" : "UpStart");
        al::stopAllSequenceBgm(this, 180);
    }

    if (al::isActionEnd(this)) {
        al::requestCaptureScreenCover(this, 4);
        al::setNerve(this, &NrvCourseSelectRocketDemoLand);
    }
}

/** @brief Warps the players to the other rocket and lands it. */
void CourseSelectRocket::exeDemoLand() {
    if (al::isFirstStep(this)) {
        al::requestCancelInterpole(this);

        s32 puppeteerNum = mPuppeteerGroup->getPuppeteerNum();
        for (s32 i = 0; i < puppeteerNum; i++) {
            CourseSelectPuppeteer* pPuppeteer = mPuppeteerGroup->getPuppeteer(i);
            if (!pPuppeteer->isDemoEnd()) {
                pPuppeteer->startRocketDemoWarp();
            }
        }

        al::startAction(mPairRocket, mIsLandRocket ? "DownEnd" : "UpEnd");
    }

    if (al::isActionEnd(mPairRocket)) {
        s32 puppeteerNum = mPuppeteerGroup->getPuppeteerNum();
        for (s32 i = 0; i < puppeteerNum; i++) {
            CourseSelectPuppeteer* pPuppeteer = mPuppeteerGroup->getPuppeteer(i);
            if (!pPuppeteer->isDemoEnd()) {
                pPuppeteer->startRocketDemoOut();
            }
        }

        al::startAction(this, "Wait");
        al::setNerve(this, &NrvCourseSelectRocketDemoExit);
    }
}

/**
 * @brief Waits until all the players got out of the rocket, then ends the demo, or shows the
 * message of the new world after the first launch.
 */
void CourseSelectRocket::exeDemoExit() {
    if (mPuppeteerGroup == nullptr) {
        return;
    }

    s32 puppeteerNum = mPuppeteerGroup->getPuppeteerNum();
    for (s32 i = 0; i < puppeteerNum; i++) {
        CourseSelectPuppeteer* pPuppeteer = mPuppeteerGroup->getPuppeteer(i);
        if (!pPuppeteer->isDemoEnd() && !pPuppeteer->isRocketDemoEndWait()) {
            return;
        }
    }

    if (!GameDataFlagFunction::isAlreadyFirstLaunchCourseSelectRocket(
            GameDataHolderAccessor(this))) {
        al::setNerve(this, &NrvCourseSelectRocketDemoBeforeSave);
        return;
    }

    al::startSequenceBgmWithAreaCheck(this, false, -1, 0, -1);
    startPuppeteerRocketDemoEnd();
    al::validateClipping(this);
    al::setNerve(this, &NrvCourseSelectRocketWait);
}

/** @brief Shows the message of the newly opened world before saving. */
void CourseSelectRocket::exeDemoBeforeSave() {
    CourseSelectWindowHolder* pWindowHolder = mDirector->getWindowHolder();
    if (al::isFirstStep(this)) {
        pWindowHolder->appearMessage("OpenWorldStar");
    }

    if (!pWindowHolder->isMessageActive()) {
        al::setNerve(this, &NrvCourseSelectRocketDemoSave);
    }
}

/** @brief Saves the first launch, then ends the demo. */
void CourseSelectRocket::exeDemoSave() {
    if (al::isFirstStep(this)) {
        GameDataFlagFunction::setFirstLaunchCourseSelectRocket(GameDataHolderWriter(this));
        GameDataFunction::playWorldStartDemo(GameDataHolderWriter(this),
                                             mDirector->getScene()->getWorldId());
        mPuppeteerGroup->startSaveDataWriteInDemo(mController);
        al::changeBgmSituation(this, "CourseSelectExitStage");
    }

    if (mPuppeteerGroup->isEndSaveDataWriteInDemo(mController)) {
        al::changeBgmSituation(this, "CourseSelectPlay");
        startPuppeteerRocketDemoEnd();
        al::validateClipping(this);
        al::setNerve(this, &NrvCourseSelectRocketWait);
    }
}
