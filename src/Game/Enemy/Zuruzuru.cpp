#include "Enemy/Zuruzuru.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Project/Collision/CollisionUtil.hpp"
namespace {
NERVE_DECL(Zuruzuru, Wait)
NERVE_DECL(Zuruzuru, TumbleSign)
NERVE_DECL(Zuruzuru, Walk)
NERVE_DECL(Zuruzuru, Turn)
NERVE_DECL(Zuruzuru, Find)
NERVE_DECL(Zuruzuru, Tumble)
NERVE_DECL(Zuruzuru, TumbleWait)
NERVE_DECL(Zuruzuru, StandUp)
NERVES_MAKE_NOSTRUCT(Zuruzuru, Wait, TumbleSign, Walk, Turn, Find, Tumble, TumbleWait, StandUp)
}
/** @brief Creates the walking platform. @param pName Actor name. */
Zuruzuru::Zuruzuru(const char* pName) : al::LiveActor(pName) {}
/** @brief Saves the placement orientation. @param rInfo Placement information. */
void Zuruzuru::init(const al::ActorInitInfo& rInfo) {
    al::initActor(this, rInfo);
    al::initNerve(this, &NrvZuruzuruWait, 0);
    mInitialQuat = al::getQuat(this);
    makeActorAppeared();
}
/** @brief Reacts to a player standing on the platform. @param pMsg Message. @param pOther Sender. @param pSelf Own sensor. @return Whether handled. */
bool Zuruzuru::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) {
    if (al::isMsgPlayerFloorTouch(pMsg)) {
        mStepsWithoutRider = 0;
        if (isEnableTumble()) {
            al::invalidateClipping(this);
            al::setNerve(this, &NrvZuruzuruTumbleSign);
            return true;
        }
    }
    return false;
}
/** @brief Checks whether a rider can cause a tumble. @return Whether walking long enough or turning. */
bool Zuruzuru::isEnableTumble() const {
    if (al::isNerve(this, &NrvZuruzuruWalk)) return al::isGreaterEqualStep(this, 12);
    return al::isNerve(this, &NrvZuruzuruTurn);
}
/** @brief Waits before choosing a player to approach. */
void Zuruzuru::exeWait() {
    if (al::isFirstStep(this)) al::startAction(this, "Wait");
    if (al::isGreaterEqualStep(this, 30)) {
        mPlayerId = al::findNearestPlayerId(this, -1.0f);
        if (mPlayerId >= 0) al::setNerve(this, &NrvZuruzuruFind);
        else al::setNerve(this, &NrvZuruzuruWalk);
    }
}
/** @brief Walks along the selected axis, checking the ground ahead. */
void Zuruzuru::exeWalk() {
    if (al::isFirstStep(this)) { al::startAction(this, "Walk"); mWalkStart = al::getTrans(this); }
    s32 duration = static_cast<s32>(mWalkDistance / 7.0f);
    al::setTrans(this, al::calcNerveValue(this, duration, 0.0f, mWalkDistance) * mDirection + mWalkStart);
    sead::Vector3f up = sead::Vector3f::ey;
    al::calcUpDir(&up, this);
    s32 ground = alCollisionUtil::checkStrikeArrow(this, al::getTrans(this) + up * 25.0f + mDirection * 25.0f, up * -50.0f, nullptr, nullptr);
    bool finished = al::isGreaterEqualStep(this, duration);
    if (!ground || finished) {
        mPlayerId = al::findNearestPlayerId(this, -1.0f);
        if (mPlayerId >= 0) al::setNerve(this, &NrvZuruzuruFind);
        else al::setNerve(this, &NrvZuruzuruTurn);
    }
}
/** @brief Chooses the placement axis most closely aligned with the player. */
void Zuruzuru::exeFind() {
    if (al::isFirstStep(this)) {
        const sead::Vector3f playerPos = al::getPlayerPos(this, mPlayerId);
        sead::Vector3f toPlayer = playerPos - al::getTrans(this);
        sead::Vector3f side = sead::Vector3f::ex;
        sead::Vector3f up = sead::Vector3f::ey;
        sead::Vector3f front = sead::Vector3f::ez;
        al::calcQuatLocalAxisAll(mInitialQuat, &side, &up, &front);
        al::verticalizeVec(&toPlayer, up, toPlayer);
        f32 sideDot = side.dot(toPlayer);
        f32 frontDot = toPlayer.dot(front);
        f32 sideDistance = sideDot > 0.0f ? sideDot : -sideDot;
        f32 frontDistance = frontDot > 0.0f ? frontDot : -frontDot;
        if (sideDistance > frontDistance) {
            mDirection = sideDot >= 0.0f ? side : -side;
            if (frontDistance < mAxisThreshold) mWalkDistance = 1000.0f;
            else mWalkDistance = sideDistance;
        } else {
            mDirection = frontDot >= 0.0f ? front : -front;
            if (sideDistance < mAxisThreshold) mWalkDistance = 1000.0f;
            else mWalkDistance = frontDistance;
        }
        al::startAction(this, "Walk");
    }
    al::turnQuatFrontToDirDegreeH(this, mDirection, 8.0f);
    if (al::isGreaterEqualStep(this, 30)) {
        al::turnQuatFrontToDirDegreeH(this, mDirection, 180.0f);
        al::setNerve(this, &NrvZuruzuruWalk);
    }
}
/** @brief Turns relative to the placement orientation. */
void Zuruzuru::exeTurn() {
    if (al::isFirstStep(this)) al::startAction(this, "Walk");
    mTurnAngle = al::modf(mTurnAngle + 0.75f + 360.0f, 360.0f) + 0.0f;
    al::rotateQuatYDirDegree(al::getQuatPtr(this), mInitialQuat, mTurnAngle);
    if (al::isGreaterEqualStep(this, 120)) al::setNerve(this, &NrvZuruzuruWalk);
}
/** @brief Slows before tumbling while the rider remains aboard. */
void Zuruzuru::exeTumbleSign() {
    if (al::isFirstStep(this)) { al::startAction(this, "TumbleSign"); mStepsWithoutRider = 1; }
    else if (mStepsWithoutRider++ >= 4) { al::setNerve(this, &NrvZuruzuruWait); return; }
    al::addTransOffsetLocalDir(this, al::calcNerveValue(this, 60, 7.0f, 0.0f), 2);
    if (al::isGreaterEqualStep(this, 60)) al::setNerve(this, &NrvZuruzuruTumble);
}
/** @brief Plays the tumble animation. */
void Zuruzuru::exeTumble() {
    if (al::isFirstStep(this)) al::startAction(this, "Tumble");
    if (al::isActionEnd(this)) al::setNerve(this, &NrvZuruzuruTumbleWait);
}
/** @brief Stays down until the rider leaves. */
void Zuruzuru::exeTumbleWait() {
    if (al::isFirstStep(this)) al::startAction(this, "TumbleWait");
    ++mStepsWithoutRider;
    if (al::isGreaterEqualStep(this, 120) && mStepsWithoutRider >= 30) al::setNerve(this, &NrvZuruzuruStandUp);
}
/** @brief Stands up and selects the next walking direction. */
void Zuruzuru::exeStandUp() {
    if (al::isFirstStep(this)) al::startAction(this, "StandUp");
    if (al::isActionEnd(this)) {
        mPlayerId = al::findNearestPlayerId(this, -1.0f);
        if (mPlayerId >= 0) al::setNerve(this, &NrvZuruzuruFind);
        else al::setNerve(this, &NrvZuruzuruWait);
    }
}
