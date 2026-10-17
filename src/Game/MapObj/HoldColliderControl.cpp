#include "MapObj/HoldColliderControl.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/LiveActor/Util/ActorFlagUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Collision/Collider.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Project/Base/StringUtil.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include <math/seadMathCalcCommon.h>

HoldColliderControl::HoldColliderControl() {}

void HoldColliderControl::init(al::LiveActor* pActor) {
    mActor = pActor;
}

void HoldColliderControl::start() {
    mStep = 0;
}

void HoldColliderControl::update(al::HitSensor* pPlayer, al::HitSensor* pHeld) {
    al::Collider* collider = mActor->getCollider();
    if (al::isNoCollide(mActor)) {
        collider->onInvalidate();
        return;
    }
    float radius = collider->getRadius();
    collider->setRadius(sead::Mathf::min(40.0f + 10.0f * sead::Mathi::max(mStep - 3, 0), radius));
    if (mStep != 0)
        collider->onInvalidate();
    sead::Vector3f push = collider->collide(sead::Vector3f::zero);
    collider->setRadius(radius);
    if (al::isOnGround(mActor, 0, 0.0f)) {
        if (rc::isPlayerOnGround(pPlayer)) {
            push -= sead::Vector3f::ey * push.dot(sead::Vector3f::ey);
        } else {
            if (GameDataFunction::isSingleMode(GameDataHolderAccessor(mActor)) &&
                al::isEqualString(al::getCollidedFloorMaterialCodeName(mActor), "Cloud")) {
                collider->onInvalidate();
                return;
            }
            sead::Vector3f groundPos = al::getCollidedGroundPos(mActor);
            sead::Vector3f direction = al::getActorTrans(pPlayer) - groundPos;
            al::verticalizeVec(&direction, sead::Vector3f::ey, direction);
            if (al::normalizeOrZero(&direction))
                direction = -rc::getPlayerFront(pPlayer);
            push += collider->getRadius() * direction * 0.5f;
        }
    }
    rc::sendMsgCameraPush(pPlayer, pHeld, push);
    ++mStep;
}
