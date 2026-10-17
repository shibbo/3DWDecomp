#include "MapObj/KinokoStateRunaway.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Enemy/EnemyStateUtil.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
namespace {
    NERVE_DECL(KinokoStateRunaway, Land);
    NERVE_DECL(KinokoStateRunaway, Reaction);
    NERVE_DECL(KinokoStateRunaway, Move);
    NERVES_MAKE_NOSTRUCT(KinokoStateRunaway, Land, Reaction, Move)
}
KinokoStateRunaway::KinokoStateRunaway(al::LiveActor* pHost)
    : al::ActorStateBase("キノコ逃げステート", pHost) {}
void KinokoStateRunaway::init() { initNerve(&NrvKinokoStateRunawayLand, 0); }
void KinokoStateRunaway::appear() {
    mIsDead = false;
    al::invalidateClipping(mHostActor);
    al::setNerve(this, &NrvKinokoStateRunawayLand);
    mBaseSpeed = mSpeed;
}
bool KinokoStateRunaway::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor*, al::HitSensor*) {
    if (rc::isMsgJumpPanelAction(pMsg) && al::getVelocity(mHostActor).y <= 0.0f)
        al::getVelocityPtr(mHostActor)->y = 24.0f;
    return false;
}
void KinokoStateRunaway::move(float speed, float gravity, float velocityScale,
                              float turnDegrees, float deceleration) {
    mSpeed = mSpeed > mBaseSpeed ? mSpeed - deceleration : mBaseSpeed;
    if (al::isOnGround(mHostActor, 0, 0.0f)) {
        sead::Vector3f direction;
        al::verticalizeVec(&direction, al::getGravity(mHostActor), al::getVelocity(mHostActor));
        if (al::normalizeOrZero(&direction))
            al::calcFrontDir(&direction, mHostActor);
        direction *= speed;
        al::setVelocity(mHostActor, direction);
    } else {
        al::addVelocityToGravity(mHostActor, gravity);
        al::scaleVelocityHV(mHostActor, velocityScale, velocityScale);
    }
    if (GameDataFunction::isSingleMode(GameDataHolderAccessor(mHostActor)) && rc::isInWaterArea(mHostActor)) {
        al::startHitReactionDeath(mHostActor);
        mHostActor->kill();
    }
    if (al::isCollidedWall(mHostActor)) {
        sead::Vector3f velocity = al::getVelocity(mHostActor);
        sead::Vector3f reflected = velocity;
        al::calcReflectionVector(&reflected, al::getCollidedWallNormal(mHostActor), 1.0f, 0.0f);
        if (!al::isParallelDirection(velocity, al::getGravity(mHostActor), 0.01f))
            al::setVelocity(mHostActor, reflected);
        if (!al::isNerve(this, &NrvKinokoStateRunawayReaction))
            al::setNerve(this, &NrvKinokoStateRunawayReaction);
    } else {
        sead::Vector3f direction = al::getVelocity(mHostActor);
        if (!al::normalizeOrZero(&direction)) {
            if (al::tryGetQuatPtr(mHostActor))
                al::turnQuatFrontToDirDegreeH(mHostActor, direction, turnDegrees);
            else
                al::turnDirectionDegree(mHostActor, al::getFrontPtr(mHostActor), direction, turnDegrees);
        }
    }
}
void KinokoStateRunaway::exeLand() {
    if (al::isFirstStep(this) && !al::tryStartAction(mHostActor, "Land")) {
        al::setNerve(this, &NrvKinokoStateRunawayMove);
        return;
    }
    if (rc::isInWaterArea(mHostActor))
        move(3.0f, 0.2f, 0.98f, 4.0f, 0.05f);
    else
        move(mSpeed, 0.4f, 0.99f, 8.0f, 0.1f);
    if (al::isActionEnd(mHostActor))
        al::setNerve(this, &NrvKinokoStateRunawayMove);
}
void KinokoStateRunaway::exeMove() {
    if (al::isFirstStep(this))
        al::startAction(mHostActor, "Move");
    if (EnemyStateUtil::tryKillByAreaOrMaterialCodeWithHitReaction(mHostActor))
        return;
    if (rc::isInWaterArea(mHostActor))
        move(3.0f, 0.2f, 0.98f, 4.0f, 0.05f);
    else
        move(mSpeed, 0.4f, 0.99f, 8.0f, 0.1f);
}
void KinokoStateRunaway::exeReaction() {
    if (al::isFirstStep(this)) {
        if (!al::tryStartAction(mHostActor, "Reaction")) {
            al::setNerve(this, &NrvKinokoStateRunawayMove);
            return;
        }
        al::tryStartSe(mHostActor, "PgHitWall");
    }
    if (rc::isInWaterArea(mHostActor))
        move(3.0f, 0.2f, 0.98f, 4.0f, 0.05f);
    else
        move(mSpeed, 0.4f, 0.99f, 8.0f, 0.1f);
    if (al::isActionEnd(mHostActor))
        al::setNerve(this, &NrvKinokoStateRunawayMove);
}
