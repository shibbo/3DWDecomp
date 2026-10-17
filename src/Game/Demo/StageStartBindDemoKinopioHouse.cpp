#include "Demo/StageStartBindDemoKinopioHouse.hpp"
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
NERVE_DECL(StageStartBindDemoKinopioHouse, BindWait);
NERVE_DECL(StageStartBindDemoKinopioHouse, BindEnd);
NERVE_DECL(StageStartBindDemoKinopioHouse, Message);
NERVES_MAKE_NOSTRUCT(StageStartBindDemoKinopioHouse, BindWait, BindEnd, Message)
}

/** @brief Creates the Toad House opening. @param pName Actor name. */
StageStartBindDemoKinopioHouse::StageStartBindDemoKinopioHouse(const char* pName)
    : StageStartEventBase(pName) {}

/** @brief Creates the message and player puppeteers. @param rInfo Actor initialization data. */
void StageStartBindDemoKinopioHouse::init(const al::ActorInitInfo& rInfo) {
    al::initActorSceneInfo(this, rInfo);
    al::initActorPoseTRSV(this);
    al::initActorSRT(this, rInfo);
    initHitSensor(1);
    al::addHitSensorBindableGoal(this, rInfo, "Bindable", 0.0f, 0, sead::Vector3f::zero);
    al::initActorAudioKeeperWithout3D(this, rInfo, nullptr, nullptr);
    al::initNerve(this, &NrvStageStartBindDemoKinopioHouseBindWait, 0);
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
void StageStartBindDemoKinopioHouse::kill() {
    al::LiveActor::kill();
    if (al::isNerve(this, &NrvStageStartBindDemoKinopioHouseBindWait) ||
        al::isNerve(this, &NrvStageStartBindDemoKinopioHouseBindEnd))
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
bool StageStartBindDemoKinopioHouse::receiveMsg(const al::SensorMsg* pMsg,
    al::HitSensor* pSender, al::HitSensor* pReceiver) {
    if (al::isNerve(this, &NrvStageStartBindDemoKinopioHouseBindEnd))
        return false;
    if (al::isMsgBindStart(pMsg)) {
        if (mPuppeteers->getPuppeteerByPlayerIndex(pSender)->isBind())
            return false;
        if (al::isNerve(this, &NrvStageStartBindDemoKinopioHouseBindWait))
            al::setNerve(this, &NrvStageStartBindDemoKinopioHouseMessage);
        return true;
    }
    if (al::isMsgBindInit(pMsg)) {
        mPuppeteers->getPuppeteerByPlayerIndex(pSender)->startBind(pSender, pReceiver);
        return true;
    }
    if (al::isMsgBindCancel(pMsg)) {
        mPuppeteers->getPuppeteerByPlayerIndex(pSender)->cancelBind();
        if (mPuppeteers->isEndBindAll())
            al::setNerve(this, &NrvStageStartBindDemoKinopioHouseBindEnd);
        return true;
    }
    return false;
}

/** @brief Checks completion. @return Whether the actor is dead. */
bool StageStartBindDemoKinopioHouse::isEndDemo() const { return al::isDead(this); }

/** @brief Requests all players and starts Toad House music. */
void StageStartBindDemoKinopioHouse::exeBindWait() {
    if (al::isFirstStep(this)) {
        rc::requestBindAllPlayerAcceptReviveBubble(this, al::getHitSensor(this, nullptr));
        al::startSequenceBgm(this, "KinopioHouse", -1, 0);
    }
}

/** @brief Waits for the opening sound before the message. */
void StageStartBindDemoKinopioHouse::exeSound() {
    al::isFirstStep(this);
    if (al::isLessStep(this, 60))
        return;
    al::setNerve(this, &NrvStageStartBindDemoKinopioHouseMessage);
}

/** @brief Shows the introduction once, after the players settle. */
void StageStartBindDemoKinopioHouse::exeMessage() {
    if (al::isFirstStep(this) && GameDataFlagFunction::isAlreadyPlayKinopioHouse(GameDataHolderAccessor(this))) {
        al::setNerve(this, &NrvStageStartBindDemoKinopioHouseBindEnd);
        return;
    }
    if (al::isStep(this, 60)) {
        int port = rc::calcPadPortByFirstActiveUser(GameDataHolderAccessor(this));
        mWindow->appearWithSystemMessage("WindowMessage", "FirstPlayKinopioHouse", port);
        GameDataFlagFunction::setPlayKinopioHouse(GameDataHolderAccessor(this));
    }
    if (al::isLessStep(this, 60))
        return;
    if (!mWindow->isAlive())
        al::setNerve(this, &NrvStageStartBindDemoKinopioHouseBindEnd);
}

/** @brief Releases every bound player and removes the event. */
void StageStartBindDemoKinopioHouse::exeBindEnd() {
    if (al::isFirstStep(this)) {
        for (int i = 0; i < mPuppeteers->getPuppeteerNumMax(); ++i) {
            auto* puppet = mPuppeteers->getPuppeteer(i);
            if (puppet->isBind())
                puppet->endBindOnGround();
        }
    }
    kill();
}
