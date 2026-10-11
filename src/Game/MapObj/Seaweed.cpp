#include "MapObj/Seaweed.hpp"
#include "MapObj/ActorMicRumbler.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Movement/AnimScaleController.hpp"
#include "Library/Nerve/NerveSetup.hpp"
namespace {
    NERVE_DECL(Seaweed, Wait);
    NERVE_DECL(Seaweed, Reaction);
    NERVE_DECL(Seaweed, ReactionAfter);
    NERVES_MAKE_NOSTRUCT(Seaweed, Wait, Reaction, ReactionAfter)
    struct SeaweedAnimScaleParam : al::AnimScaleParam {
        SeaweedAnimScaleParam() { _24 = 1.0f; _2c = 0.025f; }
    };
    SeaweedAnimScaleParam sAnimScaleParam;
}
Seaweed::Seaweed(const char* name) : al::LiveActor(name) {}
Seaweed::~Seaweed() {}
void Seaweed::init(const al::ActorInitInfo& info) {
    al::initMapPartsActor(this, info, nullptr, 0);
    al::initNerve(this, &NrvSeaweedWait, 0);
    al::startActionAtRandomFrame(this, "Wait");
    mMicRumbler = new ActorMicRumbler(this, &sAnimScaleParam);
    int colorFrame = 0;
    if (al::tryGetArg(&colorFrame, info, "ColorFrame"))
        al::startMtpAnimAndSetFrameAndStop(this, "Color", colorFrame);
    makeActorAppeared();
}
void Seaweed::control() { mMicRumbler->update(); }
bool Seaweed::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor*) {
    if (((al::isSensorPlayer(sender) || al::isSensorKoopaJr(sender)) && al::isMsgPlayerItemGet(msg)) ||
        al::isMsgBallAttack(msg) || al::isMsgBallTrample(msg) || al::isMsgEnemyTouch(msg) ||
        al::isMsgExplosion(msg) || al::isMsgKickKouraAttack(msg) || al::isMsgPlayerBoomerangAttack(msg) ||
        al::isMsgPlayerCooperationHipDrop(msg) || al::isMsgPlayerFireBallAttack(msg) || al::isMsgPlayerGiantHipDrop(msg)) {
        if (al::isNerve(this, &NrvSeaweedWait)) al::setNerve(this, &NrvSeaweedReaction);
        else if (mReactionCooldown > 0) mReactionCooldown = 30;
    }
    return false;
}
bool Seaweed::receiveMsgScreenPoint(const al::SensorMsg* msg, al::ScreenPointer*, al::ScreenPointTarget*) {
    if (al::isMsgTouchAssistAll(msg)) {
        if (al::isNerve(this, &NrvSeaweedWait)) al::setNerve(this, &NrvSeaweedReaction);
        return true;
    }
    return false;
}
void Seaweed::exeWait() {}
void Seaweed::exeReaction() {
    if (al::isFirstStep(this)) al::startAction(this, "Reaction");
    if (al::isActionEnd(this)) al::setNerve(this, &NrvSeaweedReactionAfter);
}
void Seaweed::exeReactionAfter() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait");
        mReactionCooldown = 30;
    }
    if (mReactionCooldown > 0) --mReactionCooldown;
    else mReactionCooldown = 0;
    if (mReactionCooldown == 0) al::setNerve(this, &NrvSeaweedWait);
}
