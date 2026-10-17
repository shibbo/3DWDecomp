#include "MapObj/BallGimmick.hpp"
#include "Enemy/ActorStateSupportFreeze.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Math/MatrixUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Project/Collision/CollisionParts.hpp"
#include "Project/Collision/CollisionUtil.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
namespace {
NERVE_DECL(BallGimmick, Rolling)
NERVE_DECL(BallGimmick, SupportFreeze)
NERVE_DECL(BallGimmick, Fall)
NERVE_DECL(BallGimmick, PreFall)
const struct {
    BallGimmickNrvPreFall PreFall;
    BallGimmickNrvRolling Rolling;
    BallGimmickNrvSupportFreeze SupportFreeze;
    BallGimmickNrvFall Fall;
} NrvBallGimmick{};
ActorStateSupportFreezeParam sFreezeParam(true, 15, false, false, 120, sead::Vector3f(0.0f, 150.0f, 0.0f));
}
BallGimmick::BallGimmick(const char* name) : al::LiveActor(name) {}
BallGimmick::~BallGimmick() = default;
void BallGimmick::init(const al::ActorInitInfo& info) {
    al::initActor(this, info);
    mFreezeState = new ActorStateSupportFreeze(this, &sFreezeParam);
    al::initNerve(this, &NrvBallGimmick.Rolling, 1);
    al::initNerveState(this, mFreezeState, &NrvBallGimmick.SupportFreeze, "[state]フリーズ");
    mHomeTrans = al::getTrans(this);
    mPreviousTrans = al::getTrans(this);
    al::startAction(this, "Wait");
    al::setEffectFollowMtxPtr(this, "HitCollision", &mEffectMtx);
    al::trySyncStageSwitchAppear(this);
}
void BallGimmick::kill() {
    al::stopSeByName(this, "Rolling");
    mCollideSensor = nullptr;
    al::setTrans(this, mHomeTrans);
    al::LiveActor::kill();
}
void BallGimmick::control() {
    if (rc::isInDeathArea(this)) { al::startHitReactionDeath(this); kill(); }
    if (mCollideSensor) al::sendMsgBallAttackCollide(mCollideSensor, al::getHitSensor(this, "Body"));
    mPreviousTrans = al::getTrans(this);
}
void BallGimmick::attackSensor(al::HitSensor* self, al::HitSensor* other) {
    if (al::isSensorEnemyBody(other)) al::sendMsgPushAndKillVelocityToTarget(this, self, other);
}
bool BallGimmick::receiveMsg(const al::SensorMsg* msg, al::HitSensor* other, al::HitSensor* self) {
    if (al::isMsgPush(msg)) {
        sead::Vector3f dir = al::getSensorPos(self) - al::getSensorPos(other);
        float distance = al::getSensorRadius(this) * 2.0f - dir.length();
        al::normalizeOrDirZ(&dir);
        float push = distance - al::getVelocity(this).dot(dir);
        if (push > 0.0f) { auto* velocity = al::getVelocityPtr(this); *velocity += push * dir; }
    }
    return false;
}
bool BallGimmick::receiveMsgScreenPoint(const al::SensorMsg* msg, al::ScreenPointer* pointer, al::ScreenPointTarget* target) {
    if (mFreezeState->receiveMsgScreenPoint(msg, pointer, target)) {
        if (!al::isNerve(this, &NrvBallGimmick.SupportFreeze)) al::setNerve(this, &NrvBallGimmick.SupportFreeze);
        return true;
    }
    return false;
}
void BallGimmick::exePreFall() {
    al::addVelocityToGravity(this, 1.3f);
    al::scaleVelocity(this, 0.99f);
    if (al::isCollidedGround(this)) { boundGround(); al::setNerve(this, &NrvBallGimmick.Rolling); }
    else {
        if (al::isCollidedWall(this)) boundWall();
        if (al::isGreaterEqualStep(this, 3)) al::setNerve(this, &NrvBallGimmick.Fall);
    }
    rotateBall();
}
void BallGimmick::boundGround() {
    sead::Vector3f normal = al::getOnGroundNormal(this, 0);
    sead::Vector3f velocity = al::getVelocity(this);
    sead::Vector3f parallel;
    al::parallelizeVec(&parallel, normal, velocity);
    auto* parts = al::tryGetCollidedGroundCollisionParts(this);
    float restitution = 1.4f;
    if (parts) {
        if (rc::sendMsgBoundTrampoline(al::tryGetCollidedGroundSensor(this), al::getHitSensor(this, "Body"))) {
            restitution = 2.5f;
            if (sead::Mathf::abs(velocity.y) > 4.2f) {
                al::startSe(this, "BoundTrampoline", nullptr);
                al::startSeWithParam(this, "BoundGround", sead::Mathf::abs(velocity.y), nullptr);
            }
        } else if (al::isEqualString(parts->getConnectedHost()->getName(), "協力踏みスイッチ")) {
            mCollideSensor = parts->getSensor();
            al::startSeWithParam(this, "BoundGround", sead::Mathf::abs(velocity.y), nullptr);
        } else if (sead::Mathf::abs(velocity.y) > 4.2f) al::startSeWithParam(this, "BoundGround", sead::Mathf::abs(velocity.y), nullptr);
    }
    velocity -= restitution * parallel;
    if (velocity.y > 0.0f && velocity.y < 4.0f) {
        velocity.y *= 0.4f;
        if (velocity.y < 1.0f) velocity.y = 0.0f;
    }
    al::setVelocity(this, velocity);
}
void BallGimmick::rotateBall() {
    sead::Vector3f moment = sead::Vector3f::zero;
    sead::Vector3f delta = al::getTrans(this) - mPreviousTrans;
    sead::Vector3f up = -al::getGravity(this);
    al::calcMomentRollBall(&moment, delta, up, al::getColliderRadius(this));
    auto* quat = al::getQuatPtr(this);
    al::rotateQuatMoment(quat, *quat, moment);
}
void BallGimmick::boundWall() {
    sead::Vector3f normal = al::getCollidedWallNormal(this);
    sead::Vector3f velocity = al::getVelocity(this);
    sead::Vector3f parallel;
    al::parallelizeVec(&parallel, normal, velocity);
    velocity -= parallel * 1.4f;
    al::setVelocity(this, velocity);
}
void BallGimmick::exeFall() {
    if (al::isFirstStep(this)) al::stopSeByName(this, "Rolling");
    al::addVelocityToGravity(this, 1.3f);
    al::scaleVelocity(this, 0.99f);
    if (al::isCollidedGround(this)) {
        al::sendMsgBallAttackCollide(al::tryGetCollidedGroundSensor(this), al::getHitSensor(this, "Body"));
        boundGround();
        sead::Vector3f normal = al::getOnGroundNormal(this, 0);
        sead::Vector3f pos = al::getCollidedGroundPos(this);
        al::makeMtxUpNoSupportPos(&mEffectMtx, normal, pos);
        al::emitEffect(this, "HitCollision", nullptr);
        al::setNerve(this, &NrvBallGimmick.Rolling);
        return;
    }
    if (al::isCollidedWall(this)) boundWall();
    rotateBall();
}
void BallGimmick::exeRolling() {
    if (al::isFirstStep(this) && al::isVelocityFast(this, 5.0f)) al::startSe(this, "Rolling", nullptr);
    if (al::isCollidedGround(this)) {
        sead::Vector3f normal = al::getOnGroundNormal(this, 0);
        sead::Vector3f tangent;
        tangent.setCross(sead::Vector3f::ey, normal);
        sead::Vector3f slope;
        slope.setCross(tangent, normal);
        al::addVelocity(this, slope * 1.3f);
        al::scaleVelocity(this, 0.99f);
        auto* sensor = al::tryGetCollidedGroundSensor(this);
        al::sendMsgBallAttackCollide(sensor, al::getHitSensor(this, "Body"));
        al::sendMsgEnemyFloorTouch(sensor, al::getHitSensor(this, "Body"));
    } else al::setNerve(this, &NrvBallGimmick.PreFall);
    if (al::isCollidedWall(this)) boundWall();
    rotateBall();
}
void BallGimmick::exeSupportFreeze() {
    if (al::isFirstStep(this)) mFreezeTrans = al::getTrans(this);
    if (al::updateNerveState(this) || (mFreezeTrans - al::getTrans(this)).length() > 30.0f) al::setNerve(this, &NrvBallGimmick.Rolling);
}
