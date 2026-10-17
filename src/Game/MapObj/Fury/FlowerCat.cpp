#include "Library/Math/MathUtil.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "MapObj/Fury/FlowerCat.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Project/Audio/AudioSystem.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Enemy/EnemyStateUtil.hpp"
#include "Util/DrcUtil.hpp"
#include "Util/ItemUtil.hpp"
namespace {
    NERVE_DECL(FlowerCat, Wait);
    NERVE_DECL(FlowerCat, Reaction);
    NERVES_MAKE_NOSTRUCT(FlowerCat, Wait, Reaction)
}
FlowerCat::FlowerCat(const char* name) : al::LiveActor(name) {}
void FlowerCat::init(const al::ActorInitInfo& info) {
    al::initMapPartsActor(this, info, nullptr, 0);
    al::initNerve(this, &NrvFlowerCatWait, 0);
    mCottonGone = false;
    al::emitEffect(this, "Cotton", nullptr);
    bool rotate = false;
    if (al::tryGetArg(&rotate, info, "IsRotateY") && rotate) al::rotateQuatYDirDegree(this, al::getRandom(-25.0f, 25.0f));
    al::getTransPtr(this)->y += 5.61307f;
    bool reverse = false;
    al::tryGetArg(&reverse, info, "IsRhythmAnimReverse");
    al::setShadowFixed(this, true);
    makeActorAppeared();
}
void FlowerCat::control() {
    if (!mCottonGone && !al::isNerve(this, &NrvFlowerCatReaction) && al::isMicInputOn(this)) {
        if (!((al::getTrans(this) - al::findNearestPlayerPos(this)).length() > 1500.0f)) {
            mReactionCooldown = 0;
            al::setNerve(this, &NrvFlowerCatReaction);
            return;
        }
    }
    if (mReactionCooldown - 1 >= 0) --mReactionCooldown;
    if (al::isNerve(this, &NrvFlowerCatWait) && mReactionCooldown >= 0 && al::isMicInputOn(this)) al::setNerve(this, &NrvFlowerCatReaction);
}
bool FlowerCat::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor*) {
    if (!al::isNerve(this, &NrvFlowerCatWait)) return false;
    if (al::isMsgEnemyAttackFire(msg) || al::isMsgPlayerFireBallAttack(msg)) {
        al::setNerve(this, &NrvFlowerCatReaction);
    } else if (!mCottonGone && al::isMsgPlayerCooperationHipDrop(msg)) {
        tryAppearItem(msg, sender, true, false);
        al::setNerve(this, &NrvFlowerCatReaction);
    } else if (((al::isSensorPlayer(sender) || al::isSensorKoopaJr(sender)) && al::isMsgPlayerItemGet(msg)) || al::isMsgBallTrample(msg) ||
               (EnemyStateUtil::isMsgBlowDown(msg) && !al::isMsgPlayerBodyLanding(msg)) || al::isMsgNpcTouch(msg) || al::isSensorEnemyAttack(sender)) {
        const sead::Vector3f& velocity = al::getActorVelocity(sender);
        if (al::isNearZero(sead::Mathf::sqrt(velocity.x * velocity.x + velocity.z * velocity.z), 0.001f) && mReactionCooldown >= 1) {
            mReactionCooldown = 30;
        } else {
            if (!al::isSensorEnemyAttack(sender)) tryAppearItem(msg, sender, true, false);
            al::setNerve(this, &NrvFlowerCatReaction);
        }
    }
    return false;
}
void FlowerCat::tryAppearItem(const al::SensorMsg* msg, const al::HitSensor* sender, bool direct, bool indirect) {
    if (!mItemTiming) return;
    if (indirect) {
        auto* sensor = DrcFunction::tryFindDrcPlayerSensor(this, static_cast<const al::HitSensor*>(nullptr));
        if (!sensor) return;
        al::setAppearItemFactor(this, "間接攻撃", sensor);
    } else if (direct) al::setAppearItemFactor(this, "直接攻撃", sender);
    else rc::setAppearItemFactorByMsg(this, msg, sender);
    al::appearItemTiming(this, mItemTiming);
    mItemTiming = nullptr;
}
bool FlowerCat::receiveMsgScreenPoint(const al::SensorMsg* msg, al::ScreenPointer* pointer, al::ScreenPointTarget*) {
    if (al::isMsgTouchAssistTrig(msg)) {
        tryAppearItemScreenPointer(msg, pointer);
        al::setNerve(this, &NrvFlowerCatReaction);
    } else if (!al::isNerve(this, &NrvFlowerCatReaction) && al::isMsgTouchAssist(msg)) {
        if (mReactionCooldown >= 1) mReactionCooldown = 30;
        else {
            tryAppearItemScreenPointer(msg, pointer);
            al::setNerve(this, &NrvFlowerCatReaction);
        }
    }
    return false;
}
void FlowerCat::tryAppearItemScreenPointer(const al::SensorMsg* msg, const al::ScreenPointer* pointer) {
    if (!mItemTiming) return;
    rc::setAppearItemFactorByMsg(this, msg, pointer);
    al::appearItemTiming(this, mItemTiming);
    mItemTiming = nullptr;
}
void FlowerCat::exeWait() { al::isFirstStep(this); }
void FlowerCat::exeReaction() {
    if (al::isFirstStep(this) && !mCottonGone) {
        tryAppearItem(nullptr, nullptr, false, true);
        al::emitEffect(this, "ReactionMic", nullptr);
        al::startSe(this, "PgBlowFtuff", nullptr);
        al::deleteEffect(this, "Cotton");
        mCottonGone = true;
    }
    al::setNerve(this, &NrvFlowerCatWait);
}
