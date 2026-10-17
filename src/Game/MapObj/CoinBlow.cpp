#include "MapObj/CoinBlow.hpp"
#include "MapObj/CoinRotater.hpp"
#include "MapObj/ItemStateAssistRotate.hpp"
#include "Enemy/EnemyStateUtil.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Library/Collision/PartsConnector.hpp"
#include "Library/Collision/PartsConnectorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Movement/FlashingCtrl.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Project/Collision/CollisionParts.hpp"
#include "Project/Collision/CollisionPartsTriangle.hpp"
#include "Project/Collision/CollisionUtil.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/CoinUtil.hpp"
#include "Util/InkUtil.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
namespace {
NERVE_DECL(CoinBlow, Blow)
NERVE_DECL(CoinBlow, SpinDrc)
NERVE_DECL(CoinBlow, Wait)
NERVES_MAKE_STRUCT(CoinBlow, Blow, SpinDrc, Wait)
}
CoinBlow::CoinBlow(const char* name) : al::LiveActor(name) {
    mConnector = new al::CollisionPartsConnector;
}
void CoinBlow::init(const al::ActorInitInfo& info) {
    al::initActorWithArchiveName(this, info, "Coin", nullptr);
    initCollider(49.5f, 0.0f, 0);
    al::initNerve(this, &NrvCoinBlow.Blow, 1);
    mFlashing = new al::FlashingCtrl(this, true, false);
    al::addTransOffsetLocalDir(this, 70.0f, 1);
    al::setQuat(this, sead::Quatf::unit);
    al::setScale(this, sead::Vector3f::ones);
    mBaseQuat.set(al::getQuat(this));
    mAssistRotate = new ItemStateAssistRotate(this, CoinUtil::getCoinAssistRotateParam());
    al::initNerveState(this, mAssistRotate, &NrvCoinBlow.SpinDrc, "回転");
    makeActorDead();
}
void CoinBlow::initAfterPlacement() { al::updateMaterialCodeWater(this); }
void CoinBlow::appear() {
    rotate();
    al::LiveActor::appear();
    if (mLifeTime >= 1) {
        al::invalidateClipping(this);
        mFlashing->start(mLifeTime);
    } else mFlashing->end();
    if (!mUseCollider) al::offCollide(this);
    al::invalidateHitSensors(this);
    al::setNerve(this, &NrvCoinBlow.Blow);
}
void CoinBlow::rotate() {
    if (mConnector && mConnected && al::isNerve(this, &NrvCoinBlow.Wait)) {
        sead::Quatf quat;
        al::rotateQuatYDirDegree(&quat, mBaseQuat, rc::getCoinRotateY(this));
        al::connectPoseQT(this, mConnector, quat, al::getConnectBaseTrans(mConnector));
    } else al::rotateQuatYDirDegree(this, mBaseQuat, rc::getCoinRotateY(this));
}
void CoinBlow::control() {
    if (al::isNerve(this, &NrvCoinBlow.Wait) && CoinUtil::tryStartSpinCoinIfMicInputOn(this, &NrvCoinBlow.SpinDrc)) {
        mSpinFromWait = true;
        return;
    }
    if (mLifeTime >= 1) {
        mFlashing->movement();
        if (mFlashing->isEnded()) kill();
    }
    if (mUseCollider) {
        if (EnemyStateUtil::tryKillByAreaOrMaterialCodeWithHitReaction(this)) return;
        if (GameDataFunction::isSingleMode(GameDataHolderAccessor(this)) &&
            (InkUtil::isInInkLimitSphere(this) || rc::isInWaterAreaNoSink(this))) {
            al::startHitReactionDeath(this);
            kill();
        }
    }
}
bool CoinBlow::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor*) {
    if (rc::tryDisappearItemByStartGoalDemoPole(this, msg)) return true;
    if (rc::isMsgStartGoalDemoHouse(msg)) { makeActorDead(); return true; }
    if (al::isLessEqualStep(this, mHideModelStep) && al::isNerve(this, &NrvCoinBlow.Blow)) return false;
    if (rc::isMsgCoinGet(msg)) {
        rc::acquirerItemCoin(this, sender);
        al::startHitReactionGet(this);
        kill();
        return true;
    }
    if (al::isMsgDisasterSpikeAttack(msg)) { al::startHitReactionDeath(this); kill(); }
    return false;
}
bool CoinBlow::receiveMsgScreenPoint(const al::SensorMsg* msg, al::ScreenPointer* pointer, al::ScreenPointTarget*) {
    if (al::isLessEqualStep(this, mHideModelStep) && al::isNerve(this, &NrvCoinBlow.Blow)) return false;
    if (rc::tryAcquirerCoinIfTouchAssistTrigger(msg, this, pointer)) { kill(); return true; }
    if (al::isNerve(this, &NrvCoinBlow.Blow)) return false;
    if (al::isMsgTouchAssistNoPat(msg)) {
        mSpinFromWait = al::isNerve(this, &NrvCoinBlow.SpinDrc) ? mSpinFromWait : al::isNerve(this, &NrvCoinBlow.Wait);
        if (!al::isNerve(this, &NrvCoinBlow.SpinDrc)) al::setNerve(this, &NrvCoinBlow.SpinDrc);
        return true;
    }
    return false;
}
void CoinBlow::appearWithHitReaction() { al::startHitReactionAppear(this); appear(); }
void CoinBlow::setOffCollide(int step) { mOffCollideStep = step; al::offCollide(this); }
void CoinBlow::setOffSensor(int step) { mOffSensorStep = step; al::invalidateHitSensors(this); }
void CoinBlow::setHideModel(int step) { mHideModelStep = step; al::hideModelIfShow(this); }
void CoinBlow::setWaiting() { al::setNerve(this, &NrvCoinBlow.Wait); }
void CoinBlow::exeBlow() {
    bool water = rc::isInWaterArea(this);
    float gravity = water ? 0.6f : 1.5f;
    float horizontal = 0.99f;
    float vertical = water ? 0.9f : horizontal;
    float minY = water ? -10.0f : -25.0f;
    al::addVelocityToGravity(this, gravity);
    al::scaleVelocityHV(this, horizontal, vertical);
    if (al::getVelocity(this).y < minY) al::setVelocityY(this, minY);
    if (mUseCollider && al::isStep(this, mOffCollideStep)) al::onCollide(this);
    if (al::isStep(this, mHideModelStep)) al::showModelIfHide(this);
    if (al::isStep(this, mOffSensorStep)) al::validateHitSensors(this);
    rotate();
    if (mUseCollider) setBound(water); else setBoundOnArea();
}
void CoinBlow::setBound(bool water) {
    sead::Vector3f velocity(al::getVelocity(this));
    sead::Vector3f normal(sead::Vector3f::zero);
    float restitution = water ? 0.75f : 0.8f;
    float speed = al::calcSpeed(this);
    if (al::isCollidedGround(this) && al::getVelocity(this).y <= 0.0f) {
        if (sead::Mathf::abs(speed) <= (water ? 2.5f : 7.5f) && al::isNerve(this, &NrvCoinBlow.Blow)) {
            if (rc::isInAreaObj(this, rc::AreaObjType::CoinBlowConnectGroundArea)) {
                al::Triangle triangle;
                sead::Vector3f start = al::getTrans(this) + sead::Vector3f(0.0f, 100.0f, 0.0f);
                alCollisionUtil::getFirstPolyOnArrow(this, nullptr, &triangle, start, sead::Vector3f(0.0f, -500.0f, 0.0f), nullptr, nullptr);
                if (triangle.isValid()) {
                    mConnected = true;
                    const auto* parts = triangle.mCollisionParts;
                    mConnector->init(&parts->getBaseMtx(), parts->getBaseInvMtx(), parts);
                    mConnector->setBaseQuatTrans(mBaseQuat, al::getTrans(this));
                    mAssistRotate->setConnector(mConnector, mBaseQuat);
                }
            }
            al::setNerve(this, &NrvCoinBlow.Wait);
            al::setVelocityZero(this);
            return;
        }
        velocity.set(0.0f, -restitution * al::getVelocity(this).y, 0.0f);
    } else if (al::isCollidedWall(this)) {
        normal.set(al::getCollidedWallNormal(this));
        if (!al::calcReflectionVector(&velocity, normal, restitution, 0.0f)) velocity = normal;
    } else {
        if (al::isCollidedCeiling(this) && al::getVelocity(this).y > 0.0f) al::getVelocityPtr(this)->y = 0.0f;
        return;
    }
    al::setVelocity(this, velocity);
    al::scaleVelocity(this, water ? 0.9f : 0.75f);
    if (speed > 14.0f) {
        al::stopSeByName(this, "PgBound");
        al::startSeWithParam(this, "PgBound", speed, nullptr);
    }
}
void CoinBlow::setBoundOnArea() {
    if (!rc::isInAreaObj(this, rc::AreaObjType::CoinFallBoundArea) || al::getVelocity(this).y > 0.0f) return;
    sead::Vector3f velocity(al::getVelocity(this));
    float speed = al::calcSpeed(this);
    if (sead::Mathf::abs(speed) <= 7.5f && al::isNerve(this, &NrvCoinBlow.Blow)) {
        al::setNerve(this, &NrvCoinBlow.Wait);
        al::setVelocityZero(this);
        return;
    }
    velocity.set(0.0f, al::getVelocity(this).y * -0.8f, 0.0f);
    al::setVelocity(this, velocity);
    al::scaleVelocity(this, 0.75f);
    if (speed > 14.0f) {
        al::stopSeByName(this, "PgBound");
        al::startSeWithParam(this, "PgBound", speed, nullptr);
    }
}
void CoinBlow::exeWait() {
    if (al::isFirstStep(this)) {
        if (mUseCollider && al::isEqualString(al::getCollidedFloorMaterialCodeName(this), "Ashore"))
            al::emitEffect(this, "RippleAshore", nullptr);
        al::validateHitSensors(this);
    }
    rotate();
    if (mUseCollider && mConnected) {
        if (al::isCollidedWall(this)) mConnected = false;
        if (!mConnector->isConnecting()) {
            mConnected = false;
            al::setNerve(this, &NrvCoinBlow.Blow);
        }
    }
}
void CoinBlow::exeSpinDrc() {
    bool water = rc::isInWaterArea(this);
    if (al::isFirstStep(this)) al::startSe(this, "PgTouched", nullptr);
    if (!mSpinFromWait) {
        float gravity = water ? 0.6f : 1.5f;
        float scale = water ? 0.9f : 0.99f;
        al::addVelocityToGravity(this, gravity);
        al::scaleVelocity(this, scale);
        if (mUseCollider) setBound(water); else setBoundOnArea();
    }
    if (al::updateNerveState(this)) {
        if (mSpinFromWait) al::setNerve(this, &NrvCoinBlow.Wait);
        else al::setNerve(this, &NrvCoinBlow.Blow);
    }
}
