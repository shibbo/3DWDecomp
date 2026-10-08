#include "Demo/DemoCutscene.hpp"

#include <math/seadMatrix.h>
#include <prim/seadSafeString.h>

#include "Demo/DemoSceneActorHolder.hpp"
#include "Layout/GuideGameWindow.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorSceneUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Play/Layout/WipeSimple.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Play/Placement/PlacementInfo.hpp"
#include "Library/StageSwitch/StageSwitchFunc.hpp"
#include "MapObj/BindPuppeteer.hpp"
#include "MapObj/BindPuppeteerGroup.hpp"
#include "MapObj/IslandKeeper.hpp"
#include "Player/Normal/PlayerKoopaJr.hpp"
#include "Project/AreaObj/AreaObj.hpp"
#include "Project/AreaObj/AreaObjGroup.hpp"
#include "Project/AreaObj/AreaObjUtil.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/GameDataHolderWriter.hpp"
#include "System/SaveDataAccessFunction.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/DemoUtil.hpp"
#include "Util/PlayerPuppetUtil.hpp"
#include "Util/PlayerUtil.hpp"

namespace {
NERVE_DECL(DemoCutscene, BindWait);
NERVE_DECL(DemoCutscene, WaitOnGround);
NERVE_DECL(DemoCutscene, WaitCutsceneStart);
NERVE_DECL(DemoCutscene, BindEndWait);
NERVE_DECL(DemoCutscene, FinalEndDemo);
NERVE_DECL(DemoCutscene, AllBindDone);
NERVE_DECL(DemoCutscene, GuideWindow);
NERVE_DECL(DemoCutscene, FadeOutGuideWindow);
NERVE_DECL(DemoCutscene, EndDemo);
NERVE_DECL(DemoCutscene, BindEndFade);
NERVE_DECL(DemoCutscene, BindEnd);
NERVE_DECL(DemoCutscene, FadeToBlackStart);
NERVES_MAKE_NOSTRUCT(DemoCutscene, BindWait, WaitOnGround, WaitCutsceneStart, BindEndWait,
                     FinalEndDemo, AllBindDone, GuideWindow, FadeOutGuideWindow, EndDemo,
                     BindEndFade, BindEnd, FadeToBlackStart)

/** @brief Cutscene ids (as saved in the single mode data) with special handling. */
enum CutsceneId : s32 {
    CutsceneId_GigaBellPhase1 = 9,
    CutsceneId_GigaBellPhase2 = 10,
    CutsceneId_GigaBellPhase3 = 11,
    CutsceneId_GigaBellResealed = 12,
    CutsceneId_PhaseReturnGuidePhase2 = 15,
    CutsceneId_PhaseReturnGuidePhase3 = 17,
    CutsceneId_FinalFightCheer = 20,
    CutsceneId_PhaseStartGuidePhase2PartA = 24,
    CutsceneId_PhaseStartGuidePhase2PartB = 25,
    CutsceneId_PhaseStartGuidePhase3 = 26,
    CutsceneId_PhaseStartGuidePhase4PartA = 27,
    CutsceneId_PhaseStartGuidePhase4PartB = 28,
};

/** @brief Demo type that keeps the current demo audio type. */
constexpr s32 cDemoTypeNoAudio = 5;
}  // namespace

/**
 * @brief Construct the cutscene actor.
 * @param pName Name of the actor (and of the cutscene resource).
 * @param type Audio type of the demo.
 */
DemoCutscene::DemoCutscene(const char* pName, alSeFunction::DemoType type)
    : DemoObjBase(pName), mDemoType(type) {
    mIsEndAtCutscenePos = false;
    mIsBindPlayer = true;
    mIsFollowedByDemo = false;
}

/**
 * @brief Read the placement parameters, create the bind puppeteers and the linked camera area.
 * @param rInfo Actor initialization data.
 */
void DemoCutscene::init(const al::ActorInitInfo& rInfo) {
    DemoObjBase::init(rInfo);
    al::tryGetArg(&mIsRequireOnGround, rInfo, "RequireOnGround");
    al::tryGetArg(&mIsEndOnGround, rInfo, "EndOnGround");
    initPuppets(rInfo);
    mEndTrans = sead::Vector3f::zero;
    mEndFront = sead::Vector3f::ez;
    al::tryGetArg(&mIsEndPhaseWhenDone, rInfo, "EndPhaseWhenDone");
    if (mIsEndPhaseWhenDone) {
        setEndSceneFlag();
    }

    al::tryGetArg(&mCutsceneId, rInfo, "UnlockFlag");
    mCutsceneId = rc::convertMapUnitFlagToCutscene(mCutsceneId);
    if (al::calcLinkChildNum(rInfo, "PlayerRestartPos") > 0) {
        mPlayerRestartInfo = al::createLinksPlayerActorInfo(this, rInfo);
    }

    if (al::calcLinkChildNum(rInfo, "CameraArea") > 0) {
        al::PlacementInfo placementInfo;
        al::getLinksInfoByIndex(&placementInfo, rInfo.getPlacementInfo(), "CameraArea", 0);
        al::AreaInitInfo areaInitInfo(placementInfo, rInfo.getStageSwitchDirector());
        mCameraArea = new al::AreaObj("CameraArea");
        mCameraArea->init(areaInitInfo);
        mCameraArea->invalidate();

        al::AreaObjGroup* group = al::tryFindAreaObjGroup(this, "CameraArea");
        if (group != nullptr) {
            group->resisterAreaObj(mCameraArea);
        }
    }
}

/**
 * @brief Create the bindable sensor and one bind puppeteer per controller port.
 * @param rInfo Actor initialization data.
 */
void DemoCutscene::initPuppets(const al::ActorInitInfo& rInfo) {
    initHitSensor(1);
    al::addHitSensorBindableGoal(this, rInfo, "Bindable", 0.0f, 0, sead::Vector3f::zero);
    mPuppeteerGroup =
        new BindPuppeteerGroup("CutsceneBindPuppeteerGroup", al::getMaxControllerPorts());
    for (s32 i = 0; i < mPuppeteerGroup->getPuppeteerNumMax(); i++) {
        auto* puppeteer = new BindPuppeteer("CutsceneBindPuppeteer");
        mPuppeteerGroup->registerPuppeteer(puppeteer);
    }
}

/**
 * @brief Create the black fade played around the cutscene.
 * @param rInfo Actor initialization data.
 * @param isUseWipeFade Whether the fade is played when the cutscene ends.
 * @param frames Duration of the fade in frames.
 */
void DemoCutscene::createWipeFade(const al::ActorInitInfo& rInfo, bool isUseWipeFade,
                                  s32 frames) {
    mIsUseWipeFade = isUseWipeFade;
    mWipeFade =
        new al::WipeSimple("WipeFadeBlack", "WipeFadeBlack", al::getLayoutInitInfo(rInfo), "Demo");
    mWipeFrames = frames;
}

/**
 * @brief Place the players at the given pose when the cutscene ends.
 * @param rTrans Position of the players.
 * @param rFront Front direction of the players.
 */
void DemoCutscene::setEndAtCutscenePos(const sead::Vector3f& rTrans,
                                       const sead::Vector3f& rFront) {
    mEndTrans = rTrans;
    mEndFront = rFront;
    mIsEndAtCutscenePos = true;
}

/** @brief End the current phase once the cutscene is done. */
void DemoCutscene::setEndSceneFlag() {
    mIsEndPhaseWhenDone = true;
    mEndFrameWindow = 5;
}

/**
 * @brief Request the cutscene demo from the scene.
 * @return Whether the demo could be started.
 */
bool DemoCutscene::requestStartDemo() {
    if (mDemoType != cDemoTypeNoAudio) {
        rc::setDemoAudioType(this, mDemoType);
    }

    if (_32b) {
        return true;
    }

    if (mIsBindPlayer) {
        return rc::requestStartDemoPlayerCutscene(this);
    }

    return rc::requestStartDemoCutscene(this);
}

/** @brief Bind the players first if needed, otherwise start the cutscene right away. */
void DemoCutscene::tryStartDemo() {
    if (mIsBindPlayer) {
        al::setNerve(this, &NrvDemoCutsceneBindWait);
        return;
    }

    trueStartDemo();
}

/** @brief Start the cutscene unless it was already seen, waiting for the players if needed. */
void DemoCutscene::startDemo() {
    if (mCutsceneId >= 0 && SingleModeDataFunction::hasSeenCutscene(this, mCutsceneId)) {
        return;
    }

    if (mIsRequireOnGround && !rc::isAllPlayerOnGroundOrWater(this)) {
        al::setNerve(this, &NrvDemoCutsceneWaitOnGround);
        return;
    }

    if (!requestStartDemo()) {
        al::setNerve(this, &NrvDemoCutsceneWaitCutsceneStart);
        return;
    }

    tryStartDemo();
}

/** @brief Wait until the scene accepts the cutscene demo. */
void DemoCutscene::exeWaitCutsceneStart() {
    if (mIsInvalidatePlayerInput) {
        rc::invalidatePlayerInput(this, 4);
    }

    if (requestStartDemo()) {
        tryStartDemo();
    }
}

/** @brief Wait until every player stands on the ground before starting the cutscene. */
void DemoCutscene::exeWaitOnGround() {
    if (mIsInvalidatePlayerInput) {
        rc::invalidatePlayerInput(this, 4);
    }

    if (rc::isAllPlayerOnGroundOrWater(this) && requestStartDemo()) {
        tryStartDemo();
    }
}

/** @brief Ask every player to get bound to the cutscene. */
void DemoCutscene::exeBindWait() {
    if (al::isFirstStep(this)) {
        rc::requestBindAllPlayerAcceptReviveBubble(this, al::getHitSensor(this, nullptr));
    }
}

/** @brief Fade back in after the cutscene and release the players. */
void DemoCutscene::exeBindEndFade() {
    if (al::isFirstStep(this)) {
        mWipeFade->startOpen(mWipeFrames);
        restorePlayerPosition();
        if (mCameraArea == nullptr) {
            mIsEndAtCutscenePos = false;
            DemoObjBase::endDemo(false);
            al::setCameraReset(this, false);
        }
    }

    if (mWipeFade->isAlive()) {
        return;
    }

    if (mCameraArea != nullptr) {
        mCameraArea->validate();
        al::setNerve(this, &NrvDemoCutsceneBindEndWait);
        return;
    }

    if (!_32a) {
        rc::requestEndDemoPlayerCutscene(this);
    }

    setWait();
}

/** @brief Show the bound players again, move them to their end pose and release them. */
void DemoCutscene::restorePlayerPosition() {
    for (s32 i = 0; i < mPuppeteerGroup->getPuppeteerNumMax(); i++) {
        if (!mPuppeteerGroup->isBinding(i)) {
            continue;
        }

        BindPuppeteer* puppeteer = mPuppeteerGroup->getPuppeteer(i);
        IUsePlayerPuppet* puppet = puppeteer->getPlayerPuppet();
        rc::showPuppetAllParts(puppet);
        rc::validatePuppetSensors(puppet);
        al::HitSensor* sensor = rc::getPuppetSensor(puppet);
        if (mPlayerRestartInfo != nullptr) {
            rc::startPuppetAction(puppet, "Wait");
            sead::Vector3f trans;
            if (al::tryGetTrans(&trans, *mPlayerRestartInfo)) {
                rc::setPuppetTrans(puppet, trans);
            }

            sead::Vector3f front;
            if (al::tryGetFront(&front, *mPlayerRestartInfo)) {
                rc::setPuppetFrontVec(puppet, front);
            }

            if (mIsEndOnGround) {
                puppeteer->endBindOnGround();
            } else {
                puppeteer->endBind(nullptr);
            }
        } else {
            if (mIsKeepPlayerPos || mIsEndAtCutscenePos) {
                rc::setPuppetTrans(puppet, mEndTrans);
                rc::setPuppetFrontVec(puppet, mEndFront);
                if (mIsHidePlayer) {
                    PlayerKoopaJr* koopaJr = PlayerKoopaJr::tryGetPlayerKoopaJr(this);
                    if (koopaJr != nullptr) {
                        koopaJr->resetTransformPostCutscene(mEndTrans, mEndFront);
                    }
                }
            }

            puppeteer->endBindOnGround();
        }

        rc::hidePlayerHoldingItem(sensor, false, false);
        if (mIsUseBaseMtx) {
            al::LiveActor* holdingActor = rc::getPlayerHoldingActor(al::getSensorHost(sensor));
            if (holdingActor != nullptr) {
                rc::addDemoActor(holdingActor);
            }
        }
    }
}

/** @brief Release the players once the cutscene is over. */
void DemoCutscene::exeBindEnd() {
    if (al::isFirstStep(this)) {
        al::requestCaptureScreenCover(this, 3);
        restorePlayerPosition();
        if (mCameraArea != nullptr) {
            mCameraArea->validate();
            al::setNerve(this, &NrvDemoCutsceneBindEndWait);
        } else if (!_32a) {
            rc::requestEndDemoPlayerCutscene(this);
        }
    }

    if (mCameraArea != nullptr) {
        return;
    }

    if (mDisableCapture) {
        al::requestCaptureScreenSceneCover(this);
    } else {
        al::requestCaptureScreenCover(this, 4);
    }

    al::setNerve(this, &NrvDemoCutsceneFinalEndDemo);
}

/** @brief End the demo and reset the camera. */
void DemoCutscene::exeFinalEndDemo() {
    mIsEndAtCutscenePos = false;
    DemoObjBase::endDemo(true);
    al::setCameraReset(this, false);
}

/** @brief End the demo, keeping the camera area active. */
void DemoCutscene::exeBindEndWait() {
    DemoObjBase::endDemo(true);
    if (!_32a) {
        rc::requestEndDemoPlayerCutscene(this);
    }
}

/** @brief Freeze the bound players and fade the screen to black. */
void DemoCutscene::exeFadeToBlackStart() {
    if (al::isFirstStep(this)) {
        for (s32 i = 0; i < mPuppeteerGroup->getPuppeteerNum(); i++) {
            BindPuppeteer* puppeteer = mPuppeteerGroup->getPuppeteer(i);
            if (puppeteer->isBind()) {
                IUsePlayerPuppet* puppet = puppeteer->getPlayerPuppet();
                if (!mIsUseBaseMtx) {
                    rc::startPuppetAction(puppet, "Wait");
                }

                rc::invalidatePuppetSensors(puppet);
            }
        }

        mWipeFade->startClose(mWipeFrames);
    }

    if (mWipeFade->isCloseEnd()) {
        al::setNerve(this, &NrvDemoCutsceneAllBindDone);
    }
}

/** @brief Move the bound players to their cutscene pose and hide them. */
void DemoCutscene::tryHidePlayers() {
    for (s32 i = 0; i < mPuppeteerGroup->getPuppeteerNum(); i++) {
        BindPuppeteer* puppeteer = mPuppeteerGroup->getPuppeteer(i);
        if (!puppeteer->isBind()) {
            continue;
        }

        IUsePlayerPuppet* puppet = puppeteer->getPlayerPuppet();
        if (mPlayerRestartInfo != nullptr) {
            sead::Vector3f trans;
            if (al::tryGetTrans(&trans, *mPlayerRestartInfo)) {
                rc::setPuppetTrans(puppet, trans);
            }

            sead::Vector3f front;
            if (al::tryGetFront(&front, *mPlayerRestartInfo)) {
                rc::setPuppetFrontVec(puppet, front);
            }
        } else if (mIsKeepPlayerPos || mIsEndAtCutscenePos) {
            rc::setPuppetTrans(puppet, mEndTrans);
            rc::setPuppetFrontVec(puppet, mEndFront);
        }

        rc::hidePlayerHoldingItem(rc::getPuppetSensor(puppet), true, false);
        rc::hidePuppetAllParts(puppet);
        rc::cancelSinkSe(rc::getPuppetSensor(puppet));
    }
}

/**
 * @brief Hide the bound players and start the cutscene.
 * @param pCutscene The cutscene.
 */
static void hidePlayersAndStartDemo(DemoCutscene* pCutscene) {
    pCutscene->tryHidePlayers();
    al::requestCaptureScreenCover(pCutscene, 3);
    pCutscene->trueStartDemo();
}

/** @brief Every player is bound: hide them and start the cutscene. */
void DemoCutscene::exeAllBindDone() {
    if (al::isFirstStep(this)) {
        for (s32 i = 0; i < mPuppeteerGroup->getPuppeteerNum(); i++) {
            BindPuppeteer* puppeteer = mPuppeteerGroup->getPuppeteer(i);
            if (puppeteer->isBind()) {
                IUsePlayerPuppet* puppet = puppeteer->getPlayerPuppet();
                rc::startPuppetAction(puppet, "Wait");
                rc::invalidatePuppetSensors(puppet);
            }
        }

        if (mHidePlayerStep <= 0) {
            hidePlayersAndStartDemo(this);
            return;
        }
    }

    if (al::isStep(this, mHidePlayerStep)) {
        hidePlayersAndStartDemo(this);
    }
}

/** @brief Play the cutscene, blocking the skip while the fade is still running. */
void DemoCutscene::exePlay() {
    if (al::isFirstStep(this)) {
        al::tryOnStageSwitchInstant(this, "SwitchAppearOn");
        if (mWipeFade != nullptr) {
            mWipeFade->startOpen(mWipeFrames);
        }
    }

    bool isAllowSkip = mAllowSkip;
    if (mWipeFade != nullptr && mWipeFade->isAlive()) {
        mAllowSkip = false;
    }

    DemoObjBase::exePlay();
    if (mIsSkipEndWipe && mAllowSkip) {
        mAllowSkip = isAllowSkip;
        al::setNerve(this, &NrvDemoCutsceneGuideWindow);
        return;
    }

    mAllowSkip = isAllowSkip;
}

/** @brief End the current phase if requested, otherwise end the cutscene. */
inline void DemoCutscene::endPhaseOrCutscene() {
    if (mIsEndPhaseWhenDone) {
        SingleModeDataFunction::setPhaseEnd(GameDataHolderAccessor(this), true);
        return;
    }

    endDemoCutscene();
}

/** @brief The cutscene ended: show its guide window or end it. */
void DemoCutscene::exeEndDemo() {
    if (al::isFirstStep(this)) {
        if ((mCutsceneId >= CutsceneId_GigaBellPhase1 &&
             mCutsceneId <= CutsceneId_GigaBellResealed) ||
            mCutsceneId == CutsceneId_FinalFightCheer) {
            al::setNerve(this, &NrvDemoCutsceneGuideWindow);
            return;
        }
    }

    if (!mIsFollowedByDemo && !mIsEndPhaseWhenDone) {
        al::tryOffStageSwitchInstant(this, "SwitchAppearOn");
    }

    endPhaseOrCutscene();
}

/** @brief Show the guide window of the cutscene. */
void DemoCutscene::setGuideWindowState() {
    al::setNerve(this, &NrvDemoCutsceneGuideWindow);
}

/** @brief End the cutscene, releasing the bound players. */
void DemoCutscene::endDemoCutscene() {
    if (mIsEndPhaseWhenDone) {
        SingleModeDataFunction::setPhaseEnd(GameDataHolderAccessor(this), true);
    }

    if (mIsFollowedByDemo) {
        al::tryOffStageSwitchInstant(this, "SwitchAppearOn");
    }

    bool isBindPlayer = mIsBindPlayer;
    al::requestCaptureScreenCover(this, 3);
    if (isBindPlayer) {
        if (mWipeFade != nullptr && mIsUseWipeFade) {
            al::setNerve(this, &NrvDemoCutsceneBindEndFade);
        } else {
            al::setNerve(this, &NrvDemoCutsceneBindEnd);
        }
    } else {
        if (!_32a) {
            rc::requestEndDemoCutscene(this);
        }

        al::setNerve(this, &NrvDemoCutsceneFinalEndDemo);
    }
}

/** @brief Show the guide message tied to the cutscene and wait for it to be confirmed. */
void DemoCutscene::exeGuideWindow() {
    if (al::isFirstStep(this)) {
        const char* label;
        switch (mCutsceneId) {
        case CutsceneId_GigaBellPhase1:
            label = "GigaBellCutscene_Phase1";
            break;
        case CutsceneId_GigaBellPhase2:
            label = "GigaBellCutscene_Phase2";
            break;
        case CutsceneId_GigaBellPhase3:
            label = "GigaBellCutscene_Phase3";
            break;
        case CutsceneId_GigaBellResealed:
            label = "GigaBellCutscene_Resealed";
            break;
        case CutsceneId_PhaseReturnGuidePhase2:
            label = "PhaseReturnGuide_Phase2";
            break;
        case CutsceneId_PhaseReturnGuidePhase3:
            label = "PhaseReturnGuide_Phase3";
            break;
        case CutsceneId_FinalFightCheer:
            label = "FinalFightCheer";
            break;
        case CutsceneId_PhaseStartGuidePhase2PartA:
            label = "PhaseStartGuide_Phase2_PartA";
            break;
        case CutsceneId_PhaseStartGuidePhase2PartB:
            if (SingleModeDataFunction::getIs2PAssistMode(this) &&
                al::isPadTypeJoyRight(al::getMainControllerPort()) &&
                al::isPadTypeJoyRight(rc::getPadPortByUserId(1))) {
                label = "PhaseStartGuide_Phase2_PartB_PlusOnly";
            } else {
                label = "PhaseStartGuide_Phase2_PartB";
            }
            break;
        case CutsceneId_PhaseStartGuidePhase3:
            label = "PhaseStartGuide_Phase3";
            break;
        case CutsceneId_PhaseStartGuidePhase4PartA:
            label = "PhaseStartGuide_Phase4_PartA";
            break;
        case CutsceneId_PhaseStartGuidePhase4PartB:
            if (SingleModeDataFunction::getIs2PAssistMode(this) &&
                al::isPadTypeJoySingle(al::getMainControllerPort())) {
                label = "PhaseStartGuide_Phase4_PartB_SingleJoycon";
            } else {
                label = "PhaseStartGuide_Phase4_PartB";
            }
            break;
        default:
            return;
        }

        rc::hideGuideGameWindow(this);
        rc::appearCutsceneGuideGameWindow(this, label);
        return;
    }

    displayFrameCount();
    if (rc::isGuideGameWindowWaitConfirm(this) ||
        mCutsceneId == CutsceneId_PhaseStartGuidePhase2PartA) {
        return;
    }

    if (mWipeFade != nullptr && mIsUseWipeFade) {
        al::setNerve(this, &NrvDemoCutsceneFadeOutGuideWindow);
        return;
    }

    rc::unHideGuideGameWindow(this);
    if (mIsEndPhaseWhenDone) {
        SingleModeDataFunction::setPhaseEnd(GameDataHolderAccessor(this), true);
        return;
    }

    endDemoCutscene();
}

/** @brief Fade the screen to black before ending the cutscene after its guide window. */
void DemoCutscene::exeFadeOutGuideWindow() {
    if (al::isFirstStep(this)) {
        mWipeFade->startClose(mWipeFrames);
        mIsFadingOut = true;
        mIsFadeOutEnd = false;
    }

    if (!mWipeFade->isCloseEnd()) {
        return;
    }

    mIsFadeOutEnd = true;
    rc::unHideGuideGameWindow(this);
    endPhaseOrCutscene();
}

/**
 * @brief Check whether the fade is over.
 * @return Whether a fade exists and is no longer playing.
 */
bool DemoCutscene::isFadeDone() const {
    return mWipeFade != nullptr && !mWipeFade->isAlive();
}

/**
 * @brief Check whether the fade out after the guide window is over.
 * @return Whether the screen is faded out.
 */
bool DemoCutscene::isFadedOut() {
    return mIsFadingOut && mIsFadeOutEnd;
}

/**
 * @brief Check whether the fade to black is over.
 * @return Whether a fade exists and is fully closed.
 */
bool DemoCutscene::isWipeCloseEnd() const {
    return mWipeFade != nullptr && mWipeFade->isCloseEnd();
}

/** @brief Cancel the sounds of the cutscene. */
void DemoCutscene::cancelSE() {
    if (mDemo != nullptr) {
        mDemo->tryCancel(false);
    }
}

/**
 * @brief Save the cutscene as seen and start ending it.
 * @param setWaitState Unused; the cutscene always moves to its end state.
 */
void DemoCutscene::endDemo(bool setWaitState) {
    if (mCutsceneId >= 0) {
        if (mCutsceneId == CutsceneId_PhaseStartGuidePhase4PartA) {
            IslandKeeper* islandKeeper = IslandKeeper::tryGetIslandKeeper(this);
            if (islandKeeper != nullptr) {
                SingleModeDataFunction::setIslandCheckpointPass(
                    this, islandKeeper->getActiveIslandIndex() + 1);
            }
        }

        SingleModeDataFunction::setHasSeenCutscene(this, mCutsceneId);
        SaveDataAccessFunction::startSaveDataWriteSync(GameDataHolderAccessor(this).getHolder(),
                                                       true);
    }

    al::requestCaptureScreenCover(this, 4);
    al::tryOnStageSwitch(this, "SwitchKillOn");
    al::setNerve(this, &NrvDemoCutsceneEndDemo);
}

/** @brief Put every bound player back into its wait animation. */
void DemoCutscene::setPuppetsToWait() {
    for (s32 i = 0; i < mPuppeteerGroup->getPuppeteerNum(); i++) {
        BindPuppeteer* puppeteer = mPuppeteerGroup->getPuppeteer(i);
        if (puppeteer->isBind()) {
            rc::startPuppetAction(puppeteer->getPlayerPuppet(), "Wait");
        }
    }
}

/** @brief Turn off the appear switch of the cutscene. */
void DemoCutscene::forceSwitchOff() {
    al::tryOffStageSwitchInstant(this, "SwitchAppearOn");
}

/** @brief Actually start playing the cutscene. */
void DemoCutscene::trueStartDemo() {
    mIsFadingOut = false;
    DemoObjBase::startDemo();
}

/**
 * @brief Place the cutscene at a matrix, which also becomes the end pose of the players.
 * @param pMtx The matrix.
 */
void DemoCutscene::overrideBaseMtx(const sead::Matrix34f* pMtx) {
    DemoObjBase::overrideBaseMtx(pMtx);
    if (pMtx != nullptr && mIsKeepPlayerPos) {
        mEndTrans.set(pMtx->m[0][3], pMtx->m[1][3], pMtx->m[2][3]);
        mEndFront.set(pMtx->m[0][2], pMtx->m[1][2], pMtx->m[2][2]);
    }
}

/**
 * @brief Handle the bind messages of the players.
 * @param pMsg The message.
 * @param pOther Sensor of the player.
 * @param pSelf Sensor of the cutscene.
 * @return Whether the message was handled.
 */
bool DemoCutscene::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                              al::HitSensor* pSelf) {
    if (al::isNerve(this, &NrvDemoCutsceneBindEnd)) {
        return false;
    }

    if (al::isMsgBindStart(pMsg)) {
        return !mPuppeteerGroup->getPuppeteerByPlayerIndex(pOther)->isBind();
    }

    if (al::isMsgBindInit(pMsg)) {
        BindPuppeteer* puppeteer = mPuppeteerGroup->getPuppeteerByPlayerIndex(pOther);
        puppeteer->startBind(pOther, pSelf);
        puppeteer->getPlayerPuppet();
        if (al::isNerve(this, &NrvDemoCutsceneBindWait)) {
            if (mWipeFade != nullptr) {
                al::setNerve(this, &NrvDemoCutsceneFadeToBlackStart);
            } else {
                al::setNerve(this, &NrvDemoCutsceneAllBindDone);
            }
        }

        return true;
    }

    if (al::isMsgBindCancel(pMsg)) {
        mPuppeteerGroup->getPuppeteerByPlayerIndex(pOther)->cancelBind();
        if (mPuppeteerGroup->isEndBindAll()) {
            al::setNerve(this, &NrvDemoCutsceneBindEnd);
        }

        return true;
    }

    return false;
}

/** @brief End the cutscene and mark it as not seen anymore. */
void DemoCutscene::endAndResetDemo() {
    if (mCutsceneId >= 0) {
        SingleModeDataFunction::clearHasSeenCutscene(this, mCutsceneId);
        SaveDataAccessFunction::startSaveDataWriteSync(GameDataHolderAccessor(this).getHolder(),
                                                       true);
    }

    if (mIsBindPlayer) {
        al::setNerve(this, &NrvDemoCutsceneBindEnd);
        return;
    }

    if (!_32a) {
        rc::requestEndDemoCutscene(this);
    }

    DemoObjBase::endDemo(true);
}
