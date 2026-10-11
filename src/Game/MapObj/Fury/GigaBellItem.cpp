#include "MapObj/Fury/GigaBellItem.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Util/PlayerUtil.hpp"

namespace {
NERVE_DECL(GigaBellItem, Wait);
NERVE_DECL(GigaBellItem, Hidden);
NERVE_DECL(GigaBellItem, PopUpAbove);
class GigaBellItemNrvWaitForCutscene : public al::Nerve {
public:
    void execute(al::NerveKeeper* keeper) const override {
        keeper->getParent<GigaBellItem>()->exeWait();
    }
};
NERVES_MAKE_NOSTRUCT(GigaBellItem, Wait, Hidden, PopUpAbove, WaitForCutscene)
}

GigaBellItem::GigaBellItem(const char*) : al::LiveActor("GigaBellItem") {}
GigaBellItem::~GigaBellItem() {}

void GigaBellItem::init(const al::ActorInitInfo& info) {
    al::initNerve(this, &NrvGigaBellItemWait, 0);
    al::initActorWithArchiveName(this, info, "GigaBell", nullptr);
    al::setSensorRadius(this, "Push", 900.0f);
    al::invalidateHitSensors(this);
    al::invalidateCollisionParts(this);
    makeActorDead();
    al::invalidateClipping(this);
}

void GigaBellItem::appear() { al::LiveActor::appear(); }
void GigaBellItem::kill() { al::LiveActor::kill(); }

void GigaBellItem::appearHidden() {
    al::LiveActor::appear();
    al::setNerve(this, &NrvGigaBellItemHidden);
    al::hideModelIfShow(this);
    al::tryStartMclAnimIfNotPlaying(this, "BaseColor");
}

void GigaBellItem::appearPopUpAbove() {
    al::LiveActor::appear();
    al::setNerve(this, &NrvGigaBellItemPopUpAbove);
    al::showModelIfHide(this);
    al::tryStartMclAnimIfNotPlaying(this, "BaseColor");
}

void GigaBellItem::appearPopUpAboveSilent() {
    al::LiveActor::appear();
    al::setNerve(this, &NrvGigaBellItemWait);
    al::showModelIfHide(this);
    al::tryStartMclAnimIfNotPlaying(this, "BaseColor");
}

bool GigaBellItem::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor*) {
    if (!al::isSensorPlayer(sender))
        return false;
    if (al::isNerve(this, &NrvGigaBellItemHidden))
        return false;
    if (al::isMsgItemGetAll(msg)) {
        al::startAction(this, "WaitStartUp");
        al::tryKillEmitterAndParticleAll(this);
        al::stopAllSeFromUser(this, 5);
        rc::tryChangeToGigaClimbMario(this, al::getSensorHost(sender), false);
        if (mIsRespawn)
            al::setNerve(this, &NrvGigaBellItemHidden);
        else
            kill();
        al::hideModelIfShow(this);
        al::startHitReactionGet(this);
        al::setEffectParticleScale(this, "Get", 10.0f);
        al::invalidateHitSensors(this);
        return true;
    }
    return false;
}

void GigaBellItem::exeWait() {
    if (al::isFirstStep(this) && al::isNerve(this, &NrvGigaBellItemWait)) {
        al::startAction(this, "WaitStartUp");
        al::emitEffect(this, "Before", nullptr);
        al::tryEmitEffect(this, "Before01", nullptr);
        al::validateHitSensor(this, "Push");
    }
}

void GigaBellItem::exePopUpAbove() {
    if (al::isFirstStep(this))
        al::startAction(this, "Appear");
    if (al::isStep(this, 60))
        al::validateHitSensor(this, "Push");
    if (al::isActionEnd(this))
        al::setNerve(this, &NrvGigaBellItemWait);
}

void GigaBellItem::exeHidden() {
    if (al::isGreaterEqualStep(this, 3600)) {
        al::LiveActor* player = al::tryFindAlivePlayerActorFirst(this);
        if (player) {
            sead::Vector3f playerPos = al::getTrans(player);
            if ((playerPos - al::getTrans(this)).squaredLength() >= 4000000.0f)
                appearPopUpAbove();
        }
    }
}

void GigaBellItem::stopForCutscene() {
    al::setNerve(this, &NrvGigaBellItemWaitForCutscene);
    al::startAction(this, "WaitStartUp");
    al::setActionFrameRate(this, 0.0f);
    al::stopAllSeFromUser(this, 5);
}
