#include "Enemy/SkatisHead.hpp"
#include "Enemy/Skatis.hpp"
#include "Enemy/EnemyStateBlowDown.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Math/MatrixUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"

namespace {
NERVE_DECL(SkatisHead, Wait)
NERVE_DECL(SkatisHead, BlowDown)
NERVES_MAKE_STRUCT(SkatisHead, Wait, BlowDown)
EnemyStateBlowDownParam sSkatisHeadBlowDownParam(false);
}

/** @brief Creates the detachable Skatis head. @param pHost Owning enemy. */
SkatisHead::SkatisHead(const Skatis* pHost)
    : al::LiveActor("\u30b9\u30b1\u30fc\u30c6\u30a3\u30b9\u982d\u90e8\u5206"), mHost(pHost) {}

/** @brief Initializes the head model and knockback state. @param rInfo Placement information. */
void SkatisHead::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, al::isSingleMode(rInfo) ? "KuriboFur" : "Kuribo", "Skatis");
    mJointMtx = al::getJointMtxPtr(mHost, "Ancle");
    al::updatePoseMtx(this, mJointMtx);
    al::initNerve(this, &NrvSkatisHead.Wait, 1);
    mBlowDown = new EnemyStateBlowDown(this, &sSkatisHeadBlowDownParam);
    al::initNerveState(this, mBlowDown, &NrvSkatisHead.BlowDown, "[state]\u5439\u304d\u98db\u3070\u3057");
    al::offCollide(this);
    al::invalidateClipping(this);
    makeActorAppeared();
}

/** @brief Restores the head to its attached state. */
void SkatisHead::reset() {
    al::setNerve(this, &NrvSkatisHead.Wait);
    al::invalidateClipping(this);
    makeActorAppeared();
}

/** @brief Turns the attached head toward the host's pursuit target. */
void SkatisHead::exeWait() {
    if (mHost->getTarget()) {
        sead::Vector3f front;
        al::calcFrontDir(&front, mHost);
        sead::Vector3f up;
        al::calcUpDir(&up, this);
        sead::Vector3f direction = al::getTrans(mHost->getTarget()) - al::getTrans(this);
        f32 angle = al::calcAngleOnPlaneDegree(front, direction, up);
        if (angle < -60.0f)
            angle = -60.0f;
        else if (angle > 60.0f)
            angle = 60.0f;
        mAngle = al::converge(mAngle, angle, 3.0f);
    } else {
        mAngle = al::converge(mAngle, 0.0f, 3.0f);
    }
    sead::Matrix34f pose;
    al::rotateMtxYDirDegree(&pose, *mJointMtx, mAngle);
    al::updatePoseMtx(this, &pose);
}

/** @brief Finishes knockback, emits the death reaction and releases the host's item. */
void SkatisHead::exeBlowDown() {
    if (al::updateNerveState(this)) {
        al::startHitReactionDeath(this);
        al::appearItem(mHost, al::getTrans(this), sead::Vector3f::ey);
        kill();
    }
}

/** @brief Starts directional knockback. @param pOther Attacker sensor. @param pSelf Head sensor. */
void SkatisHead::startBlowDown(al::HitSensor* pOther, al::HitSensor* pSelf) {
    mBlowDown->setBlowDir(pOther, pSelf);
    al::setNerve(this, &NrvSkatisHead.BlowDown);
}
