#include "MapObj/BreakMapParts.hpp"
#include "Library/ActorUtil.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Thread/Functor.hpp"
#include "Library/Yaml/ByamlIter.hpp"
#include "Library/Yaml/ByamlUtil.hpp"
#include "Library/LiveActor/Util/ActorResourceUtil.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/ScoreUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/DrcUtil.hpp"

BreakMapParts::BreakMapParts(const char* name) : al::LiveActor(name) {}
BreakMapParts::~BreakMapParts() {}
void BreakMapParts::init(const al::ActorInitInfo& info) {
    const char* className;
    al::getClassName(&className, info);
    if (al::isEqualString(className, "BreakMapPartsSand")) mBreakType = 1;
    else if (al::isEqualString(className, "BreakMapPartsBomb")) mBreakType = 2;
    else if (al::isEqualString(className, "BreakMapPartsGiant")) mBreakType = 3;
    else { al::tryGetArg(&mBreakType, info, "BreakTypes"); mSilent = true; }
    al::initActorPoseTRSV(this);
    al::initMapPartsActor(this, info, nullptr, 0);
    mBreakModel = al::tryGetSubActor(this, "壊れモデル");
    mTraceModel = al::tryGetSubActor(this, "残留モデル");
    if (mTraceModel) {
        bool random = false;
        al::LiveActor* trace = mTraceModel;
        if (al::isExistModelResourceYaml(trace, "InitTraceModel", nullptr)) {
            al::ByamlIter iter(al::getModelResourceYaml(trace, "InitTraceModel", nullptr));
            random = al::tryGetByamlKeyBoolOrFalse(iter, "IsRandomRotate");
        }
        mRandomRotate = random;
    }
    al::listenStageSwitchOnKill(this, al::FunctorV0M(this, &BreakMapParts::startBreakBySwitch));
    makeActorAppeared();
}
void BreakMapParts::startBreakBySwitch() {
    if (!mSilent) al::startHitReactionBreak(this);
    kill();
}
void BreakMapParts::kill() {
    if (mBreakModel) mBreakModel->appear();
    if (mTraceModel) {
        if (mRandomRotate) al::rotateQuatYDirDegree(mTraceModel, al::getRandomDegree());
        mTraceModel->appear();
    }
    al::LiveActor::kill();
    al::tryOnSwitchDeadOn(this);
}
bool BreakMapParts::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor* receiver) {
    switch (mBreakType) {
    case 0: return receiveMsgMapParts(msg, sender, receiver);
    case 1: return receiveMsgSand(msg, sender, receiver);
    case 2: return receiveMsgBomb(msg, sender, receiver);
    case 3: return receiveMsgGiant(msg, sender, receiver);
    }
    return false;
}
bool BreakMapParts::receiveMsgSand(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor* receiver) {
    if (al::isMsgPlayerRollingAttack(msg) || al::isMsgPlayerObjHipDropAll(msg) ||
        al::isMsgPlayerInvincibleAttack(msg) || al::isMsgPlayerFireBallAttack(msg) ||
        al::isMsgPlayerBoomerangAttack(msg) ||
        (al::isMsgPlayerClimbAttack(msg) && !((al::getActorTrans(sender) - al::getActorTrans(receiver)).y > al::getSensorRadius(sender) + al::getSensorRadius(receiver))) ||
        al::isMsgPlayerClimbSlidingAttack(msg) ||
        (al::isMsgPlayerTailAttack(msg) && !((al::getActorTrans(sender) - al::getActorTrans(receiver)).y > al::getSensorRadius(sender) + al::getSensorRadius(receiver))) ||
        al::isMsgPlayerSpinAttack(msg) || al::isMsgPlayerGiantAttack(msg) ||
        al::isMsgBallAttack(msg) || al::isMsgBallTrample(msg) || al::isMsgPlayerKouraAttack(msg) ||
        al::isMsgKickKouraReflect(msg) || al::isMsgKillerAttack(msg) ||
        al::isMsgExplosionCollide(msg) || al::isMsgExplosion(msg) || rc::isMsgBullAttack(msg) ||
        al::isMsgPlayerBodyAttack(msg) || rc::isMsgRaidonAttack(msg) || al::isMsgLaserAttack(msg)) {
        startBreak(msg, sender, receiver);
        return true;
    }
    if (al::isMsgKickKouraAttackCollide(msg)) {
        if (!mSilent) {
            rc::addScore(this, sender, 0.0f, 0);
            rc::requestHitReactionToAttacker("キック甲羅ヒット", receiver, sender);
            al::startHitReactionBreak(this);
        }
        kill();
        return true;
    }
    if (al::isMsgTouchAssistTrig(msg)) {
        startBreakWithTouch(msg, sender);
        return true;
    }
    return false;
}
bool BreakMapParts::receiveMsgBomb(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor* receiver) {
    if (al::isMsgExplosion(msg) || al::isMsgExplosionCollide(msg)) {
        startBreak(msg, sender, receiver);
        return true;
    }
    return false;
}
bool BreakMapParts::receiveMsgGiant(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor* receiver) {
    if (al::isMsgPlayerGiantTouch(msg)) {
        startBreak(msg, sender, receiver);
        return true;
    }
    return false;
}
bool BreakMapParts::receiveMsgMapParts(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor* receiver) {
    if (rc::isMsgForBlockAll(msg, sender, receiver, 100.0f)) {
        if (mBreakModel) mBreakModel->appear();
        if (mTraceModel) mTraceModel->appear();
        al::LiveActor::kill();
        al::tryOnSwitchDeadOn(this);
        return true;
    }
    return false;
}
void BreakMapParts::startBreak(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor* receiver) {
    if (!mSilent) {
        rc::requestHitReactionToAttacker(msg, receiver, sender);
        rc::addScore(this, sender, 0.0f, 0);
        al::startHitReactionBreak(this);
    }
    kill();
}
void BreakMapParts::startBreakWithTouch(const al::SensorMsg*, const al::HitSensor* sender) {
    if (!mSilent) {
        al::HitSensor* player = DrcFunction::tryFindDrcPlayerSensor(this, sender);
        if (player) rc::addScore(this, player, 0.0f, 0);
        al::startHitReaction(this, "タッチ破壊");
    }
    kill();
}
