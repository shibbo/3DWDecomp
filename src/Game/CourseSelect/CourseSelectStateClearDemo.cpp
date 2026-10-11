#include "CourseSelect/CourseSelectStateClearDemo.hpp"

#include <math/seadMatrix.h>
#include <math/seadQuat.h>
#include <math/seadVector.h>

#include "CourseSelect/CourseSelectActorInfo.hpp"
#include "CourseSelect/CourseSelectDirector.hpp"
#include "CourseSelect/CourseSelectMiniature.hpp"
#include "CourseSelect/CourseSelectNode.hpp"
#include "CourseSelect/CourseSelectPuppeteerGroup.hpp"
#include "CourseSelect/CourseSelectRouteDokan.hpp"
#include "CourseSelect/CourseSelectScene.hpp"
#include "CourseSelect/CourseSelectWindowHolder.hpp"
#include "CourseSelect/ICourseSelectActorController.hpp"
#include "CourseSelect/RouteOpenCursor.hpp"
#include "Layout/ListStampResult.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorSceneUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Play/Camera/CameraPoserZoomParam.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Bgm/BgmPlayingRequest.hpp"
#include "System/CourseInfoHolder.hpp"
#include "System/GameDataFlagFunction.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/GameDataHolderWriter.hpp"

/**
 * @brief Gets the game data of the scene.
 * @return The game data holder.
 */
inline GameDataHolder* CourseSelectStateClearDemo::getGameDataHolder() const {
    return getHost()->mGameDataHolder;
}

namespace {
NERVE_DECL(CourseSelectStateClearDemo, FirstClearDemo)
NERVE_DECL(CourseSelectStateClearDemo, OpenCasinoRoomStart)
NERVE_DECL(CourseSelectStateClearDemo, Kill)
NERVE_DECL(CourseSelectStateClearDemo, WaitCamera)
NERVE_DECL(CourseSelectStateClearDemo, FirstClearDemoEnd)
NERVE_DECL(CourseSelectStateClearDemo, WorldClear)
NERVE_DECL(CourseSelectStateClearDemo, OpenRoad)
NERVE_DECL(CourseSelectStateClearDemo, OpenRouteDokanRoad)
NERVE_DECL(CourseSelectStateClearDemo, OpenRouteDokan)
NERVE_DECL(CourseSelectStateClearDemo, OpenMiniature)
NERVE_DECL(CourseSelectStateClearDemo, OpenCasinoRoom)
NERVE_DECL(CourseSelectStateClearDemo, ShowNewOpenFlagMessage)
NERVE_DECL(CourseSelectStateClearDemo, ShowGetStamp)
NERVE_DECL(CourseSelectStateClearDemo, EndNewOpenFlagMessage)
NERVES_MAKE_NOSTRUCT(CourseSelectStateClearDemo, FirstClearDemo, OpenCasinoRoomStart, Kill,
                     WaitCamera, FirstClearDemoEnd, WorldClear, OpenRoad, OpenRouteDokanRoad,
                     OpenRouteDokan, OpenMiniature, OpenCasinoRoom, ShowNewOpenFlagMessage,
                     ShowGetStamp, EndNewOpenFlagMessage)

/** @brief Bgm line played while the message of an opened feature is shown. */
struct NewOpenFlagBgm {
    const char* mMessageLabel;
    const char* mBgmLineName;
};

const NewOpenFlagBgm cNewOpenFlagBgmTable[] = {
    {"OpenWorldArrange", "MessageOpenWorld"},
    {"OpenWorldChampionship", "MessageOpenWorldChampionship"},
    {"OpenAllClearMario", "MessageOpenEtc"},
    {"OpenAllClearLuigi", "MessageOpenEtc"},
    {"OpenAllClearPeach", "MessageOpenEtc"},
    {"OpenAllClearKinopio", "MessageOpenEtc"},
    {"OpenAllClearRosetta", "MessageOpenEtc"},
    {"OpenRosettaPlayable", "MessageOpenEtc"},
    {"OpenShowBestTime", "MessageOpenEtc"},
    {"OpenShowTimeAttackGhost", "MessageOpenEtc"},
};

constexpr s32 cNewOpenFlagBgmNum = sizeof(cNewOpenFlagBgmTable) / sizeof(cNewOpenFlagBgmTable[0]);

/**
 * @brief Finds the bgm line played with the message of an opened feature.
 * @param pLabel Message label of the feature.
 * @return The bgm line, or nullptr if the message plays none.
 */
inline const char* findNewOpenFlagBgmLine(const char* pLabel) {
    for (s32 i = 0; i < cNewOpenFlagBgmNum; i++) {
        if (al::isEqualString(cNewOpenFlagBgmTable[i].mMessageLabel, pLabel)) {
            return cNewOpenFlagBgmTable[i].mBgmLineName;
        }
    }

    return nullptr;
}

/**
 * @brief Finds the character whose all-clear stamp a message announces.
 * @param pLabel Message label of the opened feature.
 * @return The character index, or -1 if the message announces no stamp.
 */
inline s32 findCompleteStampCharacter(const char* pLabel) {
    if (al::isEqualString(pLabel, "OpenAllClearMario")) {
        return 0;
    }

    if (al::isEqualString(pLabel, "OpenAllClearLuigi")) {
        return 1;
    }

    if (al::isEqualString(pLabel, "OpenAllClearPeach")) {
        return 2;
    }

    if (al::isEqualString(pLabel, "OpenAllClearKinopio")) {
        return 3;
    }

    if (al::isEqualString(pLabel, "OpenAllClearRosetta")) {
        return 4;
    }

    return -1;
}

}  // namespace

/**
 * @brief Creates the clear demo state.
 * @param pScene Course select scene owning the state.
 * @param pPlayerHolder Players of the scene.
 * @param pCameraInfo Camera info of the scene.
 * @param rInfo Layout init info, used to create the stamp result layout.
 */
CourseSelectStateClearDemo::CourseSelectStateClearDemo(CourseSelectScene* pScene,
                                                       const al::PlayerHolder* pPlayerHolder,
                                                       al::SceneCameraInfo* pCameraInfo,
                                                       const al::LayoutInitInfo& rInfo)
    : al::HostStateBase<CourseSelectScene>("クリアデモ", pScene), mPlayerHolder(pPlayerHolder),
      mSceneCameraInfo(pCameraInfo), mWindowHolder(pScene->mWindowHolder) {
    mNextNodes = new ICourseSelectActorController*[cNextNodeNumMax];
    mRouteOpenCursor = new RouteOpenCursor(pScene->getCourseSelectDirector());
    mListStampResult = new ListStampResult(rInfo, getGameDataHolder());
}

/**
 * @brief Initializes the nerve and the camera of the demos.
 */
void CourseSelectStateClearDemo::init() {
    initNerve(&NrvCourseSelectStateClearDemoFirstClearDemo, 1);
    mCameraInfo = al::initProgramableCamera(this, "MiniatureOpen");
}

/**
 * @brief Starts the demos of the last played course, or ends the state at once if it has none.
 */
void CourseSelectStateClearDemo::appear() {
    al::NerveStateBase::appear();
    s32 courseId = GameDataFunction::getLastPlayCourseId(
        GameDataHolderAccessor(getGameDataHolder()));
    mMiniature = getHost()->getCourseSelectDirector()->tryFindMiniatureObj(courseId);
    if (mMiniature == nullptr) {
        kill();
        return;
    }

    if (GameDataFunction::isLastPlayCourseFirstClear(GameDataHolderAccessor(mMiniature))) {
        al::setNerve(this, &NrvCourseSelectStateClearDemoFirstClearDemo);
    } else {
        goToOpenCasinoRoomOrKill();
    }
}

/**
 * @brief Moves the camera back to the map and ends the state once it got there.
 */
void CourseSelectStateClearDemo::goToKill() {
    if (mMiniature != nullptr && mMiniature->isKinopioHouse()) {
        mEndCameraInterpoleFrame = 1;
    } else {
        mEndCameraInterpoleFrame = cEndCameraInterpoleFrame;
    }

    al::endCamera(this, mCameraInfo, mEndCameraInterpoleFrame);
    al::setNerve(this, &NrvCourseSelectStateClearDemoWaitCamera);
}

/**
 * @brief Checks whether the roads to all the nodes opened by the clear were opened.
 * @return true if all were opened.
 */
bool CourseSelectStateClearDemo::isFinishOpenNextMiniature() const {
    return mNextNodeNum <= mOpenedNodeNum;
}

/**
 * @brief Opens the casino room of the world if the clear opened it, ends the state otherwise.
 */
inline void CourseSelectStateClearDemo::goToOpenCasinoRoomOrKill() {
    if (GameDataFunction::isNeedOpenCasinoRoom(GameDataHolderAccessor(getGameDataHolder()))) {
        s32 worldId = mMiniature->getWorldId();
        CourseSelectMiniature* casinoRoom =
            getHost()->getCourseSelectDirector()->tryFindCasinoRoom(worldId);
        if (casinoRoom != nullptr && !casinoRoom->isCourseOpen()) {
            al::setNerve(this, &NrvCourseSelectStateClearDemoOpenCasinoRoomStart);
            return;
        }
    }

    al::setNerve(this, &NrvCourseSelectStateClearDemoKill);
}

/**
 * @brief Plays the clear demo of a course cleared for the first time, then raises its flag.
 */
void CourseSelectStateClearDemo::exeFirstClearDemo() {
    if (al::isFirstStep(this)) {
        mMiniature->startClearDemo();
        mOpenedNodeNum = 0;
        mNextNodeNum = mMiniature->tryFindNextNode(mNextNodes, cNextNodeNumMax);
    }

    s32 waitStep;
    if (GameDataFunction::isStageGateKeeperGoalPole(GameDataHolderAccessor(mMiniature),
                                                    mMiniature->getCourseId())) {
        waitStep = 130;
    } else if (mMiniature->isGateKeeper()) {
        waitStep = 90;
    } else {
        waitStep = mMiniature->getFlag() != nullptr ? 30 : 0;
    }

    if (!mMiniature->isPlayingClearDemo() && al::isGreaterEqualStep(this, waitStep)) {
        mMiniature->startClearFlagDemo();
        al::setNerve(this, &NrvCourseSelectStateClearDemoFirstClearDemoEnd);
    }
}

/**
 * @brief Goes on with the world clear demo, the route dokan or the roads opened by the clear.
 */
void CourseSelectStateClearDemo::exeFirstClearDemoEnd() {
    if (!al::isGreaterEqualStep(this, 1)) {
        return;
    }

    if (mMiniature->getDemoWorldClear() != nullptr) {
        al::setNerve(this, &NrvCourseSelectStateClearDemoWorldClear);
    } else if (mMiniature->getRouteDokan() == nullptr) {
        if (!isFinishOpenNextMiniature()) {
            al::setNerve(this, &NrvCourseSelectStateClearDemoOpenRoad);
        } else {
            goToOpenCasinoRoomOrKill();
        }
    } else {
        al::setNerve(this, &NrvCourseSelectStateClearDemoOpenRouteDokanRoad);
    }
}

/**
 * @brief Plays the world clear demo, then opens the roads to the next courses.
 */
void CourseSelectStateClearDemo::exeWorldClear() {
    if (al::isFirstStep(this)) {
        mMiniature->startWorldClearDemo();
    }

    if (mMiniature->isEndWorldClearDemo()) {
        if (!isFinishOpenNextMiniature()) {
            al::setNerve(this, &NrvCourseSelectStateClearDemoOpenRoad);
        } else {
            goToOpenCasinoRoomOrKill();
        }
    }
}

/**
 * @brief Shows the route dokan opened by the clear.
 */
void CourseSelectStateClearDemo::exeOpenRouteDokanStart() {
    if (al::isFirstStep(this)) {
        startDemoCamera(nullptr, mMiniature->getRouteDokan(), -1);
    }

    if (al::isGreaterEqualStep(this, 10)) {
        al::setNerve(this, &NrvCourseSelectStateClearDemoOpenRouteDokanRoad);
    }
}

/**
 * @brief Starts the demo camera looking down at an actor, or between two actors.
 * @param pSubTarget Actor the camera also looks at, or nullptr.
 * @param pTarget Actor the camera looks at.
 * @param interpoleFrame Number of frames of the interpolation to the demo camera.
 */
void CourseSelectStateClearDemo::startDemoCamera(const al::LiveActor* pSubTarget,
                                                 const al::LiveActor* pTarget,
                                                 s32 interpoleFrame) {
    al::CameraInfo* cameraInfo = mCameraInfo;
    sead::Vector3f dir = sead::Vector3f::ez;
    f32 angleV = al::getZoomParamNormal(pTarget).mAngleV;
    sead::Quatf quat;
    quat.setAxisAngle(-sead::Vector3f::ex, angleV);
    sead::Matrix34f mtx;
    mtx.makeQT(quat, {0.0f, 0.0f, 0.0f});
    dir = mtx * dir;

    sead::Vector3f lookAt;
    if (pSubTarget != nullptr) {
        lookAt = al::getTrans(pSubTarget) * 0.5f + al::getTrans(pTarget) * 0.5f;
    } else {
        lookAt = al::getTrans(pTarget);
    }

    al::setCameraLookAtPos(cameraInfo, lookAt);
    al::setCameraPos(cameraInfo, dir * 4500.0f + lookAt);
    al::setCameraFovyDegree(cameraInfo, al::getCameraFovyDegree(this));
    al::startCamera(this, cameraInfo, interpoleFrame);
}

/**
 * @brief Opens the road to the route dokan opened by the clear.
 */
void CourseSelectStateClearDemo::exeOpenRouteDokanRoad() {
    if (al::isFirstStep(this)) {
        CourseSelectNode* nodeEnd = mMiniature->tryFindNextNodeEnd();
        if (nodeEnd == nullptr) {
            al::setNerve(this, &NrvCourseSelectStateClearDemoOpenRouteDokan);
            return;
        }

        if (!mRouteOpenCursor->initialize(mMiniature->getNode(), nodeEnd)) {
            al::setNerve(this, &NrvCourseSelectStateClearDemoOpenRouteDokan);
        }
    }

    if (mRouteOpenCursor->update()) {
        al::setNerve(this, &NrvCourseSelectStateClearDemoOpenRouteDokan);
    }
}

/**
 * @brief Opens the route dokan, then the roads to the next courses.
 */
void CourseSelectStateClearDemo::exeOpenRouteDokan() {
    if (al::isStep(this, 120)) {
        mMiniature->startOpenRouteDokan();
    }

    if (al::isGreaterEqualStep(this, 120)) {
        al::endCamera(this, mCameraInfo, -1);
        if (!isFinishOpenNextMiniature()) {
            al::setNerve(this, &NrvCourseSelectStateClearDemoOpenRoad);
        } else {
            goToOpenCasinoRoomOrKill();
        }
    }
}

/**
 * @brief Opens the road to the next node opened by the clear.
 */
void CourseSelectStateClearDemo::exeOpenRoad() {
    if (al::isFirstStep(this)) {
        ICourseSelectActorController* node = mNextNodes[mOpenedNodeNum++];
        mOpeningNode = node;
        mIsGreenStarLock = false;
        const CourseSelectActorInfo* info = node->getCourseSelectActorInfo();
        if (info != nullptr &&
            (CourseInfoFunction::isOpen(GameDataHolderAccessor(getGameDataHolder()),
                                        info->getCourseId()) ||
             CourseInfoFunction::isGreenStarLock(
                GameDataHolderAccessor(getGameDataHolder()), info->getCourseId()))) {
            mIsGreenStarLock = true;
        }

        getHost()->getCourseSelectDirector()->getPuppeteerGroup()->startOpenRoadDemo(node);
        if (!node->getCourseSelectNode()->isRoadReached()) {
            startDemoCamera(mMiniature, node->getActor(), cEndCameraInterpoleFrame);
        }

        if (!mRouteOpenCursor->initialize(mMiniature->getNode(), node->getCourseSelectNode())) {
            al::setNerve(this, &NrvCourseSelectStateClearDemoOpenMiniature);
        }
    }

    if (mRouteOpenCursor->update()) {
        al::setNerve(this, &NrvCourseSelectStateClearDemoOpenMiniature);
    }
}

/**
 * @brief Opens the miniature at the end of the opened road.
 */
void CourseSelectStateClearDemo::exeOpenMiniature() {
    // The first step check is kept although nothing is done on it.
    al::isFirstStep(this);
    const CourseSelectActorInfo* info = mOpeningNode->getCourseSelectActorInfo();

    s32 waitStep;
    if (mIsGreenStarLock) {
        waitStep = 15;
    } else if (info == nullptr) {
        waitStep = 60;
    } else if (info->isEnterGateKeeper()) {
        waitStep = 180;
    } else {
        waitStep = CourseInfoFunction::isGreenStarLock(
                       GameDataHolderAccessor(getGameDataHolder()), info->getCourseId()) ?
                       180 :
                       60;
    }

    if (al::isGreaterEqualStep(this, waitStep)) {
        if (!isFinishOpenNextMiniature()) {
            al::setNerve(this, &NrvCourseSelectStateClearDemoOpenRoad);
        } else {
            goToOpenCasinoRoomOrKill();
        }
    }
}

/**
 * @brief Shows where the casino room of the world appears.
 */
void CourseSelectStateClearDemo::exeOpenCasinoRoomStart() {
    if (al::isStep(this, 5)) {
        al::startSe(getHost(), "PgBonusRoomAppearSign");
    }

    if (al::isStep(this, 40)) {
        CourseSelectMiniature* casinoRoom =
            getHost()->getCourseSelectDirector()->tryFindCasinoRoom(mMiniature->getWorldId());
        al::requestCaptureScreenCover(casinoRoom, 4);
        startDemoCamera(nullptr, casinoRoom, 0);
    }

    if (al::isGreaterEqualStep(this, 100)) {
        al::setNerve(this, &NrvCourseSelectStateClearDemoOpenCasinoRoom);
    }
}

/**
 * @brief Makes the casino room of the world appear.
 */
void CourseSelectStateClearDemo::exeOpenCasinoRoom() {
    if (al::isFirstStep(this)) {
        CourseSelectMiniature* casinoRoom =
            getHost()->getCourseSelectDirector()->tryFindCasinoRoom(mMiniature->getWorldId());
        al::startSe(getHost(), "PgBonusRoomAppearHit");
        casinoRoom->startAppear();
        GameDataFunction::resetCasinoRoomCounter(
            GameDataHolderWriter(getGameDataHolder()));
        al::startSequenceBgm(getHost(), "AppearRouletteRoom", -1, 10);
        al::changeBgmSituation(getHost(), "CourseSelectExitStage");
    }

    if (al::isStep(this, 68)) {
        al::startSe(getHost(), "PgBonusRoomAppear");
    }

    if (al::isGreaterEqualStep(this, 130)) {
        goToKill();
    }
}

/**
 * @brief Waits for the camera to be back to the map, then shows the messages of the features
 * opened by the clear or ends the state.
 */
void CourseSelectStateClearDemo::exeWaitCamera() {
    if (!al::isGreaterEqualStep(this, mEndCameraInterpoleFrame + 1)) {
        return;
    }

    if (GameDataFlagFunction::isNewOpenNetworkSetting(
            GameDataHolderAccessor(getGameDataHolder()))) {
        GameDataFlagFunction::setOpenNetworkSetting(
            GameDataHolderWriter(getGameDataHolder()));
    }

    if (GameDataFlagFunction::isExistNewOpenFlag(
            GameDataHolderAccessor(getGameDataHolder()), true)) {
        al::setNerve(this, &NrvCourseSelectStateClearDemoShowNewOpenFlagMessage);
        return;
    }

    getHost()->getCourseSelectDirector()->getPuppeteerGroup()->endDemo();
    kill();
}

/**
 * @brief Runs the Miiverse posting state, then shows the messages of the opened features or ends
 * the state.
 */
void CourseSelectStateClearDemo::exeStartMiiverse() {
    if (al::isFirstStep(this)) {
        al::changeBgmSituation(getHost(), "CourseSelectExitStage");
    }

    if (!al::updateNerveState(this)) {
        return;
    }

    if (GameDataFlagFunction::isExistNewOpenFlag(
            GameDataHolderAccessor(getGameDataHolder()), true)) {
        al::setNerve(this, &NrvCourseSelectStateClearDemoShowNewOpenFlagMessage);
        return;
    }

    al::changeBgmSituation(getHost(), "CourseSelectPlay");
    getHost()->getCourseSelectDirector()->getPuppeteerGroup()->endDemo();
    kill();
}

/**
 * @brief Shows the message of the first feature opened by the clear.
 */
void CourseSelectStateClearDemo::exeShowNewOpenFlagMessage() {
    if (al::isFirstStep(this)) {
        mBgmLineName = nullptr;
        CourseSelectWindowHolder* windowHolder = mWindowHolder;
        const char* label = GameDataFlagFunction::tryGetFirstNewOpenFlagMessageLabelByStageClear(
            GameDataHolderAccessor(getGameDataHolder()));
        if (label == nullptr) {
            goToKill();
            return;
        }

        windowHolder->appearMessage(label);
        mMessageLabel = GameDataFlagFunction::tryGetFirstNewOpenFlagMessageLabelByStageClear(
            GameDataHolderAccessor(getGameDataHolder()));
        if (mMessageLabel != nullptr) {
            mBgmLineName = findNewOpenFlagBgmLine(mMessageLabel);
            if (mBgmLineName != nullptr) {
                al::BgmPlayingRequest request(mBgmLineName, -1, 30);
                request._15 = true;
                al::startSequenceBgm(getHost(), request);
                al::changeLineAutoStopMode(getHost(), mBgmLineName, true);
            } else {
                al::changeBgmSituation(getHost(), "CourseSelectExitStage");
            }
        }
    }

    if (mWindowHolder->isMessageActive()) {
        return;
    }

    GameDataFlagFunction::setFirstAlreadyOpenFlagByStageClear(
        GameDataHolderWriter(getGameDataHolder()));
    if (findCompleteStampCharacter(mMessageLabel) != -1) {
        al::setNerve(this, &NrvCourseSelectStateClearDemoShowGetStamp);
    } else {
        al::setNerve(this, &NrvCourseSelectStateClearDemoEndNewOpenFlagMessage);
    }
}

/**
 * @brief Shows the message of the next opened feature, or ends the state.
 */
void CourseSelectStateClearDemo::exeEndNewOpenFlagMessage() {
    if (GameDataFlagFunction::isExistNewOpenFlag(
            GameDataHolderAccessor(getGameDataHolder()), true)) {
        al::setNerve(this, &NrvCourseSelectStateClearDemoShowNewOpenFlagMessage);
        return;
    }

    getHost()->getCourseSelectDirector()->getPuppeteerGroup()->endDemo();
    if (mBgmLineName != nullptr &&
        !GameDataFlagFunction::isExistNewOpenFlag(
            GameDataHolderAccessor(getGameDataHolder()), true)) {
        al::changeLineAutoStopMode(getHost(), mBgmLineName, false);
        mBgmLineName = nullptr;
        al::changeBgmSituation(getHost(), "CourseSelectPlay");
    }

    kill();
}

/**
 * @brief Shows the stamp got for clearing everything with a character.
 */
void CourseSelectStateClearDemo::exeShowGetStamp() {
    if (al::isFirstStep(this)) {
        mListStampResult->startAppearCharacterComplete(findCompleteStampCharacter(mMessageLabel));
    }

    if (mListStampResult->isEnd()) {
        al::setNerve(this, &NrvCourseSelectStateClearDemoEndNewOpenFlagMessage);
    }
}

/**
 * @brief Ends the state.
 */
void CourseSelectStateClearDemo::exeKill() {
    goToKill();
}
