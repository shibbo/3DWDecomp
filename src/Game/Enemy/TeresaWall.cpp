#include "Enemy/TeresaWall.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Rail/RailUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include <math/seadBoundBox.h>

namespace {
NERVE_DECL(TeresaWall, MoveSign)
NERVE_DECL(TeresaWall, Stop)
NERVE_DECL(TeresaWall, Move)
NERVES_MAKE_STRUCT(TeresaWall, MoveSign, Stop, Move)
}

/** @brief Creates the wall with default speed and detection distance. @param pName Actor name. */
TeresaWall::TeresaWall(const char* pName) : al::LiveActor(pName) {}

/** @brief Initializes the rail and scaled attack sensor. @param rInfo Placement information. */
void TeresaWall::init(const al::ActorInitInfo& rInfo) {
    al::initActor(this, rInfo);
    al::initNerve(this, &NrvTeresaWall.Stop, 0);
    al::tryGetArg(&mMoveSpeed, rInfo, "MoveSpeed");
    al::tryGetArg(&mFindDistance, rInfo, "FindDistance");
    al::tryGetArg(&mWaitTime, rInfo, "WaitTime");
    if (al::isExistRail(this)) {
        al::setSyncRailToStart(this);
        al::setRailClippingInfo(&mClippingCenter, this, 100.0f, 600.0f);
        al::setSensorRadius(this, "Attack", al::getScale(this).x * 85.0f);
        sead::Vector3f offset(0.0f, al::getScale(this).y * 50.0f, 0.0f);
        al::setSensorFollowPosOffset(this, "Attack", offset);
        makeActorAppeared();
    } else {
        makeActorDead();
    }
}

/** @brief Breaks non-player actors inside the moving wall. @param pSelf Own sensor. @param pOther Contact sensor. */
void TeresaWall::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (!al::isNerve(this, &NrvTeresaWall.Move))
        return;
    sead::Vector3f center = al::getSensorPos(pSelf);
    sead::Vector3f other = al::getSensorPos(pOther);
    f32 scale = al::getScale(this).x;
    sead::Vector3f front;
    al::calcFrontDir(&front, this);
    al::normalize(&front);
    center.x += front.x * 55.0f;
    center.z += front.z * 55.0f;
    f32 halfSize = scale * 100.0f * 0.5f;
    sead::BoundBox3f box(center - sead::Vector3f(halfSize, halfSize, halfSize),
                       center + sead::Vector3f(halfSize, halfSize, halfSize));
    if (box.isInside(other) && !al::isSensorPlayer(pOther))
        al::sendMsgExplosion(pOther, pSelf, nullptr);
}

/** @brief Waits shyly until a player in front looks away. */
void TeresaWall::exeStop() {
    if (al::isFirstStep(this))
        al::startAction(this, "ShyWait");
    if (isStartMove())
        al::setNerve(this, &NrvTeresaWall.MoveSign);
}

/** @brief Tests the detection volume and player facing direction. @return Whether a player permits movement. */
bool TeresaWall::isStartMove() {
    al::getTrans(this);
    sead::Vector3f front;
    al::calcFrontDir(&front, this);
    al::normalize(&front);
    sead::Vector3f scale = al::getScale(this);
    sead::Vector3f center = al::getTrans(this);
    f32 distance = mFindDistance;
    f32 absX = front.x > 0.0f ? front.x : -front.x;
    f32 absZ = front.z > 0.0f ? front.z : -front.z;
    center = front * (scale.z * 50.0f + distance * 0.5f) + center;
    center.y = (scale.z * 50.0f - 5.0f) + center.y;
    sead::Vector3f halfSize((scale.x * 100.0f + distance * absX) * 0.5f,
                          scale.y * 100.0f * 0.5f,
                          (scale.z * 100.0f + distance * absZ) * 0.5f);
    sead::BoundBox3f box(center - halfSize, center + halfSize);
    s32 count = al::getPlayerNumMax(this);
    for (s32 i = 0; i < count; ++i) {
        auto* pPlayer = al::getPlayerActor(this, i);
        if (al::isDead(pPlayer))
            continue;
        sead::Vector3f playerPosition = al::getTrans(pPlayer);
        if (box.isInside(playerPosition)) {
            sead::Vector3f playerFront = rc::getPlayerFront(pPlayer);
            al::normalize(&playerFront);
            if (playerFront.dot(front) >= -0.1f)
                return true;
        }
    }
    return false;
}

/** @brief Delays movement while the player remains eligible. */
void TeresaWall::exeMoveSign() {
    if (isStartMove()) {
        if (al::isGreaterEqualStep(this, mWaitTime))
            al::setNerve(this, &NrvTeresaWall.Move);
    } else {
        al::setNerve(this, &NrvTeresaWall.Stop);
    }
}

/** @brief Advances on the rail until the player faces the wall. */
void TeresaWall::exeMove() {
    if (al::isFirstStep(this))
        al::startAction(this, "Wait");
    al::moveSyncRail(this, mMoveSpeed);
    if (!isStartMove())
        al::setNerve(this, &NrvTeresaWall.Stop);
}
