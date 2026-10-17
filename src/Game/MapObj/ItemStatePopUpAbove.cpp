#include "MapObj/ItemStatePopUpAbove.hpp"
#include "Enemy/EnemyStateUtil.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorClippingUtil.hpp"
#include "Library/LiveActor/Util/ActorFlagUtil.hpp"
#include "Library/LiveActor/Util/ActorMovementUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Util/AreaObjUtil.hpp"

namespace {
    const float sPunchMove[] = {30.0f, 20.0f, 10.0f, 7.5f, 5.0f, 2.5f};
    NERVE_DECL(ItemStatePopUpAbove, Punch);
    NERVE_DECL(ItemStatePopUpAbove, Fall);
    NERVES_MAKE_NOSTRUCT(ItemStatePopUpAbove, Punch, Fall)
}

ItemStatePopUpAbove::ItemStatePopUpAbove(al::LiveActor* pActor)
    : al::ActorStateBase("アイテム跳ね上げ(真上)ステート", pActor) {}

void ItemStatePopUpAbove::init() {
    initNerve(&NrvItemStatePopUpAbovePunch, 0);
}

void ItemStatePopUpAbove::appear() {
    al::ActorStateBase::appear();
    al::setNerve(this, &NrvItemStatePopUpAbovePunch);
    al::invalidateClipping(mHostActor);
    al::invalidateHitSensors(mHostActor);
    mIsInWater = rc::isInWaterArea(mHostActor);
}

void ItemStatePopUpAbove::kill() {
    al::ActorStateBase::kill();
    al::validateClipping(mHostActor);
    al::onCollide(mHostActor);
}

void ItemStatePopUpAbove::exePunch() {
    if (al::isFirstStep(this))
        al::tryStartAction(mHostActor, "PopUp");
    float move = sPunchMove[al::getNerveStep(this)];
    sead::Vector3f* trans = al::getTransPtr(mHostActor);
    trans->y = move + trans->y;
    if (al::isGreaterEqualStep(this, 5)) {
        al::setVelocity(mHostActor, -8.0f * al::getGravity(mHostActor) * mSpeedScale);
        al::setNerve(this, &NrvItemStatePopUpAboveFall);
    }
}

void ItemStatePopUpAbove::exeFall() {
    if (al::isStep(this, 16)) {
        al::validateHitSensors(mHostActor);
        al::onCollide(mHostActor);
    }
    if (EnemyStateUtil::tryKillByAreaOrMaterialCodeWithHitReaction(mHostActor))
        return;
    float gravity = mIsInWater ? 0.2f : 0.4f;
    gravity *= mSpeedScale;
    al::addVelocityToGravity(mHostActor, gravity);
    if (al::isGreaterEqualStep(this, 16) && al::isOnGround(mHostActor, 0, 0.0f))
        kill();
}
