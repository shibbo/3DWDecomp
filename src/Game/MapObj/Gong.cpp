#include "MapObj/Gong.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Util/ProjectMsgUtil.hpp"

namespace {
    NERVE_DECL(Gong, Wait);
    NERVE_DECL(Gong, Sound);
    NERVES_MAKE_NOSTRUCT(Gong, Wait, Sound)
}

Gong::Gong(const char* pName) : al::LiveActor(pName) {}
Gong::~Gong() {}

void Gong::init(const al::ActorInitInfo& rInfo) {
    al::initActor(this, rInfo);
    al::initNerve(this, &NrvGongWait, 0);
    makeActorAppeared();
}

void Gong::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isNerve(this, &NrvGongSound) && al::isLessStep(this, 10) &&
        al::isSensorName(pSelf, "Explosion") && al::isSensorEnemyBody(pOther))
        rc::sendMsgGongShockwave(pOther, pSelf);
}

bool Gong::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) {
    if (al::isNerve(this, &NrvGongSound))
        return false;
    if (al::isMsgBallAttackCollide(pMsg) || al::isMsgExplosion(pMsg) || al::isMsgExplosionCollide(pMsg)) {
        al::startHitReaction(this, "命中[振動無効]");
        rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        al::setNerve(this, &NrvGongSound);
        return true;
    }
    if (al::isMsgPlayerBodyAttack(pMsg) || al::isMsgPlayerBoomerangAttack(pMsg) ||
        al::isMsgPlayerKouraAttack(pMsg) || al::isMsgPlayerRollingAttack(pMsg) ||
        al::isMsgPlayerSlidingAttack(pMsg) || al::isMsgPlayerClimbAttack(pMsg) ||
        al::isMsgPlayerTailAttack(pMsg) ||
        (al::isMsgPlayerSpinAttack(pMsg) &&
         !((al::getActorTrans(pOther) - al::getActorTrans(pSelf)).y >
             al::getSensorRadius(pOther) + al::getSensorRadius(pSelf))) ||
        al::isMsgKickKouraAttack(pMsg)) {
        al::startHitReactionHit(this);
        rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        al::setNerve(this, &NrvGongSound);
        return true;
    }
    if (al::isMsgPlayerFireBallAttack(pMsg)) {
        al::startHitReactionHit(this);
        al::setNerve(this, &NrvGongSound);
    }
    return false;
}

bool Gong::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                  al::ScreenPointTarget* pTarget) {
    if (al::isMsgTouchAssistTrig(pMsg)) {
        al::startHitReaction(this, "命中[振動無効]");
        al::setNerve(this, &NrvGongSound);
        return true;
    }
    return false;
}

void Gong::exeWait() {}

void Gong::exeSound() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Sound");
        al::tryOnStageSwitch(this, "SwitchSoundOn");
        al::validateHitSensor(this, "Explosion");
    }
    if (al::isNerve(this, &NrvGongSound) && al::isLessStep(this, 10))
        al::setSensorRadius(this, "Explosion", al::calcNerveValue(this, 10, 0.0f, 750.0f));
    if (al::isStep(this, 10)) {
        al::setSensorRadius(this, "Explosion", 0.0f);
        al::invalidateHitSensor(this, "Explosion");
    }
    if (al::isActionEnd(this) && al::isGreaterEqualStep(this, 10)) {
        al::setNerve(this, &NrvGongWait);
        al::setSensorRadius(this, "Explosion", 0.0f);
        al::invalidateHitSensor(this, "Explosion");
    }
}
