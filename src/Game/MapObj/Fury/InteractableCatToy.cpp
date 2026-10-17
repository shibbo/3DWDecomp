#include "MapObj/Fury/InteractableCatToy.hpp"
#include "MapObj/CoinBlow.hpp"
#include "Scene/ProjectActorFactory.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Controller/PadRumbleFunction.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
namespace {
    NERVE_DECL(InteractableCatToy, Wait);
    NERVES_MAKE_NOSTRUCT(InteractableCatToy, Wait)
}
InteractableCatToy::InteractableCatToy(const char* name) : al::LiveActor(name) {}
InteractableCatToy::~InteractableCatToy() {}
void InteractableCatToy::init(const al::ActorInitInfo& info) {
    al::initActorWithArchiveName(this, info, "InteractableCatToy", nullptr);
    al::initNerve(this, &NrvInteractableCatToyWait, 0);
    ProjectActorFactory factory;
    if (al::calcLinkChildNum(info, "AttachTo")) {
        mAttachment = al::createLinksActorFromFactory(factory, info, "AttachTo", 0);
        mAttachment->makeActorAppeared();
        mRestLength = (al::getTrans(this) - al::getTrans(mAttachment)).length();
    }
    mCoins.allocBuffer(mCoinCount, nullptr);
    for (int i = 0; i < mCoinCount; ++i) {
        mCoins.pushBack(new CoinBlow("CoinBlow"));
        mCoins.unsafeAt(i)->init(info);
        mCoins.at(i)->setOffSensor(14);
    }
    mAttackCooldown = 8;
    al::invalidateClipping(this);
    makeActorAppeared();
}
void InteractableCatToy::attackSensor(al::HitSensor* sender, al::HitSensor* receiver) {
    if (al::isSensorName(sender, "Push") && al::isSensorPlayer(receiver)) {
        sead::Vector3f dir(0.0f, 0.0f, 0.0f);
        al::calcDirBetweenSensors(&dir, receiver, sender);
        if (al::isNearZero(dir, 0.001f)) dir.set(sead::Vector3f::ey);
        al::addVelocity(this, dir);
    }
}
void InteractableCatToy::Push(al::HitSensor* sender, al::HitSensor* receiver, float force) {
    sead::Vector3f dir(0.0f, 0.0f, 0.0f);
    al::calcDirBetweenSensors(&dir, sender, receiver);
    if (al::isNearZero(dir, 0.001f)) dir.set(sead::Vector3f::ey);
    dir *= force;
    al::addVelocity(this, dir);
}
bool InteractableCatToy::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor* receiver) {
    if (!al::isMsgPlayerClimbAttack(msg)) return false;
    Push(sender, receiver, 5.0f);
    if (mAttackCooldown < 8) return true;
    mAttackCooldown = 0;
    al::setScaleAll(this, 2.0f);
    al::startSe(this, "Attack", nullptr);
    alPadRumbleFunction::startPadRumbleNo3D(this, "弱", -1, true);
    if (mCoinsDropped < mCoinCount) al::emitEffectCurrentPos(this, "Reward");
    else al::emitEffectCurrentPos(this, "Empty");
    if (al::calcSpeed(this) > 20.0f) al::startSe(this, "Move", nullptr);
    int count = al::getRandom(1, 3);
    for (int i = 0; i < count; ++i) TryDropCoin(sender, receiver);
    return true;
}
void InteractableCatToy::TryDropCoin(al::HitSensor* sender, al::HitSensor* receiver) {
    if (mCoinsDropped >= mCoinCount) return;
    CoinBlow* coin = mCoins.at(mCoinsDropped);
    al::setTrans(coin, al::getTrans(this));
    al::resetPosition(coin, false);
    coin->setLifeTime(600);
    coin->appearWithHitReaction();
    al::startSe(coin, "PgAppearLight", nullptr);
    sead::Vector3f dir(0.0f, 0.0f, 0.0f);
    al::calcDirBetweenSensors(&dir, sender, receiver);
    if (al::isNearZero(dir, 0.001f)) dir.set(sead::Vector3f::ey);
    dir *= 30.0f;
    al::addRandomVector(&dir, dir, 7.5f);
    al::setVelocity(coin, dir);
    ++mCoinsDropped;
}
void InteractableCatToy::exeWait() {
    if (mAttackCooldown < 8) ++mAttackCooldown;
    if (mAttachment) {
        float distanceSquared = (al::getTrans(this) - al::getTrans(mAttachment)).squaredLength();
        if (distanceSquared > mRestLength * mRestLength) {
            sead::Vector3f dir = al::getTrans(mAttachment) - al::getTrans(this);
            al::normalizeOrZero(&dir);
            al::addVelocity(this, dir * (2.0f * (distanceSquared / (mRestLength * mRestLength) - 1.0f)));
        }
    }
    al::addVelocityToGravity(this, 0.7f);
    al::scaleVelocity(this, 0.995f);
    al::limitVelocity(this, 25.0f);
    al::setScaleAll(this, al::lerpValue(0.2f, al::getScaleX(this), 1.0f));
    if (al::isCollidedWall(this) || al::isCollidedCeiling(this) || al::isCollidedGround(this))
        al::reboundVelocityFromCollision(this, 0.5f, 0.0f, 1.0f);
}

