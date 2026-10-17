#include "MapObj/DoubleMario.hpp"
#include "MapObj/ItemBubble.hpp"
#include "MapObj/ItemStatePopUpFront.hpp"
#include "Scene/PlayerStocker.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Movement/RumbleCalculator.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Thread/Functor.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ScoreUtil.hpp"
namespace {
NERVE_DECL(DoubleMario, Wait);
NERVE_DECL(DoubleMario, PopUpFront);
NERVE_DECL(DoubleMario, AttachBubble);
NERVE_DECL(DoubleMario, ItemGetAfterWait);
NERVE_DECL(DoubleMario, Land);
NERVE_DECL(DoubleMario, LandWait);
NERVES_MAKE_STRUCT(DoubleMario, Wait, PopUpFront, AttachBubble, ItemGetAfterWait)
NERVES_MAKE_NOSTRUCT(DoubleMario, Land, LandWait)
ItemStatePopUpFrontParam sDoubleMarioParam(sead::Vector3f(0.0f, 11.0f, 5.0f), 0.4f, 0.99f, 30, 0.7f, true, "PopUp", false, nullptr);
}
DoubleMario::DoubleMario(const char* name, ItemBubble* bubble, bool delayedKill) : al::LiveActor(name), mBubble(bubble), mDelayedKill(delayedKill) {}
DoubleMario::~DoubleMario() {}
void DoubleMario::init(const al::ActorInitInfo& info) {
    al::initActorWithArchiveName(this, info, "DoubleMario", nullptr);
    al::initNerve(this, &NrvDoubleMario.Wait, 1);
    mPopUpFront = new ItemStatePopUpFront(this);
    al::initNerveState(this, mPopUpFront, &NrvDoubleMario.PopUpFront, "[state]跳ね上げ(前方)");
    mSensorRadius = al::getSensorRadius(this, "Body");
    mRumble = new al::RumbleCalculatorCosMultLinear(2.5f, 1.5707963705062866f, 0.2f, 30);
    if (mBubble) al::setNerve(this, &NrvDoubleMario.AttachBubble);
    al::listenStageSwitchOnKill(this, al::FunctorV0M(this, &DoubleMario::kill));
    makeActorAppeared();
}
void DoubleMario::initAfterPlacement() {
    al::updateMaterialCodeArea(this);
    if (al::isAlive(this)) PlayerStockerFunction::addDoubleItemAppearedNum(this);
}
void DoubleMario::appear() {
    PlayerStockerFunction::addDoubleItemAppearedNum(this);
    al::LiveActor::appear();
    if (mBubble) al::setNerve(this, &NrvDoubleMario.AttachBubble);
    else al::setNerve(this, &NrvDoubleMario.Wait);
}
void DoubleMario::kill() {
    if (al::isDead(this)) return;
    al::LiveActor::kill();
    PlayerStockerFunction::decDoubleItemAppearedNum(this);
}
void DoubleMario::control() {
    if (!mRumble->isEnd()) { mRumble->calc(); al::setScaleY(this, mRumble->getValueY() + 1.0f); }
    else al::setScaleY(this, 1.0f);
}
void DoubleMario::attackSensor(al::HitSensor* sender, al::HitSensor* receiver) {
    if (al::isNerve(this, &NrvDoubleMario.Wait) || al::isNerve(this, &NrvDoubleMario.AttachBubble)) return;
    if (mDelayedKill && al::isSensorMapObj(sender) && al::isSensorName(sender, "Push") && al::isSensorEnemyBody(receiver) && !al::sendMsgPushStrong(receiver, sender)) al::sendMsgPush(receiver, sender);
}
bool DoubleMario::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor* receiver) {
    if (al::isSensorName(receiver, "Push")) return false;
    if (al::isNerve(this, &NrvDoubleMario.ItemGetAfterWait)) return false;
    if (al::isNerve(this, &NrvDoubleMario.AttachBubble) || al::isNerve(this, &NrvDoubleMario.Wait)) {
    if (rc::isMsgItemBubbleBreak(msg)) {
        mPopUpFront->setParamDefault(); al::setNerve(this, &NrvDoubleMario.PopUpFront); return true;
    }
    if (al::isNerve(this, &NrvDoubleMario.AttachBubble)) {
        if (!rc::isMsgItemBubbleBreakAndGetItem(msg)) return false;
        if (!mBubble->isEnableGetPlayerSensor()) return false;
        sender = mBubble->getHitPlayerSensor();
        PlayerStockerFunction::appearDoubleMario(this, sender);
        rc::addScore(this, sender, 0.0f, 0);
        kill(); return true;
    }
    }
    if (al::isMsgPlayerFireBallAttack(msg) || al::isMsgPlayerBoomerangAttack(msg)) {
        if (mRumble->isEnd()) { al::startHitReactionHit(this); rc::requestHitReactionToAttacker(msg, receiver, sender); mRumble->start(0); }
        return al::isMsgPlayerFireBallAttack(msg);
    }
    if (!al::isSensorPlayer(sender)) return false;
    if (!al::isMsgItemGetDirectAll(msg)) return false;
    PlayerStockerFunction::appearDoubleMario(this, sender);
    rc::addScore(this, sender, 0.0f, 0);
    if (mDelayedKill) { al::hideModel(this); al::invalidateClipping(this); al::setNerve(this, &NrvDoubleMario.ItemGetAfterWait); }
    else kill();
    return true;
}
bool DoubleMario::receiveMsgScreenPoint(const al::SensorMsg* msg, al::ScreenPointer*, al::ScreenPointTarget*) { return al::isMsgTouchAssist(msg); }
void DoubleMario::appearPopUpFront() {
    al::invalidateHitSensors(this); al::invalidateClipping(this); appear();
    mPopUpFront->setParam(sDoubleMarioParam, nullptr); al::setNerve(this, &NrvDoubleMario.PopUpFront);
}
void DoubleMario::setSensorRadius(float radius) { al::setSensorRadius(this, "Body", radius); }
void DoubleMario::restoreSensorRadius() { al::setSensorRadius(this, "Body", mSensorRadius); }
void DoubleMario::exeWait() { if (al::isFirstStep(this)) { al::startAction(this, "Wait"); al::setVelocityZero(this); al::offCollide(this); } }
void DoubleMario::exeAttachBubble() {
    if (al::isFirstStep(this)) { al::startAction(this, "Wait"); al::setVelocityZero(this); al::offCollide(this); }
    if (al::isDead(mBubble)) al::setNerve(this, &NrvDoubleMario.PopUpFront);
}
void DoubleMario::exePopUpFront() { al::updateNerveStateAndNextNerve(this, &NrvDoubleMarioLand); rc::startHitReactionIfThroughWater(this); }
void DoubleMario::exeLand() {
    if (al::isFirstStep(this)) { al::startAction(this, "Land"); al::setVelocityZero(this); al::offCollide(this); }
    if (al::isActionEnd(this)) al::setNerve(this, &NrvDoubleMarioLandWait);
}
void DoubleMario::exeLandWait() { if (al::isFirstStep(this)) { al::startAction(this, "Wait"); al::setVelocityZero(this); al::offCollide(this); } }
void DoubleMario::exeItemGetAfterWait() { if (al::isGreaterEqualStep(this, 15)) { al::showModel(this); al::validateClipping(this); kill(); } }
