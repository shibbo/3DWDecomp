#include "MapObj/TractorBubblePuppeteer.hpp"
#include "MapObj/TractorBubble.hpp"
#include "Player/Normal/PlayerActor.hpp"
#include "Player/PlayerBindEndParam.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/PlayerPuppetUtil.hpp"
#include "Util/PlayerUtil.hpp"

namespace {
NERVE_DECL(TractorBubblePuppeteer, End)
NERVE_DECL(TractorBubblePuppeteer, ActivatePlayer)
NERVE_DECL(TractorBubblePuppeteer, BubbleStart)
NERVE_DECL(TractorBubblePuppeteer, Bubble)
class TractorBubblePuppeteerNrvBindWaitScreenOut : public al::Nerve {
    void execute(al::NerveKeeper* keeper) const override {
        keeper->getParent<TractorBubblePuppeteer>()->exeBindWait();
    }
};
class TractorBubblePuppeteerNrvBindWaitInput : public al::Nerve {
    void execute(al::NerveKeeper* keeper) const override {
        keeper->getParent<TractorBubblePuppeteer>()->exeBindWait();
    }
};
NERVES_MAKE_NOSTRUCT(TractorBubblePuppeteer, BindWaitScreenOut, BindWaitInput, ActivatePlayer, Bubble)
struct {
    TractorBubblePuppeteerNrvEnd End;
    TractorBubblePuppeteerNrvBubbleStart BubbleStart;
    PlayerBindEndParam endParam = {"BubbleJump", 0, 10, true, false, true, 0, 2.2f, 5, true};
    PlayerBindEndParam punchEndParam = {"BubbleJump", 0, 10, true, false, true, 0, -1.0f, 30, true};
} NrvTractorBubblePuppeteer;
}  // namespace

TractorBubblePuppeteer::TractorBubblePuppeteer(TractorBubble* bubble)
    : BindPuppeteer("泡バインド操作"), mBubble(bubble), mPlayerActor(nullptr) {
    initNerve(&NrvTractorBubblePuppeteer.End, 0);
}
void TractorBubblePuppeteer::startBubbleWithScreenOut(al::LiveActor* player) {
    if (isBind()) return;
    al::setNerve(this, &NrvTractorBubblePuppeteerBindWaitScreenOut);
    mPlayerActor = player;
}
void TractorBubblePuppeteer::startBubbleWithInput(al::LiveActor* player) {
    if (isBind()) return;
    al::setNerve(this, &NrvTractorBubblePuppeteerBindWaitInput);
    mPlayerActor = player;
}
void TractorBubblePuppeteer::startBubbleWithActivate(al::LiveActor* player) {
    if (isBind()) return;
    al::setNerve(this, &NrvTractorBubblePuppeteerActivatePlayer);
    mPlayerActor = player;
}
void TractorBubblePuppeteer::endBubble() {
    if (!isBind()) return;
    rc::startPuppetSe(getPlayerPuppet(), "RetryFromBubble");
    sead::Vector3f velocity(0, 10, 0);
    rc::setPuppetVelocity(getPlayerPuppet(), velocity);
    rc::validatePuppetSensors(getPlayerPuppet());
    rc::clearPuppetExPush(getPlayerPuppet());
    endBind(&NrvTractorBubblePuppeteer.endParam);
    mPlayerActor = nullptr;
    al::setNerve(this, &NrvTractorBubblePuppeteer.End);
}
void TractorBubblePuppeteer::endBubblePunch() {
    if (!isBind()) return;
    rc::startPuppetSe(getPlayerPuppet(), "RetryFromBubble");
    sead::Vector3f velocity(0, 10, 0);
    rc::setPuppetVelocity(getPlayerPuppet(), velocity);
    rc::clearPuppetExPush(getPlayerPuppet());
    endBind(&NrvTractorBubblePuppeteer.punchEndParam);
    mPlayerActor = nullptr;
    al::setNerve(this, &NrvTractorBubblePuppeteer.End);
}
void TractorBubblePuppeteer::endBindForce() {
    rc::validatePuppetSensors(getPlayerPuppet());
    rc::clearPuppetExPush(getPlayerPuppet());
    endBind(nullptr);
    mPlayerActor = nullptr;
    al::setNerve(this, &NrvTractorBubblePuppeteer.End);
}
void TractorBubblePuppeteer::update() { updateNerve(); }
bool TractorBubblePuppeteer::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender,
                                      al::HitSensor* receiver) {
    if (al::isMsgBindStart(msg)) return true;
    if (al::isMsgBindInit(msg)) {
        startBind(sender, receiver);
        mBubble->receiveBindInit(sender, receiver);
        al::setNerve(this, &NrvTractorBubblePuppeteer.BubbleStart);
        return true;
    }
    if (al::isMsgBindCancel(msg)) {
        mBubble->receiveBindCancel();
        rc::validatePuppetSensors(getPlayerPuppet());
        cancelBind();
        mPlayerActor = nullptr;
        al::setNerve(this, &NrvTractorBubblePuppeteer.End);
        return true;
    }
    return false;
}
void TractorBubblePuppeteer::exeActivatePlayer() {
    auto* player = static_cast<PlayerActor*>(mPlayerActor);
    rc::initPlayerFigureType(player, 0, false);
    rc::cancelInvincibleMarioForce(player, false, false);
    rc::cancelGiantMarioForce(player);
    rc::cancelPanelDash(player);
    rc::revivePlayer(player);
    (void)static_cast<int>(player->getChara());
    int user = rc::calcControlUserIdByCharacterType(GameDataHolderAccessor(player), player->getChara(), true);
    int port = rc::getControlUserPortNumber(GameDataHolderAccessor(player), user);
    sead::Vector3f pos(0, 10000, 0);
    rc::activatePlayer(player, port, &pos, nullptr);
    rc::requestPlayerBindTractorBubble(player, al::getHitSensor(mBubble, "BindOutScreen"));
    al::setNerve(this, &NrvTractorBubblePuppeteerBindWaitScreenOut);
}
void TractorBubblePuppeteer::exeBindWait() {
    rc::requestPlayerBindTractorBubble(mPlayerActor, al::getHitSensor(mBubble,
        al::isNerve(this, &NrvTractorBubblePuppeteerBindWaitInput) ? "BindInput" : "BindOutScreen"));
}
void TractorBubblePuppeteer::exeBubbleStart() {
    if (al::isFirstStep(this)) {
        rc::cancelGiantMario(mPlayerActor);
        rc::cancelPanelDash(mPlayerActor);
        if (rc::isPuppetHidden(getPlayerPuppet())) rc::showPuppet(getPlayerPuppet());
        rc::startPuppetAction(getPlayerPuppet(), "BubbleStart");
        rc::setPuppetVelocity(getPlayerPuppet(), sead::Vector3f::zero);
        rc::setPuppetUpVec(getPlayerPuppet(), sead::Vector3f::ey);
        rc::invalidatePuppetSensors(getPlayerPuppet());
        rc::clearPuppetCollisionInfo(getPlayerPuppet());
    }
    sead::Vector3f pos = al::getTrans(mBubble);
    pos.y -= 70.0f;
    rc::setPuppetTrans(getPlayerPuppet(), pos);
    if (rc::isPuppetActionEnd(getPlayerPuppet())) al::setNerve(this, &NrvTractorBubblePuppeteerBubble);
}
void TractorBubblePuppeteer::exeBubble() {
    if (al::isFirstStep(this)) rc::startPuppetAction(getPlayerPuppet(), "BubbleWait");
    sead::Vector3f pos = al::getTrans(mBubble);
    pos.y -= 70.0f;
    rc::setPuppetTrans(getPlayerPuppet(), pos);
}
void TractorBubblePuppeteer::exeEnd() {}
