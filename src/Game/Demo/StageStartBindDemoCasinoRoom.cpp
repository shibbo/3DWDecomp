#include "Demo/StageStartBindDemoCasinoRoom.hpp"
#include "Layout/WindowMessage.hpp"
#include "MapObj/BindPuppeteer.hpp"
#include "MapObj/BindPuppeteerGroup.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "System/GameDataFlagFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/GameDataHolderWriter.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/PlayerUtil.hpp"

namespace {
NERVE_DECL(StageStartBindDemoCasinoRoom, BindWait);
NERVE_DECL(StageStartBindDemoCasinoRoom, BindEnd);
NERVE_DECL(StageStartBindDemoCasinoRoom, Message);
NERVES_MAKE_NOSTRUCT(StageStartBindDemoCasinoRoom, BindWait, BindEnd, Message)
}

/** @brief Creates the casino opening. @param pName Actor name. */
StageStartBindDemoCasinoRoom::StageStartBindDemoCasinoRoom(const char* pName)
    : StageStartEventBase(pName) {}

/** @brief Creates the message and player puppeteers. @param rInfo Actor initialization data. */
void StageStartBindDemoCasinoRoom::init(const al::ActorInitInfo& rInfo) {
    al::initActorSceneInfo(this, rInfo);
    al::initActorPoseTRSV(this);
    al::initActorSRT(this, rInfo);
    initHitSensor(1);
    al::addHitSensorBindableGoal(this, rInfo, "Bindable", 0.0f, 0, sead::Vector3f::zero);
    al::initActorAudioKeeperWithout3D(this, rInfo, nullptr, nullptr);
    al::initNerve(this, &NrvStageStartBindDemoCasinoRoomBindWait, 0);
    al::initExecutorUpdate(this, rInfo, "デモオブジェクト");
    makeActorDead();
    mWindow = new WindowMessage(al::getLayoutInitInfo(rInfo), "WindowMessage",
                                "メッセージウインドウ", nullptr);
    mPuppeteers = new BindPuppeteerGroup("カジノデモバインド操作グループ", al::getPlayerNumMax(this));
    for (int i = 0; i < mPuppeteers->getPuppeteerNumMax(); ++i) {
        auto* puppet = new BindPuppeteer("カジノデモバインド操作");
        mPuppeteers->registerPuppeteer(puppet);
    }
}

/** @brief Closes an active introduction and releases its players. */
void StageStartBindDemoCasinoRoom::kill() {
    al::LiveActor::kill();
    if (al::isNerve(this, &NrvStageStartBindDemoCasinoRoomBindWait) ||
        al::isNerve(this, &NrvStageStartBindDemoCasinoRoomBindEnd))
        return;
    mWindow->kill();
    for (int i = 0; i < mPuppeteers->getPuppeteerNumMax(); ++i) {
        auto* puppet = mPuppeteers->getPuppeteer(i);
        if (puppet->isBind())
            puppet->endBindOnGround();
    }
}

/** @brief Handles player binding. @param pMsg Message. @param pSender Player sensor.
 * @param pReceiver Binding sensor. @return Whether the message was accepted. */
bool StageStartBindDemoCasinoRoom::receiveMsg(const al::SensorMsg* pMsg,
    al::HitSensor* pSender, al::HitSensor* pReceiver) {
    if (al::isNerve(this, &NrvStageStartBindDemoCasinoRoomBindEnd))
        return false;
    if (al::isMsgBindStart(pMsg)) {
        if (mPuppeteers->getPuppeteerByPlayerIndex(pSender)->isBind())
            return false;
        if (al::isNerve(this, &NrvStageStartBindDemoCasinoRoomBindWait))
            al::setNerve(this, &NrvStageStartBindDemoCasinoRoomMessage);
        return true;
    }
    if (al::isMsgBindInit(pMsg)) {
        mPuppeteers->getPuppeteerByPlayerIndex(pSender)->startBind(pSender, pReceiver);
        return true;
    }
    if (al::isMsgBindCancel(pMsg)) {
        mPuppeteers->getPuppeteerByPlayerIndex(pSender)->cancelBind();
        if (mPuppeteers->isEndBindAll())
            al::setNerve(this, &NrvStageStartBindDemoCasinoRoomBindEnd);
        return true;
    }
    return false;
}

/** @brief Checks completion. @return Whether the actor is dead. */
bool StageStartBindDemoCasinoRoom::isEndDemo() const { return al::isDead(this); }

/** @brief Requests all players and starts casino music. */
void StageStartBindDemoCasinoRoom::exeBindWait() {
    if (al::isFirstStep(this)) {
        rc::requestBindAllPlayerAcceptReviveBubble(this, al::getHitSensor(this, nullptr));
        al::startBgm(this, "RouletteRoom", -1, 0, -1, -1);
    }
}

/** @brief Shows the introduction once, after the players settle. */
void StageStartBindDemoCasinoRoom::exeMessage() {
    if (al::isFirstStep(this) && GameDataFlagFunction::isAlreadyPlayCasinoRoom(GameDataHolderAccessor(this))) {
        al::setNerve(this, &NrvStageStartBindDemoCasinoRoomBindEnd);
        return;
    }
    if (al::isLessStep(this, 60))
        return;
    if (al::isStep(this, 60)) {
        int port = rc::calcPadPortByFirstActiveUser(GameDataHolderAccessor(this));
        mWindow->appearWithSystemMessage("WindowMessage", "FirstPlayCasinoRoom", port);
        GameDataFlagFunction::setPlayCasinoRoom(GameDataHolderAccessor(this));
    }
    if (!mWindow->isAlive())
        al::setNerve(this, &NrvStageStartBindDemoCasinoRoomBindEnd);
}

/** @brief Releases every bound player and removes the event. */
void StageStartBindDemoCasinoRoom::exeBindEnd() {
    if (al::isFirstStep(this)) {
        for (int i = 0; i < mPuppeteers->getPuppeteerNumMax(); ++i) {
            auto* puppet = mPuppeteers->getPuppeteer(i);
            if (puppet->isBind())
                puppet->endBindOnGround();
        }
    }
    kill();
}
