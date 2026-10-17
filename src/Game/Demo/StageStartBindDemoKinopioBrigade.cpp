#include "Demo/StageStartBindDemoKinopioBrigade.hpp"
#include "Layout/GuideGameWindow.hpp"
#include "Layout/WindowMessage.hpp"
#include "MapObj/BindPuppeteer.hpp"
#include "MapObj/BindPuppeteerGroup.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Math/MatrixUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Resource/ResourceFunction.hpp"
#include "Project/Base/StringUtil.hpp"
#include "System/CourseInfoHolder.hpp"
#include "System/GameDataFlagFunction.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/GameDataHolderWriter.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/DemoUtil.hpp"
#include "Util/PlayerPuppetUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"

namespace {
NERVE_DECL(StageStartBindDemoKinopioBrigade, BindWait);
NERVE_DECL(StageStartBindDemoKinopioBrigade, BindEnd);
NERVE_DECL(StageStartBindDemoKinopioBrigade, Camera);
NERVE_DECL(StageStartBindDemoKinopioBrigade, Walk);
NERVE_DECL(StageStartBindDemoKinopioBrigade, Message);
NERVE_DECL(StageStartBindDemoKinopioBrigade, DokanAppear);
NERVE_DECL(StageStartBindDemoKinopioBrigade, KinopioIntro);
NERVE_DECL(StageStartBindDemoKinopioBrigade, DokanDisappear);
NERVE_DECL(StageStartBindDemoKinopioBrigade, Guide);
NERVES_MAKE_NOSTRUCT(StageStartBindDemoKinopioBrigade, BindWait, BindEnd, Camera, Walk, Message,
                     DokanAppear, KinopioIntro, DokanDisappear, Guide)
}  // namespace

/** @brief Creates the Captain Toad opening. @param pName Actor name. */
StageStartBindDemoKinopioBrigade::StageStartBindDemoKinopioBrigade(const char* pName)
    : StageStartEventBase(pName) {
    mFrontDir = sead::Vector3f::ez;
}

/**
 * @brief Sets up the brigade placements, the message window, the player puppeteers, the
 * opening camera and the entrance pipe.
 * @param rInfo Actor initialization data.
 */
void StageStartBindDemoKinopioBrigade::init(const al::ActorInitInfo& rInfo) {
    al::initActorSceneInfo(this, rInfo);
    al::initActorPoseTRSV(this);
    al::initActorSRT(this, rInfo);
    initHitSensor(1);
    al::addHitSensorBindableGoal(this, rInfo, "Bindable", 0.0f, 0, sead::Vector3f::zero);
    al::initStageSwitch(this, rInfo);
    al::initActorAudioKeeperWithout3D(this, rInfo, nullptr, nullptr);
    al::initNerve(this, &NrvStageStartBindDemoKinopioBrigadeBindWait, 0);
    al::initExecutorUpdate(this, rInfo, "デモオブジェクト");
    makeActorDead();

    s32 playerNumMax = al::getPlayerNumMax(this);
    s32 placeNum = al::calcLinkChildNum(rInfo, "KinopioPlacePos");
    if (placeNum == playerNumMax - 1) {
        mBrigadeIntros = new BrigadeIntro[playerNumMax];
        for (s32 i = 0; i < placeNum; i++) {
            BrigadeIntro& intro = mBrigadeIntros[i + 1];
            al::getLinksMatrixByIndex(&intro.mBaseMtx, rInfo, "KinopioPlacePos", i);
            s32 delay = (i + 1) * 30;
            intro.mDelay = delay;
            intro.mTimer = delay;
        }
    }

    if (al::calcLinkChildNum(rInfo, "KinopioBrigadeStartPos") >= 1) {
        mGuideIntro = new BrigadeIntro;
        al::getLinksMatrixByIndex(&mGuideIntro->mBaseMtx, rInfo, "KinopioBrigadeStartPos", 0);
        mGuideIntro->mTimer = 0;
    }

    mWindow = new WindowMessage(al::getLayoutInitInfo(rInfo), "WindowMessage",
                                "メッセージウインドウ", nullptr);
    mPuppeteers = new BindPuppeteerGroup("キノピオ探検隊開始デモバインド操作グループ", playerNumMax);
    for (s32 i = 0; i < mPuppeteers->getPuppeteerNumMax(); i++) {
        auto* puppeteer = new BindPuppeteer("キノピオ探検隊開始デモバインド操作");
        mPuppeteers->registerPuppeteer(puppeteer);
    }

    al::Resource* resource = al::findOrCreateResource("ObjectData/DemoCamera", nullptr);
    mCameraName.format("Demo%s",
                       GameDataFunction::findStageName(
                           GameDataHolderAccessor(this),
                           GameDataFunction::getPlayingCourseId(GameDataHolderAccessor(this))));
    mCameraName.removeSuffix("Stage");
    mCamera = al::initAnimCamera(this, rInfo, resource, mCameraName.cstr(), false);

    mDokan = new al::LiveActor("デモ土管");
    al::initActorWithArchiveName(mDokan, rInfo, "Dokan", nullptr);
    mDokan->makeActorDead();

    const char* stageName = GameDataFunction::findStageName(
        GameDataHolderAccessor(this),
        GameDataFunction::getPlayingCourseId(GameDataHolderAccessor(this)));
    if (al::isEqualString(stageName, "KinopioBrigadeTeresaStage")) {
        mIsTeresaStage = true;
    }
}

/**
 * @brief Handles player binding and lines each bound player up with a brigade placement.
 * @param pMsg Message.
 * @param pSender Player sensor.
 * @param pReceiver Binding sensor.
 * @return Whether the message was accepted.
 */
bool StageStartBindDemoKinopioBrigade::receiveMsg(const al::SensorMsg* pMsg,
                                                  al::HitSensor* pSender,
                                                  al::HitSensor* pReceiver) {
    if (al::isNerve(this, &NrvStageStartBindDemoKinopioBrigadeBindEnd)) {
        return false;
    }

    if (al::isMsgBindStart(pMsg)) {
        BindPuppeteer* puppeteer = mPuppeteers->getPuppeteer(pSender);
        if (puppeteer != nullptr && puppeteer->isBind()) {
            return false;
        }

        return true;
    }

    if (al::isMsgBindInit(pMsg)) {
        BindPuppeteer* puppeteer = mPuppeteers->getPuppeteerNoBind();
        puppeteer->startBind(pSender, pReceiver);
        s32 index = mPuppeteers->getBindingIndex(puppeteer);
        IUsePlayerPuppet* puppet = puppeteer->getPlayerPuppet();
        mFirstPlayerTrans = rc::getPuppetTrans(puppet);

        bool isFaceToBase = index >= 1;
        if (index == 0) {
            al::calcFrontDir(&mFrontDir, al::getSensorHost(pSender));
            if (mGuideIntro != nullptr && al::getAlivePlayerNum(this) >= 2) {
                mBrigadeIntros[0].mBaseMtx = mGuideIntro->mBaseMtx;
                isFaceToBase = true;
            } else {
                al::makeMtxRT(&mBrigadeIntros[0].mBaseMtx, al::getSensorHost(pSender));
            }
        }

        if (isFaceToBase) {
            BrigadeIntro& intro = mBrigadeIntros[index];
            sead::Vector3f baseTrans;
            intro.mBaseMtx.getTranslation(baseTrans);
            sead::Vector3f dir = baseTrans - mFirstPlayerTrans;
            dir.y = 0.0f;
            dir.normalize();
            intro.mFrontDir = dir;
            rc::setPuppetFrontVec(puppet, dir);
        } else {
            mBrigadeIntros[index].mFrontDir = mFrontDir;
        }

        rc::hidePuppet(puppet);
        rc::hidePuppetSilhouette(puppet);
        rc::hidePuppetShadow(puppet);
        rc::startPuppetAction(puppet, "DokanOut");
        rc::setPuppetActionRate(puppet, 0.0f);
        if (al::isNerve(this, &NrvStageStartBindDemoKinopioBrigadeBindWait)) {
            al::setNerve(this, &NrvStageStartBindDemoKinopioBrigadeCamera);
        }

        return true;
    }

    if (al::isMsgBindCancel(pMsg)) {
        mPuppeteers->getPuppeteer(pSender)->cancelBind();
        if (mPuppeteers->isEndBindAll()) {
            al::setNerve(this, &NrvStageStartBindDemoKinopioBrigadeBindEnd);
        }

        return true;
    }

    if (rc::isMsgIsDisableCancelBubble(pMsg) &&
        !al::isNerve(this, &NrvStageStartBindDemoKinopioBrigadeBindEnd)) {
        return true;
    }

    return false;
}

/**
 * @brief Moves every bound player to where the opening would leave them.
 * @param isEndBind Whether the players are released afterwards.
 */
void StageStartBindDemoKinopioBrigade::warpDemoEndPos(bool isEndBind) {
    for (s32 i = 0; i < mPuppeteers->getPuppeteerNumMax(); i++) {
        BindPuppeteer* puppeteer = mPuppeteers->getPuppeteer(i);
        if (!puppeteer->isBind()) {
            continue;
        }

        IUsePlayerPuppet* puppet = puppeteer->getPlayerPuppet();
        rc::setPuppetTrans(puppet, rc::getPuppetFrontVec(puppet) * 200.0f + mFirstPlayerTrans);
        rc::setPuppetFrontVec(puppet, mFrontDir);
        if (isEndBind) {
            puppeteer->endBindOnGround();
        }
    }
}

/**
 * @brief Advances the entrance of one brigade member out of the pipe.
 * @param index Index of the member's puppeteer.
 * @return Whether the member has finished its entrance.
 */
bool StageStartBindDemoKinopioBrigade::updateBrigadeIntro(s32 index) {
    BindPuppeteer* puppeteer = mPuppeteers->getPuppeteer(index);
    if (!puppeteer->isBind()) {
        return true;
    }

    BrigadeIntro& intro = mBrigadeIntros[index];
    IUsePlayerPuppet* puppet = puppeteer->getPlayerPuppet();
    switch (intro.mState) {
    case IntroState::Delay:
        if (intro.mTimer > 0) {
            intro.mTimer--;
            break;
        }

        intro.mState = IntroState::Appear;
        intro.mStartTrans = rc::getPuppetTrans(puppet);
        rc::setPuppetTrans(puppet, rc::getPuppetTrans(puppet) + sead::Vector3f(0.0f, 160.0f, 0.0f));
        rc::showPuppet(puppet);
        rc::setPuppetActionRate(puppet, 1.0f);
        if (!mIsStartAppearSe) {
            mIsStartAppearSe = true;
            if (mIsTeresaStage) {
                rc::startPuppetSe(puppet, "PgKinotanAppearAtStageHaunt");
            } else {
                rc::startPuppetSe(puppet, "PgKinotanAppearAtStage");
            }
        }

        break;
    case IntroState::Appear:
        if (rc::isPuppetActionEnd(puppet)) {
            intro.mState = IntroState::Rise;
            rc::showPuppetSilhouette(puppet);
            rc::showPuppetShadow(puppet);
            rc::startPuppetAction(puppet, "Move");
            intro.mStartTrans = rc::getPuppetTrans(puppet);
            intro.mTimer = 30;
        }

        break;
    case IntroState::Rise:
        if (intro.mTimer > 0) {
            intro.mTimer--;
            f32 rate = intro.mTimer / -30.0f + 1.0f;
            f32 distance = al::lerpValue(rate, 0.0f, 160.0f);
            rc::setPuppetTrans(puppet, intro.mFrontDir * distance + intro.mStartTrans);
        } else {
            rc::startPuppetAction(puppet, "Fall");
            intro.mStartTrans = rc::getPuppetTrans(puppet);
            intro.mTimer = 15;
            intro.mState = IntroState::Fall;
        }

        break;
    case IntroState::Fall:
        if (intro.mTimer > 0) {
            intro.mTimer--;
            f32 rate = intro.mTimer / -15.0f + 1.0f;
            const sead::Vector3f& up = rc::getPuppetUpVec(puppet);
            f32 distance = al::lerpValue(al::easeIn(rate), 0.0f, 160.0f);
            rc::setPuppetTrans(puppet, intro.mStartTrans - up * distance);
        } else {
            rc::startPuppetAction(puppet, "Move");
            intro.mStartTrans = rc::getPuppetTrans(puppet);
            intro.mTimer = 15;
            intro.mState = IntroState::Walk;
        }

        rc::solveAirPuppet(puppet);
        break;
    case IntroState::Walk:
        if (intro.mTimer > 0) {
            intro.mTimer--;
            f32 rate = intro.mTimer / -15.0f + 1.0f;
            f32 distance = al::lerpValue(rate, 0.0f, 40.0f);
            rc::setPuppetTrans(puppet, intro.mFrontDir * distance + intro.mStartTrans);
        } else {
            intro.mTimer = 10;
            intro.mStartTrans = rc::getPuppetTrans(puppet);
            intro.mState = IntroState::Turn;
        }

        rc::solveAirPuppet(puppet);
        break;
    case IntroState::Turn:
        if (intro.mTimer > 0) {
            if (rc::faceToDirection(puppet, mFrontDir, 10.0f, 0.1f)) {
                intro.mTimer = 0;
            }
        } else {
            intro.mStartTrans = rc::getPuppetTrans(puppet);
            rc::startPuppetAction(puppet, "Wait");
            intro.mState = IntroState::End;
        }

        rc::solveAirPuppet(puppet);
        break;
    case IntroState::End:
        return true;
    default:
        break;
    }

    return false;
}

/**
 * @brief Checks whether the opening is still being shown.
 * @return Whether a demo nerve is active.
 */
inline bool StageStartBindDemoKinopioBrigade::isPlayingDemo() const {
    return al::isNerve(this, &NrvStageStartBindDemoKinopioBrigadeCamera) ||
           al::isNerve(this, &NrvStageStartBindDemoKinopioBrigadeDokanAppear) ||
           al::isNerve(this, &NrvStageStartBindDemoKinopioBrigadeWalk) ||
           al::isNerve(this, &NrvStageStartBindDemoKinopioBrigadeDokanDisappear) ||
           al::isNerve(this, &NrvStageStartBindDemoKinopioBrigadeMessage) ||
           al::isNerve(this, &NrvStageStartBindDemoKinopioBrigadeKinopioIntro);
}

/** @brief Lets the players skip the opening of an already cleared course. */
void StageStartBindDemoKinopioBrigade::control() {
    if (isPlayingDemo() &&
        CourseInfoFunction::isClear(
            GameDataHolderAccessor(this),
            GameDataFunction::getPlayingCourseId(GameDataHolderAccessor(this))) &&
        rc::tryCancelStageDemoStrict(this)) {
        endDemo();
    }
}

/** @brief Cuts the opening short and moves the players to their final positions. */
void StageStartBindDemoKinopioBrigade::endDemo() {
    if (!isPlayingDemo()) {
        return;
    }

    mWindow->kill();
    mDokan->kill();
    warpDemoEndPos(true);
    if (!al::isNerve(this, &NrvStageStartBindDemoKinopioBrigadeMessage) &&
        !al::isNerve(this, &NrvStageStartBindDemoKinopioBrigadeBindEnd)) {
        al::endCamera(this, mCamera, 0);
    }

    al::setNerve(this, &NrvStageStartBindDemoKinopioBrigadeBindEnd);
}

/** @brief Checks completion. @return Whether the opening is no longer shown. */
bool StageStartBindDemoKinopioBrigade::isEndDemo() const {
    return !isPlayingDemo();
}

/**
 * @brief Finds the puppet of the first bound player.
 * @return The puppet, or nullptr when no player is bound.
 */
inline IUsePlayerPuppet* StageStartBindDemoKinopioBrigade::findFirstBindPuppet() const {
    for (s32 i = 0; i < mPuppeteers->getPuppeteerNumMax(); i++) {
        if (mPuppeteers->getPuppeteer(i)->isBind()) {
            return mPuppeteers->getPuppeteer(i)->getPlayerPuppet();
        }
    }

    return nullptr;
}

/** @brief Requests all players and starts the opening camera unless it is skipped. */
void StageStartBindDemoKinopioBrigade::exeBindWait() {
    if (!al::isFirstStep(this)) {
        return;
    }

    rc::requestBindAllPlayerAcceptReviveBubble(this, al::getHitSensor(this, nullptr));
    al::startBgmWithAreaCheck(this, false, -1, 0, -1);
    if (GameDataFunction::isRestartStage(GameDataHolderAccessor(this)) ||
        GameDataFunction::isSkipStartDemo(GameDataHolderAccessor(this))) {
        al::tryOnSwitchDeadOn(this);
        return;
    }

    al::makeMtxFrontUpPos(&mCameraMtx, sead::Vector3f::ez, sead::Vector3f::ey,
                          al::findNearestPlayerPos(this));
    al::startAnimCamera(this, mCamera, mCameraName.cstr(), &mCameraMtx, 0);
}

/** @brief Waits for the opening camera, or skips straight to the message. */
void StageStartBindDemoKinopioBrigade::exeCamera() {
    if (GameDataFunction::isRestartStage(GameDataHolderAccessor(this)) ||
        GameDataFunction::isSkipStartDemo(GameDataHolderAccessor(this))) {
        warpDemoEndPos(false);
        al::setNerve(this, &NrvStageStartBindDemoKinopioBrigadeMessage);
        return;
    }

    if (al::isEndAnimCamera(mCamera)) {
        al::setNerve(this, &NrvStageStartBindDemoKinopioBrigadeDokanAppear);
    }
}

/** @brief Raises the entrance pipe under the first bound player. */
void StageStartBindDemoKinopioBrigade::exeDokanAppear() {
    if (al::isFirstStep(this)) {
        IUsePlayerPuppet* puppet = findFirstBindPuppet();
        al::resetPosition(mDokan, rc::getPuppetTrans(puppet), false);
        al::invalidateClipping(mDokan);
        al::startAction(mDokan, "Appear");
        mDokan->appear();
    }

    if (al::isActionEnd(mDokan)) {
        al::setNerve(this, &NrvStageStartBindDemoKinopioBrigadeKinopioIntro);
    }
}

/** @brief Plays the entrance of every brigade member. */
void StageStartBindDemoKinopioBrigade::exeKinopioIntro() {
    if (al::isFirstStep(this)) {
        for (s32 i = 0; i < mPuppeteers->getPuppeteerNumMax(); i++) {
            if (mPuppeteers->isBinding(i)) {
                IUsePlayerPuppet* puppet = mPuppeteers->getPuppeteer(i)->getPlayerPuppet();
                mBrigadeIntros[i].mStartTrans = rc::getPuppetTrans(puppet);
            }
        }
    }

    bool isEndAll = true;
    for (s32 i = 0; i < mPuppeteers->getPuppeteerNumMax(); i++) {
        isEndAll &= updateBrigadeIntro(i);
    }

    if (isEndAll) {
        al::setNerve(this, &NrvStageStartBindDemoKinopioBrigadeDokanDisappear);
    }
}

/** @brief Walks the first bound player out of the pipe. */
void StageStartBindDemoKinopioBrigade::exeWalk() {
    IUsePlayerPuppet* puppet = findFirstBindPuppet();
    if (al::isFirstStep(this)) {
        mWalkStartTrans = rc::getPuppetTrans(puppet);
        rc::startPuppetAction(puppet, "Move");
    }

    if (al::isLessEqualStep(this, 30)) {
        const sead::Vector3f& front = rc::getPuppetFrontVec(puppet);
        f32 distance = al::lerpValue(al::calcNerveRate(this, 30), 0.0f, 160.0f);
        rc::setPuppetTrans(puppet, front * distance + mWalkStartTrans);
        if (al::isStep(this, 30)) {
            rc::startPuppetAction(puppet, "Fall");
            mWalkStartTrans = rc::getPuppetTrans(puppet);
        }
    } else if (al::isLessEqualStep(this, 45)) {
        f32 rate = sead::Mathf::clamp((al::getNerveStep(this) - 30) / 15.0f, 0.0f, 1.0f);
        const sead::Vector3f& up = rc::getPuppetUpVec(puppet);
        f32 distance = al::lerpValue(al::easeIn(rate), 0.0f, 160.0f);
        rc::setPuppetTrans(puppet, mWalkStartTrans - up * distance);
        if (al::isStep(this, 45)) {
            rc::startPuppetAction(puppet, "Move");
            mWalkStartTrans = rc::getPuppetTrans(puppet);
        }

        rc::solveAirPuppet(puppet);
    } else if (al::isLessEqualStep(this, 60)) {
        const sead::Vector3f& front = rc::getPuppetFrontVec(puppet);
        f32 rate = sead::Mathf::clamp((al::getNerveStep(this) - 45) / 15.0f, 0.0f, 1.0f);
        f32 distance = al::lerpValue(rate, 0.0f, 40.0f);
        rc::setPuppetTrans(puppet, front * distance + mWalkStartTrans);
        if (al::isStep(this, 60)) {
            mWalkStartTrans = rc::getPuppetTrans(puppet);
            rc::startPuppetAction(puppet, "Wait");
        }

        rc::solveAirPuppet(puppet);
    } else if (!al::isLessStep(this, 90)) {
        al::setNerve(this, &NrvStageStartBindDemoKinopioBrigadeDokanDisappear);
    }
}

/** @brief Lowers the entrance pipe and hands the camera back to the players. */
void StageStartBindDemoKinopioBrigade::exeDokanDisappear() {
    if (al::isFirstStep(this)) {
        al::startAction(mDokan, "Disappear");
    }

    if (al::isActionEnd(mDokan)) {
        mDokan->kill();
        al::endCamera(this, mCamera, 60);
        al::requestCancelInputKinopioBrigadeCamera(this, 60);
        al::setNerve(this, &NrvStageStartBindDemoKinopioBrigadeMessage);
    }
}

/** @brief Shows the first-play message once, then releases the players. */
void StageStartBindDemoKinopioBrigade::exeMessage() {
    if (al::isFirstStep(this) &&
        !GameDataFlagFunction::isAlreadyPlayKinopioBrigade(GameDataHolderAccessor(this))) {
        s32 port = rc::calcPadPortByFirstActiveUser(GameDataHolderAccessor(this));
        mWindow->appearWithSystemMessage("WindowMessage", "FirstPlayKinopioBrigade", port);
        GameDataFlagFunction::setPlayKinopioBrigade(GameDataHolderAccessor(this));
    }

    if (mWindow->isAlive()) {
        return;
    }

    for (s32 i = 0; i < mPuppeteers->getPuppeteerNumMax(); i++) {
        if (mPuppeteers->isBinding(i)) {
            mPuppeteers->getPuppeteer(i)->endBindOnGround();
        }
    }

    al::setNerve(this, &NrvStageStartBindDemoKinopioBrigadeBindEnd);
}

/** @brief Turns the switch on and shows the course guide. */
void StageStartBindDemoKinopioBrigade::exeBindEnd() {
    if (al::isFirstStep(this)) {
        al::tryOnSwitchDeadOn(this);
    }

    rc::appearGuideGameWindow(this, "GuideMessage", "KinopioBrigade", -1, 250.0f);
    al::setNerve(this, &NrvStageStartBindDemoKinopioBrigadeGuide);
}

/** @brief Hides the course guide after a while and removes the event. */
void StageStartBindDemoKinopioBrigade::exeGuide() {
    if (al::isLessStep(this, 840)) {
        return;
    }

    rc::disappearGuideGameWindow(this);
    kill();
}
