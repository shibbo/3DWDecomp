#include "MapObj/GrassHigh.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Project/Audio/AudioSystem.hpp"
#include "Project/Effect/Core/EffectKeeper.hpp"
#include "Enemy/EnemyStateUtil.hpp"
namespace {
    NERVE_DECL(GrassHigh, Wait);
    NERVE_DECL(GrassHigh, Destroy);
    NERVE_DECL(GrassHigh, Reaction);
    NERVES_MAKE_NOSTRUCT(GrassHigh, Wait, Destroy, Reaction)
}
GrassHigh::GrassHigh(const char* name) : al::LiveActor(name) {}
void GrassHigh::init(const al::ActorInitInfo& info) {
    al::initActorWithArchiveName(this, info, "GrassHigh", nullptr);
    al::initNerve(this, &NrvGrassHighWait, 0);
    al::startActionAtRandomFrame(this, "Wait");
    al::rotateQuatYDirRandomDegree(this);
    bool depthShadow = false;
    al::tryGetArg(&depthShadow, info, "UsingDepthShadow");
    if (!depthShadow) al::setShadowFixed(this, true);
    makeActorAppeared();
}
void GrassHigh::attackSensor(al::HitSensor* sender, al::HitSensor* receiver) {
    if (al::isSensorEnemyBody(sender) && al::isSensorPlayer(receiver) && al::isNerve(this, &NrvGrassHighWait) &&
        al::getActorVelocity(receiver).length() < 0.01f)
        al::sendMsgPush(receiver, sender);
}
bool GrassHigh::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor* receiver) {
    if (!al::isSensorMapObj(receiver) || al::isNerve(this, &NrvGrassHighDestroy)) return false;
    if (al::isMsgLaserAttack(msg)) {
        al::setNerve(this, &NrvGrassHighDestroy);
        return true;
    }
    if (((al::isSensorPlayer(sender) || al::isSensorKoopaJr(sender)) && al::isMsgPlayerItemGet(msg)) ||
        al::isSensorEnemyBody(sender) || al::isMsgBallTrample(msg) || EnemyStateUtil::isMsgBlowDown(msg) || al::isMsgNpcTouch(msg)) {
        if (al::isNerve(this, &NrvGrassHighWait)) {
            if ((al::isSensorPlayer(sender) && al::isMsgPlayerItemGet(msg)) || al::isSensorEnemyBody(sender)) {
                if (al::getActorVelocity(sender).length() < 0.01f) return false;
            }
            al::setNerve(this, &NrvGrassHighReaction);
        }
    }
    return false;
}
bool GrassHigh::receiveMsgScreenPoint(const al::SensorMsg* msg, al::ScreenPointer*, al::ScreenPointTarget*) {
    if (al::isMsgTouchAssistAll(msg)) {
        if (al::isNerve(this, &NrvGrassHighWait)) al::setNerve(this, &NrvGrassHighReaction);
        return true;
    }
    return false;
}
void GrassHigh::exeWait() {
    if (al::isFirstStep(this)) al::tryStartActionIfNotPlaying(this, "Wait");
    if (al::isMicInputOn(this)) al::setNerve(this, &NrvGrassHighReaction);
}
void GrassHigh::exeReaction() {
    if (al::isFirstStep(this)) al::startAction(this, "Reaction");
    al::setNerveAtActionEnd(this, &NrvGrassHighWait);
}
void GrassHigh::exeDestroy() {
    if (al::isFirstStep(this)) {
        getEffectKeeper()->tryEmitEffect("Reaction", nullptr);
        al::hideModelIfShow(this);
    }
    if (al::isGreaterStep(this, 60)) kill();
}
