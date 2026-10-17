#include "Enemy/PipePackunBodyParts.hpp"
#include <cmath>
#include "Enemy/PipePackun.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Factory/Factory.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/ProjectMsgUtil.hpp"
namespace {
NERVE_DECL(PipePackunBodyParts, Wait)
NERVES_MAKE_NOSTRUCT(PipePackunBodyParts, Wait)
}
/** @brief Creates a stem segment. @param pHost Plant head. @param pName Actor name. */
PipePackunBodyParts::PipePackunBodyParts(PipePackun* pHost, const char* pName) : al::LiveActor(pName), mHost(pHost) {}
/** @brief Initializes segment geometry and direction. @param rInfo Placement information. */
void PipePackunBodyParts::init(const al::ActorInitInfo& rInfo) {
    al::initActorChangeModel(this, rInfo);
    const char* pModel = nullptr;
    if (alPlacementFunction::tryGetModelName(&pModel, rInfo)) mIsStraight = al::isEqualString(pModel, "PipePackunBodyPartsStraight");
    if (mIsStraight) al::calcFrontDir(&mDirection, this);
    else al::calcSideDir(&mDirection, this);
    mLength = al::getScale(this).z * 200.0f;
    al::initNerve(this, &NrvPipePackunBodyPartsWait, 0);
    makeActorAppeared();
}
/** @brief Checks the segment shape. @return Whether the segment is straight. */
bool PipePackunBodyParts::isStraight() const { return mIsStraight; }
/** @brief Tests a sensor against the exposed stem. @param pSelf Own sensor. @param pOther Contact sensor. @param pHitPos Optional hit position. @param pNormal Optional normal. @return Whether it intersects. */
bool PipePackunBodyParts::isHitToBody(al::HitSensor* pSelf, al::HitSensor* pOther, sead::Vector3f* pHitPos, sead::Vector3f* pNormal) {
    if (mIsStraight) {
        if (!(mCoordRate >= 0.8f)) return false;
        sead::Vector3f pos = al::getSensorPos(pSelf);
        if (mHost->isDamageType2D()) {
            sead::Vector3f side;
            al::calcSideDir(&side, mHost);
            pos += side * (al::getSensorPos(pOther) - pos).dot(side);
        }
        if (al::isHitCylinderSensor(pHitPos, pNormal, pOther, pos, mDirection, 40.0f)) return true;
    } else {
        if (!(mCoordRate >= 0.8f)) return false;
        sead::Vector3f pos = sead::Vector3f::zero;
        al::multVecPose(&pos, this, sead::Vector3f(0.0f, 100.0f, 100.0f));
        if (mHost->isDamageType2D()) {
            sead::Vector3f side;
            al::calcSideDir(&side, mHost);
            pos += side * (al::getSensorPos(pOther) - al::getSensorPos(pSelf)).dot(side);
        }
        if (al::isHitCircleSensor(pHitPos, pNormal, pOther, pos, mDirection, 100.0f, 40.0f)) return true;
    }
    return false;
}
/** @brief Reflects items and damages actors touching the stem. @param pSelf Own sensor. @param pOther Contact sensor. */
void PipePackunBodyParts::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isHideModel(this)) return;
    sead::Vector3f normal;
    if (isHitToBody(pSelf, pOther, nullptr, &normal)) {
        rc::sendMsgItemReflect(pOther, pSelf, normal);
        al::sendMsgEnemyAttack(pOther, pSelf);
    }
    if (GameDataFunction::isSingleMode(GameDataHolderAccessor(this))) {
        if (al::isSensorKickKoura(pOther)) al::sendMsgKouraDestroy(pOther, pSelf);
        else if (al::isSensorMapObj(pOther)) al::sendMsgPushStrong(pOther, pSelf);
    }
}
/** @brief Forwards qualifying attacks to the plant head. @param pMsg Message. @param pOther Sender. @param pSelf Own sensor. @return Whether handled. */
bool PipePackunBodyParts::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) {
    if (!al::isSensorRide(pOther) && (al::isMsgPlayerFireBallAttack(pMsg) || al::isMsgPlayerBoomerangReflect(pMsg)) && isHitToBody(pSelf, pOther, nullptr, nullptr)) {
        rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        return true;
    }
    if (al::isMsgPlayerInvincibleAttack(pMsg) && isHitToBody(pSelf, pOther, nullptr, nullptr)) {
        mHost->hitInvincibleAttack(pMsg, pOther);
        rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        return true;
    }
    if (GameDataFunction::isSingleMode(GameDataHolderAccessor(this)) && rc::isMsgBobsledBodyAttack(pMsg) && isHitToBody(pSelf, pOther, nullptr, nullptr)) return mHost->receiveMsg(pMsg, pOther, pSelf);
    return false;
}
/** @brief Removes the segment with a disappearance reaction when visible. */
void PipePackunBodyParts::killSwitch() { if (!al::isHideModel(this)) al::startHitReactionDisappear(this); kill(); }
/** @brief Sets the endpoint coordinate. @param length Cumulative stem length. */
void PipePackunBodyParts::setTotalLength(f32 length) { mTotalLength = length; }
/** @brief Updates visibility and animation from the head coordinate. @param coord Head coordinate. */
void PipePackunBodyParts::setCoord(f32 coord) {
    f32 end = mTotalLength;
    f32 start = end - mLength;
    f32 offset = coord - start;
    mCoordRate = offset / mLength;
    if (start > coord) { al::hideModelIfShow(this); return; }
    al::showModelIfHide(this);
    if (end > coord) {
        f32 rate = offset / mLength;
        f32 frame = mTurnPattern ? al::lerpValue(rate, 13.0f, 25.0f) : al::lerpValue(rate, 0.0f, 12.0f);
        if (al::isMtsAnimExist(this, "Appear")) { al::tryStartMtsAnimIfNotPlaying(this, "Appear"); al::setMtsAnimFrameAndStop(this, frame); }
        if (al::isSklAnimExist(this, "Appear")) { al::tryStartSklAnimIfNotPlaying(this, "Appear"); al::setSklAnimFrame(this, frame, 0); al::setSklAnimFrameRate(this, 0.0f, 0); }
        if (al::isVisAnimExist(this, "Appear")) { al::tryStartVisAnimIfNotPlaying(this, "Appear"); al::setVisAnimFrameAndStop(this, frame); }
    } else {
        f32 rate = al::modf(coord, mLength) / mLength;
        f32 frame = mTurnPattern ? al::lerpValue(rate, 13.0f, 25.0f) : al::lerpValue(rate, 0.0f, 12.0f);
        if (al::isMtsAnimExist(this, "Move")) { al::tryStartMtsAnimIfNotPlaying(this, "Move"); al::setMtsAnimFrameAndStop(this, frame); }
        if (al::isSklAnimExist(this, "Move")) { al::tryStartSklAnimIfNotPlaying(this, "Move"); al::setSklAnimFrame(this, frame, 0); al::setSklAnimFrameRate(this, 0.0f, 0); }
        if (al::isVisAnimExist(this, "Move")) { al::tryStartVisAnimIfNotPlaying(this, "Move"); al::setVisAnimFrameAndStop(this, frame); }
    }
}
/** @brief Gets segment length. @return Length in world units. */
f32 PipePackunBodyParts::getLength() const { return mLength; }
/** @brief Alternates the texture pattern across adjoining bends. @param pPrevious Previous segment. */
void PipePackunBodyParts::setTurnPattern(const PipePackunBodyParts* pPrevious) {
    sead::Vector3f previousSide, side;
    al::calcSideDir(&previousSide, pPrevious);
    al::calcSideDir(&side, this);
    f32 dot = previousSide.dot(side);
    mTurnPattern = (dot > 0.0f ? dot : -dot) <= 0.707f;
    if (pPrevious->mTurnPattern) mTurnPattern = !mTurnPattern;
}
/** @brief Waits while the plant head drives the segment. */
void PipePackunBodyParts::exeWait() {}
/** @brief Calculates a point along the stem. @param pPos Output position. @param rate Fraction along the segment. */
void PipePackunBodyParts::calcPos(sead::Vector3f* pPos, f32 rate) const {
    s32 length = static_cast<s32>(mLength);
    if (mIsStraight) pPos->setScaleAdd((rate - 0.5f) * length, mDirection, al::getTrans(this));
    else {
        f32 radius = length * 0.5f;
        f32 angle = rate * 90.0f * 0.017453292f;
        f32 z = radius * (1.0f - cosf(angle));
        f32 y = radius * (1.0f - sinf(angle));
        al::multVecPose(pPos, this, sead::Vector3f(0.0f, y, z));
    }
}
/** @brief Calculates the stem tangent. @param pDir Output direction. @param rate Fraction along the segment. */
void PipePackunBodyParts::calcDir(sead::Vector3f* pDir, f32 rate) const {
    if (mIsStraight) pDir->set(mDirection);
    else {
        f32 angle = rate * 90.0f * 0.017453292f;
        f32 z = sinf(angle);
        f32 y = -cosf(angle);
        pDir->setRotated(al::getQuat(this), sead::Vector3f(0.0f, y, z));
    }
}
/** @brief Finds the nearest coordinate along the segment. @param pDistance Optional distance output. @param rPos World position. @return Cumulative stem coordinate. */
f32 PipePackunBodyParts::calcNearCoord(f32* pDistance, const sead::Vector3f& rPos) {
    f32 rate;
    if (mIsStraight) {
        f32 radius = mLength * 0.5f;
        sead::Vector3f foot = sead::Vector3f::zero;
        sead::Vector3f start = al::getTrans(this) - radius * mDirection;
        const sead::Vector3f& trans = al::getTrans(this);
        sead::Vector3f end = radius * mDirection + trans;
        rate = al::calcPerpendicFootToLineInside(&foot, rPos, start, end);
        if (pDistance) *pDistance = (rPos - foot).length();
    } else {
        sead::Vector3f local;
        al::multVecInvPose(&local, this, rPos);
        rate = al::normalize(atan2f(-(local.y - 100.0f), -(local.z - 100.0f)) * 57.295776f, 0.0f, 90.0f);
        if (pDistance) {
            sead::Vector3f foot = sead::Vector3f::zero;
            calcPos(&foot, rate);
            *pDistance = (rPos - foot).length();
        }
    }
    f32 remaining = mLength * (1.0f - rate);
    return mTotalLength - remaining;
}
