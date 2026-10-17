#include "MapObj/DestructableMapParts.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Collision/CollisionCheckInfo.hpp"
#include "Project/Collision/CollisionParts.hpp"
#include "Util/ProjectMsgUtil.hpp"
namespace {
NERVE_ACTION_IMPL(DestructableMapParts, Wait);
NERVE_ACTION_IMPL(DestructableMapParts, Hit);
NERVE_ACTION_IMPL(DestructableMapParts, Destruct);
NERVE_ACTION_IMPL(DestructableMapParts, Construct);
NERVE_ACTION_IMPL(DestructableMapParts, End);
NERVE_ACTIONS_MAKE_STRUCT(DestructableMapParts, Wait, Hit, Destruct, Construct, End)
bool checkStrike(al::CollisionParts* parts, al::ArrowHitResultBuffer* buffer, al::HitSensor* sender, al::HitSensor* receiver) {
    buffer->clear();
    sead::Vector3f pos = al::getSensorPos(sender);
    sead::Vector3f dir = al::getSensorPos(receiver) - pos;
    float radius = al::getSensorRadius(sender);
    al::normalizeOrDirZ(&dir);
    sead::Vector3f offset = radius * dir;
    return parts->checkStrikeArrow(buffer, pos - offset, offset * 2.0f, nullptr) > 0;
}
}
DestructableMapParts::DestructableMapParts(const char* name) : al::LiveActor(name) {}
DestructableMapParts::~DestructableMapParts() {}
void DestructableMapParts::init(const al::ActorInitInfo& info) {
    al::initNerveAction(this, "Wait", &NrvDestructableMapParts.collector, 0);
    al::initMapPartsActor(this, info, nullptr, 0);
    if (!getPoseKeeper()) al::initActorPoseTQSV(this);
    if (!al::trySyncStageSwitchAppear(this)) al::trySyncStageSwitchKill(this);
    mParts = getCollisionParts();
    mHitBuffer = new al::ArrowHitResultBuffer;
    mHitBuffer->allocBuffer(1, nullptr);
}
void DestructableMapParts::initAfterPlacement() {}
void DestructableMapParts::appear() {
    al::LiveActor::appear();
    al::tryStartAction(this, "Appear");
    al::startNerveAction(this, "Wait");
    mDestructStarted = false;
    mHitCount = 0;
}
void DestructableMapParts::kill() { al::LiveActor::kill(); }
bool DestructableMapParts::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor* receiver) {
    if (al::isMsgPlayerDisregard(msg)) return true;
    if (!mDestructStarted && (al::isMsgLaserAttack(msg) || al::isMsgSink(msg) || al::isMsgExplosion(msg))) {
        mDestructStarted = true;
        al::startNerveAction(this, "Destruct");
        return true;
    }
    if (!al::isNerve(this, NrvDestructableMapParts.Wait.data())) return false;
    if (rc::isMsgPlayerGigaStep(msg)) {
        sead::Vector3f dir;
        al::calcDirBetweenSensors(&dir, sender, receiver);
        startDestruct(al::getSensorPos(sender) + al::getSensorRadius(sender) * dir, true);
        ++mHitCount;
        return true;
    }
    if (al::isMsgPlayerClimbAttack(msg) || al::isMsgPlayerBodyAttack(msg)) {
        if (checkStrike(mParts, mHitBuffer, sender, receiver)) {
            ++mHitCount;
            startDestruct(mHitBuffer->front()->mPos, true);
            return true;
        }
    } else if (al::isMsgGigaEnemyAttack(msg) && al::isNerve(this, NrvDestructableMapParts.Wait.data())) {
        if (checkStrike(mParts, mHitBuffer, sender, receiver)) startDestruct(mHitBuffer->front()->mPos, false);
        else startDestruct(al::getTrans(this), false);
        return true;
    }
    return false;
}
void DestructableMapParts::startDestruct(const sead::Vector3f& pos, bool stop) {
    al::startHitReactionHitEffect(this, "HrHit", pos);
    if (stop) al::stopScene(this, 6, 1, true, false);
    al::startNerveAction(this, "Destruct");
}
void DestructableMapParts::control() {}
void DestructableMapParts::exeWait() {}
void DestructableMapParts::exeHit() {
    if (al::isFirstStep(this) && mHitCount >= 1) al::startNerveAction(this, "Destruct");
    if (al::isGreaterEqualStep(this, 45)) al::startNerveAction(this, "Wait");
}
void DestructableMapParts::exeDestruct() {
    if (al::isFirstStep(this)) {
        al::tryStartAction(this, "Destruct");
        al::invalidateCollisionParts(this);
    }
    if (al::isGreaterEqualStep(this, 8)) al::startNerveAction(this, "End");
}
void DestructableMapParts::exeConstruct() { if (al::isFirstStep(this)) al::validateCollisionParts(this); }
void DestructableMapParts::exeEnd() { kill(); }
