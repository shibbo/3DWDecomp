#include "MapObj/ItemStatePlayerHold.hpp"
#include "MapObj/ItemStatePlayerHoldParam.hpp"
#include "MapObj/HoldColliderControl.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Math/MatrixUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Util/PlayerUtil.hpp"
namespace {
const ItemStatePlayerHoldParam sDefaultParam;
NERVE_DECL(ItemStatePlayerHold, Hold)
NERVES_MAKE_NOSTRUCT(ItemStatePlayerHold, Hold)
}
ItemStatePlayerHoldParam::ItemStatePlayerHoldParam() : mHoldRotate(0.0f, 0.0f, 270.0f) {
    for (auto& offset : mOffsets) offset.set(0.0f, 0.0f, 0.0f);
}
ItemStatePlayerHoldParam::ItemStatePlayerHoldParam(const sead::Vector3f& pos, const sead::Vector3f& rotate)
    : mHoldRotate(rotate) {
    for (auto& offset : mOffsets) offset.set(pos);
}
ItemStatePlayerHoldParam::ItemStatePlayerHoldParam(
    const sead::Vector3f& p0, const sead::Vector3f& p1, const sead::Vector3f& p2,
    const sead::Vector3f& p3, const sead::Vector3f& p4, const sead::Vector3f& p5,
    const sead::Vector3f& p6, const sead::Vector3f& p7, const sead::Vector3f& p8,
    const sead::Vector3f& p9, const sead::Vector3f& rotate) : mHoldRotate(rotate) {
    mOffsets[0].set(p0);
    mOffsets[1].set(p1);
    mOffsets[2].set(p2);
    mOffsets[3].set(p3);
    mOffsets[4].set(p4);
    mOffsets[5].set(p5);
    mOffsets[6].set(p6);
    mOffsets[7].set(p7);
    mOffsets[8].set(p8);
    mOffsets[9].set(p9);
}
const sead::Vector3f& ItemStatePlayerHoldParam::getPlayerHoldPos(const al::HitSensor* sensor) const {
    int type = rc::getPlayerCharaType(sensor);
    if (rc::isPlayerMini(sensor)) {
        if (type == 0) return mOffsets[5];
        switch (type) {
        case 1: return mOffsets[6];
        case 2: return mOffsets[7];
        case 3: return mOffsets[8];
        case 4: return mOffsets[9];
        }
    } else {
        if (type == 0) return mOffsets[0];
        switch (type) {
        case 1: return mOffsets[1];
        case 2: return mOffsets[2];
        case 3: return mOffsets[3];
        case 4: return mOffsets[4];
        }
    }
    return mOffsets[0];
}
ItemStatePlayerHold::ItemStatePlayerHold(al::LiveActor* actor, const ItemStatePlayerHoldParam* param,
    bool flag21, bool flag22)
    : al::ActorStateBase("プレイヤーに持たれるステート", actor), mInvalidateSensors(true),
      _21(flag21), _22(flag22), _23(false), mHolderSensor(nullptr), mParam(param),
      mColliderControl(nullptr), _4c(0.0f, 0.0f, 0.0f) {
    if (!mParam) mParam = &sDefaultParam;
}
void ItemStatePlayerHold::init() { initNerve(&NrvItemStatePlayerHoldHold, 0); }
void ItemStatePlayerHold::appear() {
    if (mInvalidateSensors) al::invalidateHitSensors(mHostActor);
    al::invalidateClipping(mHostActor);
    if (mColliderControl) mColliderControl->start();
    else al::offCollide(mHostActor);
    al::ActorStateBase::appear();
    al::setNerve(this, &NrvItemStatePlayerHoldHold);
}
void ItemStatePlayerHold::kill() {
    mHolderSensor = nullptr;
    if (mInvalidateSensors) al::validateHitSensors(mHostActor);
    if (!_23) al::validateClipping(mHostActor);
    if (!mColliderControl) al::onCollide(mHostActor);
    al::ActorStateBase::kill();
}
bool ItemStatePlayerHold::receiveMsg(const al::SensorMsg* msg, al::HitSensor* other, al::HitSensor*) {
    if (al::isMsgPlayerRelease(msg) || al::isMsgPlayerReleaseDamage(msg) || al::isMsgPlayerReleaseDead(msg) || al::isMsgHoldCancel(msg) || al::isMsgWarpStart(msg)) {
        if (al::isMsgWarpStart(msg)) rc::requestPlayerRelease(other);
        rc::trySetPlayerForwardToInput(other);
        al::makeQuatFrontUp(al::getQuatPtr(mHostActor), rc::getPlayerFront(other), sead::Vector3f::ey);
        kill();
        return true;
    }
    return false;
}
bool ItemStatePlayerHold::tryStartCarryUp(const al::SensorMsg* msg, al::HitSensor* sensor, bool invalidate) {
    if (al::isMsgPlayerCarryUp(msg)) {
        mHolderSensor = sensor;
        mInvalidateSensors = invalidate;
        return true;
    }
    return false;
}
bool ItemStatePlayerHold::tryStartCarryFront(const al::SensorMsg* msg, al::HitSensor* sensor, bool invalidate) {
    if (al::isMsgPlayerCarryFront(msg) || al::isMsgPlayerCarryFrontNeko(msg)) {
        mHolderSensor = sensor;
        mInvalidateSensors = invalidate;
        return true;
    }
    return false;
}
void ItemStatePlayerHold::initColliderControl() {
    mColliderControl = new HoldColliderControl;
    mColliderControl->init(mHostActor);
}
void ItemStatePlayerHold::updateCollider(al::HitSensor* sensor) { mColliderControl->update(mHolderSensor, sensor); }
void ItemStatePlayerHold::exeHold() {
    _4c = mParam->getPlayerHoldPos(mHolderSensor);
    _40.set(al::getTrans(mHostActor));
    sead::Matrix34f holdMtx;
    rc::calcPlayerHoldMtx(&holdMtx, mHolderSensor);
    sead::Vector3f pos;
    al::calcTransLocalOffsetByMtx(&pos, holdMtx, _4c);
    if (_21) {
        holdMtx(0, 0) = 1.0f;
        holdMtx(0, 1) = 0.0f;
        holdMtx(0, 2) = 0.0f;
        holdMtx(1, 0) = 0.0f;
        holdMtx(1, 1) = 1.0f;
        holdMtx(1, 2) = 0.0f;
        holdMtx(2, 0) = 0.0f;
        holdMtx(2, 1) = 0.0f;
        holdMtx(2, 2) = 1.0f;
    } else if (_22) {
        sead::Vector3f front;
        holdMtx.getBase(front, 2);
        sead::Vector3f side = sead::Vector3f::ey.cross(front);
        al::normalizeOrDirZ(&side);
        front = side.cross(sead::Vector3f::ey);
        if (al::normalizeOrZero(&front)) front = sead::Vector3f::ex;
        holdMtx.setBase(0, side);
        holdMtx.setBase(1, sead::Vector3f::ey);
        holdMtx.setBase(2, front);
    }
    sead::Matrix34f rotation;
    rotation.makeR(mParam->getHoldRotate() * sead::Mathf::deg2rad(1.0f));
    holdMtx.setMul(holdMtx, rotation);
    holdMtx.setTranslation(pos);
    al::updatePoseMtx(mHostActor, &holdMtx);
}
