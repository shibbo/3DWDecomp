#include "MapObj/ItemStateCheckCollision.hpp"
#include "Enemy/EnemyStateUtil.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/Collision/Collider.hpp"
#include "Library/Collision/PartsConnector.hpp"
#include "Library/Collision/PartsConnectorUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Project/Collision/CollisionParts.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"

namespace {
inline bool isMissingCollision(const al::CollisionParts* parts) { return !parts; }
NERVE_DECL(ItemStateCheckCollision, Land);
NERVE_DECL(ItemStateCheckCollision, Fall);
NERVE_DECL(ItemStateCheckCollision, WaitConnect);
NERVES_MAKE_NOSTRUCT(ItemStateCheckCollision, Land, Fall, WaitConnect)
}

ItemStateCheckCollision::ItemStateCheckCollision(al::LiveActor* pActor)
    : al::ActorStateBase("アイテムコリジョンチェックステート", pActor),
      mConnector(new al::CollisionPartsConnector) {}

void ItemStateCheckCollision::init() {
    initNerve(&NrvItemStateCheckCollisionLand, 0);
}

void ItemStateCheckCollision::appear() {
    al::ActorStateBase::appear();
    connectToCollisionParts();
    al::setNerve(this, &NrvItemStateCheckCollisionLand);
    if (al::isInvalidClipping(mHostActor))
        mPreserveInvalidClipping = true;
}

void ItemStateCheckCollision::kill() {
    al::ActorStateBase::kill();
}

bool ItemStateCheckCollision::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor*, al::HitSensor*) {
    if (rc::isMsgJumpPanelAction(pMsg) && al::getVelocity(mHostActor).y <= 0.0f) {
        al::setNerve(this, &NrvItemStateCheckCollisionFall);
        mCollisionParts = nullptr;
        al::getVelocityPtr(mHostActor)->y = 40.0f;
    }
    return false;
}

void ItemStateCheckCollision::connectToCollisionParts() {
    al::LiveActor* actor = mHostActor;
    if (al::isCollidedGround(actor)) {
        al::Triangle triangle = al::getActorCollider(actor)->mFloor.mTriangle;
        if (triangle.isValid()) {
            mCollisionParts = triangle.mCollisionParts;
            mConnector->init(&mCollisionParts->mBaseMtx, mCollisionParts->mBaseInvMtx,
                             mCollisionParts);
            mConnector->setBaseQuatTrans(al::getQuat(actor), al::getTrans(actor));
            mBaseTrans.set(al::getTrans(actor));
            al::connectPoseTrans(actor, mConnector, mBaseTrans);
            return;
        }
    }
    mCollisionParts = nullptr;
}

void ItemStateCheckCollision::exeLand() {
    al::LiveActor* actor = mHostActor;
    if (al::isFirstStep(this)) {
        al::startAction(actor, "Land");
        sead::Vector3f* velocity = al::getVelocityPtr(actor);
        al::parallelizeVec(velocity, sead::Vector3f::ey, *velocity);
    }
    if (EnemyStateUtil::tryKillByAreaOrMaterialCodeWithHitReaction(actor))
        return;
    if (al::isCollidedGround(actor)) {
        if (mCollisionParts)
            al::connectPoseQT(actor, mConnector);
        if (!al::isActionEnd(actor))
            return;
        al::setNerve(this, &NrvItemStateCheckCollisionWaitConnect);
    } else {
        al::setNerve(this, &NrvItemStateCheckCollisionFall);
    }
    al::startAction(actor, "Wait");
}

void ItemStateCheckCollision::exeWaitConnect() {
    al::LiveActor* actor = mHostActor;
    if (al::isFirstStep(this)) {
        al::setVelocityZero(actor);
        mPreviousTrans.set(al::getTrans(actor));
    }
    if (EnemyStateUtil::tryKillByAreaOrMaterialCodeWithHitReaction(actor))
        return;
    al::Triangle triangle;
    const al::CollisionParts* parts = nullptr;
    if (al::isCollidedGround(actor)) {
        triangle = al::getActorCollider(actor)->mFloor.mTriangle;
        parts = triangle.mCollisionParts;
    }
    if (mCollisionParts != parts && parts && al::isGreaterEqualStep(this, 5)) {
        mCollisionParts = parts;
        mConnector->init(&parts->mBaseMtx, parts->mBaseInvMtx, parts);
        mConnector->setBaseQuatTrans(al::getQuat(actor), al::getTrans(actor));
        mBaseTrans.set(al::getTrans(actor));
        al::connectPoseTrans(actor, mConnector, mBaseTrans);
        al::setNerve(this, &NrvItemStateCheckCollisionWaitConnect);
        return;
    }
    if (!mCollisionParts || (isMissingCollision(parts) && al::isGreaterEqualStep(this, 5))) {
        al::setNerve(this, &NrvItemStateCheckCollisionFall);
        al::setVelocityZero(actor);
        return;
    }
    if (al::isCollidedWall(actor)) {
        sead::Vector3f normal = al::getCollidedWallNormal(actor);
        sead::Vector3f direction = al::getTrans(actor) - mPreviousTrans;
        al::normalizeOrZero(&direction);
        if (!al::isNearZero(direction, 0.001f) && normal.dot(direction) < 0.0f)
            return;
        mPreviousTrans.set(al::getTrans(actor));
    } else {
        mPreviousTrans.set(al::getTrans(actor));
    }
    al::connectPoseTrans(actor, mConnector, mBaseTrans);
}

void ItemStateCheckCollision::exeFall() {
    al::LiveActor* actor = mHostActor;
    if (al::isFirstStep(this) && !mPreserveInvalidClipping)
        al::invalidateClipping(actor);
    if (EnemyStateUtil::tryKillByAreaOrMaterialCodeWithHitReaction(actor))
        return;
    al::addVelocityToGravity(actor, rc::isInWaterArea(actor) ? 0.3f : 1.2f);
    al::scaleVelocity(actor, 0.99f);
    if (mWaterReaction)
        rc::startHitReactionIfThroughWater(actor);
    else
        rc::startHitReactionDeathIfThroughWater(actor);
    if (al::isCollidedGround(actor) && al::getVelocity(actor).y < 0.0f) {
        if (!mPreserveInvalidClipping)
            al::validateClipping(actor);
        connectToCollisionParts();
        if (al::isGreaterEqualStep(this, 5)) {
            al::tryEmitEffect(actor, "Land", nullptr);
            al::startSe(actor, "Land", nullptr);
        }
        al::setNerve(this, &NrvItemStateCheckCollisionWaitConnect);
    }
}
