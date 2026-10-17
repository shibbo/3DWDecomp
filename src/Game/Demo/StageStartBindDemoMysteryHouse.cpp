#include "Demo/StageStartBindDemoMysteryHouse.hpp"
#include "Layout/WindowMessage.hpp"
#include "MapObj/BindPuppeteer.hpp"
#include "MapObj/BindPuppeteerGroup.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "System/GameDataFlagFunction.hpp"
#include "System/GameDataFunction.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Project/Bgm/BgmPlayingRequest.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/GameDataHolderWriter.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/PlayerUtil.hpp"

namespace {
NERVE_DECL(StageStartBindDemoMysteryHouse, BindWait);
NERVE_DECL(StageStartBindDemoMysteryHouse, BindEnd);
NERVE_DECL(StageStartBindDemoMysteryHouse, Sound);
NERVE_DECL(StageStartBindDemoMysteryHouse, Message);
NERVES_MAKE_NOSTRUCT(StageStartBindDemoMysteryHouse, BindWait, BindEnd, Sound, Message)
}

/** @brief Creates the Mystery House opening. @param pName Actor name. */
StageStartBindDemoMysteryHouse::StageStartBindDemoMysteryHouse(const char* pName)
    : StageStartEventBase(pName) {}

/** @brief Creates the message and player puppeteers. @param rInfo Actor initialization data. */
void StageStartBindDemoMysteryHouse::init(const al::ActorInitInfo& rInfo) {
    al::initActorSceneInfo(this, rInfo);
    al::initActorPoseTRSV(this);
    al::initActorSRT(this, rInfo);
    initHitSensor(1);
    al::addHitSensorBindableGoal(this, rInfo, "Bindable", 0.0f, 0, sead::Vector3f::zero);
    al::initActorAudioKeeperWithout3D(this, rInfo, "StageStartEventSound", nullptr);
    al::initNerve(this, &NrvStageStartBindDemoMysteryHouseBindWait, 0);
    al::initExecutorUpdate(this, rInfo, "デモオブジェクト");
    makeActorDead();
    mWindow = new WindowMessage(al::getLayoutInitInfo(rInfo), "WindowMessage",
                                "メッセージウインドウ", nullptr);
    mPuppeteers = new BindPuppeteerGroup("ミステリーハウスデモバインド操作グループ", al::getPlayerNumMax(this));
    for (int i = 0; i < mPuppeteers->getPuppeteerNumMax(); ++i) {
        auto* puppet = new BindPuppeteer("ミステリーハウスデモバインド操作");
        mPuppeteers->registerPuppeteer(puppet);
    }
}

/** @brief Closes an active introduction and releases its players. */
void StageStartBindDemoMysteryHouse::kill() {
    al::LiveActor::kill();
    if (al::isNerve(this, &NrvStageStartBindDemoMysteryHouseBindEnd))
        return;
    mWindow->kill();
    for (int i = 0; i < mPuppeteers->getPuppeteerNumMax(); ++i) {
        auto* puppet = mPuppeteers->getPuppeteer(i);
        if (puppet->isBind())
            puppet->endBindOnGround();
    }
    al::setNerve(this, &NrvStageStartBindDemoMysteryHouseBindEnd);
}

/** @brief Handles player binding. @param pMsg Message. @param pSender Player sensor.
 * @param pReceiver Binding sensor. @return Whether the message was accepted. */
bool StageStartBindDemoMysteryHouse::receiveMsg(const al::SensorMsg* pMsg,
    al::HitSensor* pSender, al::HitSensor* pReceiver) {
    if (al::isNerve(this, &NrvStageStartBindDemoMysteryHouseBindEnd))
        return false;
    if (al::isMsgBindStart(pMsg)) {
        if (mPuppeteers->getPuppeteerByPlayerIndex(pSender)->isBind())
            return false;
        if (al::isNerve(this, &NrvStageStartBindDemoMysteryHouseBindWait)) {
            if (GameDataFunction::isRestartStage(GameDataHolderAccessor(this)))
                al::setNerve(this, &NrvStageStartBindDemoMysteryHouseMessage);
            else
                al::setNerve(this, &NrvStageStartBindDemoMysteryHouseSound);
        }
        return true;
    }
    if (al::isMsgBindInit(pMsg)) {
        mPuppeteers->getPuppeteerByPlayerIndex(pSender)->startBind(pSender, pReceiver);
        return true;
    }
    if (al::isMsgBindCancel(pMsg)) {
        mPuppeteers->getPuppeteerByPlayerIndex(pSender)->cancelBind();
        if (mPuppeteers->isEndBindAll())
            al::setNerve(this, &NrvStageStartBindDemoMysteryHouseBindEnd);
        return true;
    }
    return false;
}

/** @brief Checks completion. @return Whether the actor is dead. */
bool StageStartBindDemoMysteryHouse::isEndDemo() const { return al::isDead(this); }

/** @brief Requests all players for the opening explanation. */
void StageStartBindDemoMysteryHouse::exeBindWait() {
    if (al::isFirstStep(this)) {
        rc::requestBindAllPlayerAcceptReviveBubble(this, al::getHitSensor(this, nullptr));
    }
}

/** @brief Waits for the opening sound before the message. */
void StageStartBindDemoMysteryHouse::exeSound() {
    if (al::isFirstStep(this))
        al::startSe(this, "Door", nullptr);
    if (al::isLessStep(this, 60))
        return;
    al::setNerve(this, &NrvStageStartBindDemoMysteryHouseMessage);
}

/** @brief Allows the opening wipe after binding and the door sound.
 * @return Whether the stage wipe may open. */
bool StageStartBindDemoMysteryHouse::isEnableOpenStartWipe() const {
    return !al::isNerve(this, &NrvStageStartBindDemoMysteryHouseBindWait) &&
           !al::isNerve(this, &NrvStageStartBindDemoMysteryHouseSound);
}

/** @brief Plays entrance music and shows the first-visit or retry explanation. */
void StageStartBindDemoMysteryHouse::exeMessage() {
    if (al::isFirstStep(this)) {
        al::startSequenceBgm(this, "MysteryHouseEntrance", -1, 0);
        al::startBgm(this, al::BgmPlayingRequest("Stage", -1, 0, -1, true, -1));
        al::pauseBgm(this, "Stage", -1);
        if (GameDataFlagFunction::isAlreadyPlayMysteryHouse(GameDataHolderAccessor(this)) &&
            GameDataFlagFunction::isAlreadyRetryMysteryHouse(GameDataHolderAccessor(this))) {
            al::setNerve(this, &NrvStageStartBindDemoMysteryHouseBindEnd);
            return;
        }
    }
    GameDataFunction::isRestartStage(GameDataHolderAccessor(this));
    if (al::isLessStep(this, 60)) return;
    if (al::isStep(this, 60)) {
        if (!GameDataFlagFunction::isAlreadyPlayMysteryHouse(GameDataHolderAccessor(this))) {
            int port = rc::calcPadPortByFirstActiveUser(GameDataHolderAccessor(this));
            mWindow->appearWithSystemMessage("WindowMessage", "FirstPlayMysteryHouseStage", port);
            GameDataFlagFunction::setPlayMysteryHouse(GameDataHolderWriter(this));
        } else if (GameDataFunction::isRestartStage(GameDataHolderAccessor(this)) &&
                   !GameDataFlagFunction::isAlreadyRetryMysteryHouse(GameDataHolderAccessor(this))) {
            int port = rc::calcPadPortByFirstActiveUser(GameDataHolderAccessor(this));
            mWindow->appearWithSystemMessage("WindowMessage", "FirstRetryMysteryHouseStage", port);
            GameDataFlagFunction::setRetryMysteryHouse(GameDataHolderWriter(this));
        }
    }
    if (!mWindow->isAlive()) al::setNerve(this, &NrvStageStartBindDemoMysteryHouseBindEnd);
}

/** @brief Releases every bound player and removes the event. */
void StageStartBindDemoMysteryHouse::exeBindEnd() {
    if (al::isFirstStep(this)) {
        for (int i = 0; i < mPuppeteers->getPuppeteerNumMax(); ++i) {
            auto* puppet = mPuppeteers->getPuppeteer(i);
            if (puppet->isBind())
                puppet->endBindOnGround();
        }
    }
    kill();
}
