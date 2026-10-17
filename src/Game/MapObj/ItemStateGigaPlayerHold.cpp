#include "MapObj/ItemStateGigaPlayerHold.hpp"
#include "MapObj/ItemStatePlayerHoldParam.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Math/MatrixUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include <math/seadQuat.h>

ItemStateGigaPlayerHold::ItemStateGigaPlayerHold(al::LiveActor* actor,
        const ItemStatePlayerHoldParam* param, bool invalidateSensors, bool flag)
    : ItemStatePlayerHold(actor, param, invalidateSensors, flag) {}

void ItemStateGigaPlayerHold::exeHold() {
    _4c = mParam->getPlayerHoldPos(mHolderSensor);
    _4c *= al::getScaleX(al::getSensorHost(mHolderSensor));
    _40.set(al::getTrans(mHostActor));
    sead::Matrix34f holdMtx;
    calcPlayerHoldMtx(&holdMtx);
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

void ItemStateGigaPlayerHold::calcPlayerHoldMtx(sead::Matrix34f* mtx) {
    sead::Vector3f pos;
    rc::calcPlayerHoldPos(&pos, mHolderSensor);
    sead::Matrix34f jointMtx = *rc::getPlayerModelJointMtxPtr(mHolderSensor, "Spine2");
    float invScale = 1.0f / al::getScaleX(al::getSensorHost(mHolderSensor));
    jointMtx.scaleBases(invScale, invScale, invScale);
    sead::Quatf quat;
    quat.setAxisRadian(sead::Vector3f::ex, sead::Mathf::deg2rad(-90.0f));
    sead::Matrix34f rotation;
    rotation.fromQuat(quat);
    mtx->setMul(jointMtx, rotation);
    mtx->setTranslation(pos);
}
