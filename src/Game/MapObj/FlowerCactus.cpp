#include "MapObj/FlowerCactus.hpp"
#include "Enemy/EnemyStateBlowDown.hpp"
#include "Enemy/EnemyStateUtil.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/ScoreUtil.hpp"
namespace {
NERVE_DECL(FlowerCactus, Wait);
NERVE_DECL(FlowerCactus, BlowDown);
NERVE_DECL(FlowerCactus, Break);
NERVE_DECL(FlowerCactus, Trampled);
NERVE_DECL(FlowerCactus, ReactionTouch);
NERVE_DECL(FlowerCactus, ReactionEnd);
NERVE_DECL(FlowerCactus, Reaction);
NERVES_MAKE_STRUCT(FlowerCactus, Wait, BlowDown, Break, Trampled, ReactionTouch, ReactionEnd, Reaction)
}
FlowerCactus::FlowerCactus(const char* name) : al::LiveActor(name) {}
FlowerCactus::~FlowerCactus() {}
void FlowerCactus::init(const al::ActorInitInfo& info) {
    al::initActor(this, info);
    al::tryGetArg(&mColor, info, "Color");
    al::initNerve(this, &NrvFlowerCactus.Wait, 1);
    mBlowDownParam = new EnemyStateBlowDownParam(false);
    mBlowDown = new EnemyStateBlowDown(this, mBlowDownParam);
    al::initNerveState(this, mBlowDown, &NrvFlowerCactus.BlowDown, "state:BlowDown");
    mTraceModel = al::tryGetSubActor(this, "サボテン残留モデル");
    if (mTraceModel) mTraceModel->kill();
    makeActorAppeared();
}
void FlowerCactus::attackSensor(al::HitSensor* sender, al::HitSensor* receiver) {
    if (al::isNerve(this, &NrvFlowerCactus.BlowDown) || al::isNerve(this, &NrvFlowerCactus.Break)) return;
    if (al::isSensorPlayer(receiver)) al::sendMsgPush(receiver, sender);
    if (al::isSensorEnemyBody(receiver)) al::sendMsgPush(receiver, sender);
}
bool FlowerCactus::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor* receiver) {
    if (al::isNerve(this, &NrvFlowerCactus.BlowDown)) return false;
    if (al::isNerve(this, &NrvFlowerCactus.Break)) return false;
    if (al::isMsgPlayerCooperationHipDrop(msg)) return false;
    if (al::isMsgPlayerBodyLanding(msg)) return false;
    if (al::isMsgPlayerTrample(msg)) {
        al::setNerve(this, &NrvFlowerCactus.Trampled);
        return true;
    }
    if (al::isMsgPlayerObjHipDropAll(msg)) {
        doBreak(msg, sender);
        al::setNerve(this, &NrvFlowerCactus.Break);
        return true;
    }
    if (EnemyStateUtil::tryRequestBlowDownAndNextNerve(msg, sender, receiver, mBlowDown, &NrvFlowerCactus.BlowDown, false)) {
        doBreak(msg, sender);
        return true;
    }
    if (al::isMsgPlayerSlidingAttack(msg) || al::isMsgPlayerObjRollingAttack(msg)) {
        doBreak(msg, sender);
        EnemyStateUtil::requestBlowDown(msg, sender, receiver, mBlowDown, false);
        al::setNerve(this, &NrvFlowerCactus.BlowDown);
        return true;
    }
    if (al::isMsgPlayerObjTouch(msg)) return tryChangeNerveToReaction();
    return false;
}
void FlowerCactus::doBreak(const al::SensorMsg* msg, al::HitSensor* sender) {
    rc::setAppearItemFactorByMsg(this, msg, sender);
    rc::addScoreCombo(this, sender, msg, 0.0f);
}
bool FlowerCactus::tryChangeNerveToReaction() {
    if (al::isNerve(this, &NrvFlowerCactus.ReactionEnd)) {
        al::setNerve(this, &NrvFlowerCactus.ReactionEnd);
        return true;
    }
    if (al::isNerve(this, &NrvFlowerCactus.Wait)) {
        al::setNerve(this, &NrvFlowerCactus.Reaction);
        return true;
    }
    return false;
}
bool FlowerCactus::receiveMsgScreenPoint(const al::SensorMsg* msg, al::ScreenPointer*, al::ScreenPointTarget*) {
    if (al::isNerve(this, &NrvFlowerCactus.Break)) return false;
    if (al::isNerve(this, &NrvFlowerCactus.BlowDown)) return false;
    if (al::isMsgTouchAssist(msg)) {
        if (!al::isNerve(this, &NrvFlowerCactus.Trampled)) al::setNerve(this, &NrvFlowerCactus.ReactionTouch);
        return true;
    }
    return false;
}
void FlowerCactus::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait");
        al::startMtpAnim(this, "Color");
        al::setMtpAnimFrameAndStop(this, float(mColor));
    }
}
void FlowerCactus::exeTrampled() {
    if (al::isFirstStep(this)) al::startAction(this, "Trampled");
    if (al::isActionEnd(this)) al::setNerve(this, &NrvFlowerCactus.Wait);
}
void FlowerCactus::exeBlowDown() {
    if (al::isFirstStep(this) && mTraceModel) mTraceModel->appear();
    if (al::updateNerveState(this)) {
        al::appearItem(this);
        al::startHitReactionDisappear(this);
        kill();
    }
}
void FlowerCactus::exeBreak() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Break");
        if (mTraceModel) mTraceModel->appear();
    }
    if (al::isActionEnd(this)) { al::appearItem(this); kill(); }
}
void FlowerCactus::exeReaction() {
    if (al::isFirstStep(this)) al::startAction(this, "Reaction");
    if (al::isActionEnd(this)) al::setNerve(this, &NrvFlowerCactus.ReactionEnd);
}
void FlowerCactus::exeReactionEnd() {
    if (al::isFirstStep(this)) al::startAction(this, "Wait");
    if (al::isGreaterEqualStep(this, 5)) al::setNerve(this, &NrvFlowerCactus.Wait);
}
void FlowerCactus::exeReactionTouch() {
    al::tryStartActionIfNotPlaying(this, "ReactionTouchSlide");
    if (al::isGreaterEqualStep(this, 5)) al::setNerve(this, &NrvFlowerCactus.Wait);
}
