#include "MapObj/BindWarpEffect.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Util/PlayerUtil.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
namespace {
NERVE_DECL(BindWarpEffect, Move);
NERVE_DECL(BindWarpEffect, Wait);
NERVE_DECL(BindWarpEffect, Done);
NERVES_MAKE_NOSTRUCT(BindWarpEffect, Move, Wait, Done)
}
BindWarpEffect::BindWarpEffect() : al::LiveActor("バインドワープエフェクト") {}
BindWarpEffect::~BindWarpEffect() {}
void BindWarpEffect::init(const al::ActorInitInfo& info) {
    al::initActorWithArchiveName(this, info, "BindWarpEffect", nullptr);
    al::initNerve(this, &NrvBindWarpEffectMove, 0);
    makeActorDead();
}
const char* BindWarpEffect::getEffectName() const {
    if (mIsKoopaJr) return "BindWarpKoopaJr";
    if (mPlayerSensor) {
        if (rc::isPlayerCharaMario(mPlayerSensor)) return "BindWarpMario";
        if (rc::isPlayerCharaLuigi(mPlayerSensor)) return "BindWarpLuigi";
        if (rc::isPlayerCharaPeach(mPlayerSensor)) return "BindWarpPeach";
        if (rc::isPlayerCharaKinopio(mPlayerSensor)) return "BindWarpKinopio";
        if (rc::isPlayerCharaRosetta(mPlayerSensor)) return "BindWarpRosetta";
    }
    return "BindWarp";
}
void BindWarpEffect::start(const sead::Vector3f& startPos, const sead::Vector3f& endPos, bool noTrail) {
    makeActorAppeared();
    mPlayerSensor = nullptr;
    mStartPos.set(startPos);
    mEndPos.set(endPos);
    mEndPosPtr = nullptr;
    al::setTrans(this, mStartPos);
    al::emitEffect(this, noTrail ? "BindWarpNoTrail" : getEffectName(), nullptr);
    al::setNerve(this, &NrvBindWarpEffectWait);
}
void BindWarpEffect::start(const sead::Vector3f& startPos, const sead::Vector3f* endPos, bool noTrail) {
    makeActorAppeared();
    mPlayerSensor = nullptr;
    mStartPos.set(startPos);
    mEndPos.set(sead::Vector3f::zero);
    mEndPosPtr = endPos;
    al::setTrans(this, mStartPos);
    al::emitEffect(this, noTrail ? "BindWarpNoTrail" : getEffectName(), nullptr);
    al::setNerve(this, &NrvBindWarpEffectWait);
}
void BindWarpEffect::start(const al::HitSensor* sensor, const sead::Vector3f& endPos, bool noTrail) {
    makeActorAppeared();
    mPlayerSensor = sensor;
    rc::calcPlayerModelJointPos(&mStartPos, sensor, "JointRoot");
    mEndPos.set(endPos);
    mEndPosPtr = nullptr;
    al::setTrans(this, mStartPos);
    al::emitEffect(this, noTrail ? "BindWarpNoTrail" : getEffectName(), nullptr);
    al::setNerve(this, &NrvBindWarpEffectWait);
}
void BindWarpEffect::start(const al::HitSensor* sensor, const sead::Vector3f* endPos, bool noTrail) {
    makeActorAppeared();
    mPlayerSensor = sensor;
    rc::calcPlayerModelJointPos(&mStartPos, sensor, "JointRoot");
    mEndPos.set(sead::Vector3f::zero);
    mEndPosPtr = endPos;
    al::setTrans(this, mStartPos);
    al::emitEffect(this, noTrail ? "BindWarpNoTrail" : getEffectName(), nullptr);
    al::setNerve(this, &NrvBindWarpEffectWait);
}
void BindWarpEffect::cancel() {
    al::deleteEffectAll(this);
    al::setNerve(this, &NrvBindWarpEffectDone);
}
void BindWarpEffect::setEndPos(const sead::Vector3f& pos) { mEndPos.set(pos); }
void BindWarpEffect::calcEndPos(sead::Vector3f* pos) const {
    if (mEndPosPtr) pos->set(*mEndPosPtr);
    else pos->set(mEndPos);
}
bool BindWarpEffect::isMoving() const { return al::isNerve(this, &NrvBindWarpEffectMove); }
bool BindWarpEffect::isEnd() const { return al::isNerve(this, &NrvBindWarpEffectDone); }
void BindWarpEffect::exeWait() {
    if (al::isFirstStep(this)) al::startSe(this, mIsKoopaJr ? "PrepareKoopaJr" : "Prepare");
    if (mIsKoopaJr || al::isGreaterEqualStep(this, 20)) {
        al::setNerve(this, &NrvBindWarpEffectMove);
        al::startSe(this, mIsKoopaJr ? "MoveKoopaJr" : "Move");
    }
}
void BindWarpEffect::exeMove() {
    float rate = al::calcNerveRate(this, 16);
    sead::Vector3f endPos;
    calcEndPos(&endPos);
    al::lerpVec(al::getTransPtr(this), mStartPos, endPos, al::easeIn(rate));
    al::getTransPtr(this)->y += 0.0f;
    if (al::isGreaterEqualStep(this, 16)) al::setNerve(this, &NrvBindWarpEffectDone);
}
void BindWarpEffect::exeDone() { kill(); }
