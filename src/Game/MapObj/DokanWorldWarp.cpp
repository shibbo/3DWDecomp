#include "MapObj/DokanWorldWarp.hpp"
#include "MapObj/BindPuppeteerGroup.hpp"
#include "MapObj/DokanBindPuppeteer.hpp"
#include "Layout/DokanGuideBalloon.hpp"
#include "Player/IUsePlayerKeyConfig.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/GhostPlayerUtil.hpp"
#include "Util/PlayerUtil.hpp"
namespace {
NERVE_DECL(DokanWorldWarp, Wait);
NERVE_DECL(DokanWorldWarp, PlayerIn);
NERVE_DECL(DokanWorldWarp, WaitStartWorldWarp);
NERVES_MAKE_NOSTRUCT(DokanWorldWarp, Wait)
NERVES_MAKE_STRUCT(DokanWorldWarp, PlayerIn, WaitStartWorldWarp)
}
DokanWorldWarp::DokanWorldWarp(const char* name) : al::LiveActor(name) {}
DokanWorldWarp::~DokanWorldWarp() {}
void DokanWorldWarp::init(const al::ActorInitInfo& info) {
    al::initActor(this, info);
    al::initNerve(this, &NrvDokanWorldWarpWait, 0);
    mPuppeteers = new BindPuppeteerGroup("土管バインド操作グループ", al::getPlayerNumMax(this));
    for (int i = 0; i < mPuppeteers->getPuppeteerNumMax(); ++i) {
        auto* puppeteer = new DokanBindPuppeteer("土管バインド操作", false, true, nullptr);
        puppeteer->init(info);
        mPuppeteers->registerPuppeteer(puppeteer);
    }
    mBalloons = new DokanGuideBalloon*[rc::getControlUserNumMax()];
    for (int i = 0; i < rc::getControlUserNumMax(); ++i) {
        mBalloons[i] = new DokanGuideBalloon(al::getTransPtr(this));
        mBalloons[i]->init(info);
    }
    makeActorAppeared();
}
bool DokanWorldWarp::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor* receiver) {
    if (al::isMsgPlayerFloorTouch(msg)) {
        setLayout(sender);
        getPuppeteer(sender)->setOnPlayerCountMax();
        return true;
    }
    if (al::isMsgBindStart(msg)) {
        if (al::isSensorName(receiver, "DokanHipDropBind")) {
            mHipDrop = al::isSensorPlayer(sender) && !rc::isPlayerOnGround(sender) && rc::isPlayerHipDropping(sender) && rc::getPlayerVelocity(sender).y < -1.0f;
            if (mHipDrop) getPuppeteer(sender)->setOnPlayerCountMax();
            if (!getPuppeteer(sender)->isEnableStartBind(mHipDrop, mBindAll)) {
                mHipDrop = false;
                return false;
            }
        } else {
            bool squat = rc::getPlayerKeyConfig(sender)->isPadHoldPlayerSquat();
            if (!getPuppeteer(sender)->isEnableStartBind(squat, mBindAll)) return false;
            mHipDrop = false;
        }
        if (!al::isNerve(this, &NrvDokanWorldWarp.PlayerIn)) al::setNerve(this, &NrvDokanWorldWarp.PlayerIn);
        return true;
    }
    if (al::isMsgBindInit(msg)) {
        mBinderSensor = receiver;
        getPuppeteer(sender)->startBindWorldWarp(sender, receiver, this, mBindAll, mHipDrop);
        return true;
    }
    if (al::isMsgBindCancel(msg)) {
        getPuppeteer(sender)->cancelBind();
        if (mPuppeteers->getPuppeteerNum() == 0)
            rc::cancelRequestBindAndResetDisableReviveBubbleForAllPlayer(this, receiver);
        return true;
    }
    return false;
}
void DokanWorldWarp::setLayout(const al::HitSensor* sensor) {
    if (rc::isPlayerBinded(sensor)) return;
    int user = rc::findControlUserId(sensor);
    if (al::isDead(mBalloons[user])) {
        int port = rc::getPlayerInputPort(sensor);
        mBalloons[user]->setPlayerSensor(sensor);
        mBalloons[user]->startShow(port);
    } else mBalloons[user]->playerTouched();
}
DokanBindPuppeteer* DokanWorldWarp::getPuppeteer(const al::HitSensor* sensor) const {
    return mPuppeteers->getPuppeteerByPlayerIndex<DokanBindPuppeteer>(sensor);
}
bool DokanWorldWarp::isGoal() const {
    return al::isNerve(this, &NrvDokanWorldWarp.PlayerIn) || al::isNerve(this, &NrvDokanWorldWarp.WaitStartWorldWarp);
}
bool DokanWorldWarp::isEndGoalDemo() const {
    return al::isNerve(this, &NrvDokanWorldWarp.WaitStartWorldWarp);
}
void DokanWorldWarp::updatePuppeteer() {
    for (int i = 0; i < mPuppeteers->getPuppeteerNum(); ++i)
        mPuppeteers->getPuppeteer<DokanBindPuppeteer>(i)->update();
}
bool DokanWorldWarp::isEnableStartWorldWarp() const {
    if (!rc::isAllPlayerBinded(this)) return false;
    rc::setDisableReviveBubbleForAllPlayer(al::getSensorHost(mBinderSensor));
    for (int i = 0; i < mPuppeteers->getPuppeteerNum(); ++i) {
        auto* puppeteer = mPuppeteers->getPuppeteer<DokanBindPuppeteer>(i);
        if (puppeteer->isBind() && !puppeteer->isWaitStartWorldWarp()) return false;
    }
    return true;
}
void DokanWorldWarp::exeWait() {
    if (al::isFirstStep(this)) al::tryStartAction(this, "Wait");
    updatePuppeteer();
}
void DokanWorldWarp::exePlayerIn() {
    if (al::isFirstStep(this)) al::invalidateClipping(this);
    updatePuppeteer();
    if (al::isGreaterStep(this, 130)) {
        rc::requestBindAllPlayer(this, mBinderSensor);
        mBindAll = true;
    }
    if (rc::isAllPlayerBinded(this)) {
        rc::setDisableReviveBubbleForAllPlayer(this);
        al::stopAllBgm(this, 105);
    }
    if (isEnableStartWorldWarp()) {
        for (int i = 0; i < rc::getControlUserNumMax(); ++i) mBalloons[i]->kill();
        rc::endRecordGhostPlayerRecorder(this, true);
        al::setNerve(this, &NrvDokanWorldWarp.WaitStartWorldWarp);
        alSeFunction::setIsStateAfterGoal(this, true);
    }
}
void DokanWorldWarp::exeWaitStartWorldWarp() {}
