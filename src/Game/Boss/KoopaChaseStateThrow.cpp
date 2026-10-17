#include "Boss/KoopaChaseStateThrow.hpp"
#include "Boss/KoopaChase.hpp"
#include "Boss/KoopaChaseKoopa.hpp"
#include "Boss/KoopaChaseFunction.hpp"
#include "Boss/KoopaChaseMover.hpp"
#include "Boss/KoopaChaseMovePos.hpp"
#include "Boss/BombBound.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Math/MatrixUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/LiveActor/Common/LiveActorGroup.hpp"
namespace {
NERVE_DECL(KoopaChaseStateThrow, ThrowSign);
NERVE_DECL(KoopaChaseStateThrow, Throw);
NERVES_MAKE_NOSTRUCT(KoopaChaseStateThrow, ThrowSign, Throw)
const char* const sThrowSign[] = {"LeftOverThrowSign", "RightOverThrowSign", "LeftUnderThrowSign", "RightUnderThrowSign"};
const char* const sThrow[] = {"LeftOverThrow", "RightOverThrow", "LeftUnderThrow", "RightUnderThrow"};
bool isLeftThrow(KoopaChaseStateThrow::ThrowType type) {
    return type == KoopaChaseStateThrow::ThrowType::LeftOver || type == KoopaChaseStateThrow::ThrowType::LeftUnder;
}
BombBound::Type getBombType(KoopaChaseStateThrow::ThrowType type) {
    return type == KoopaChaseStateThrow::ThrowType::LeftUnder || type == KoopaChaseStateThrow::ThrowType::RightUnder ? BombBound::Type_Roll : BombBound::Type_Bound;
}
const char* getThrowSign(KoopaChaseStateThrow::ThrowType type) { return sThrowSign[type]; }
const char* getThrowAction(KoopaChaseStateThrow::ThrowType type) { return sThrow[type]; }
void calcThrowOffset(float* pX, float* pY, KoopaChaseStateThrow::ThrowType type) {
    using ThrowType = KoopaChaseStateThrow::ThrowType;
    *pX = 222.0f;
    *pY = 550.0f;
    switch (type) {
    case ThrowType::RightOver: *pX = -222.0f; *pY = 550.0f; break;
    case ThrowType::LeftUnder: *pX = 222.0f; *pY = 344.0f; break;
    case ThrowType::RightUnder: *pX = -222.0f; *pY = 344.0f; break;
    default: break;
    }
}
sead::Vector3f calcThrowPosition(const al::LiveActor* pKoopa, const sead::Vector3f& offset) {
    sead::Matrix34f mtx = sead::Matrix34f::ident;
    al::makeMtxUpFront(&mtx, -al::getGravity(pKoopa), al::getFront(pKoopa));
    return mtx * offset;
}

}
/** @brief Creates the bomb pool and measures the throw animation duration. @param pHost Chase actor. @param rInfo Initialization data. @param throwSpeed Movement speed used to check remaining time. */
KoopaChaseStateThrow::KoopaChaseStateThrow(KoopaChase* pHost, const al::ActorInitInfo& rInfo, f32 throwSpeed)
    : al::NerveStateBase("クッパチェイスState[投げる]"), mHost(pHost), mThrowType(ThrowType::LeftUnder),
      mBallType(BallType::Bomb), mThrowMode(0), mBombs(nullptr), mThrowCount(0), mIsForce(false),
      mThrowSpeed(throwSpeed), mThrowDuration(0.0f) {
    initNerve(&NrvKoopaChaseStateThrowThrowSign, 0);
    mBombs = new al::DeriveActorGroup<BombBound>("はねる爆弾保持", 8);
    auto* group = mBombs;
    for (int i = 0; i < group->getMaxActorCount(); i++) {
        auto* bomb = new BombBound("はねる爆弾");
        al::initCreateActorWithPlacementInfo(bomb, rInfo);
        group->registerActor(bomb);
    }
    mBombs->makeActorDeadAll();
    for (int i = 0; i < mBombs->getMaxActorCount(); i++) al::invalidateClipping(mBombs->getDeriveActor(i));
    float sign = al::getActionFrameMax(KoopaChaseFunction::getKoopa(mHost), "LeftOverThrowSign");
    mThrowDuration = sign + al::getActionFrameMax(KoopaChaseFunction::getKoopa(mHost), "LeftOverThrow");
}
/** @brief Destroys the throw state. */
KoopaChaseStateThrow::~KoopaChaseStateThrow() = default;
/** @brief Removes the hand-held dummy bombs and ends the state. */
void KoopaChaseStateThrow::kill() {
    auto* left = al::getSubActor(KoopaChaseFunction::getKoopa(mHost), "爆弾ダミーL");
    auto* right = al::getSubActor(KoopaChaseFunction::getKoopa(mHost), "爆弾ダミーR");
    if (!al::isDead(left)) { al::startHitReaction(left, "消滅"); left->kill(); }
    if (!al::isDead(right)) { al::startHitReaction(right, "消滅"); right->kill(); }
    al::NerveStateBase::kill();
}
/** @brief Checks whether the next jump or warp leaves time to throw. @return Whether a throw can finish. */
bool KoopaChaseStateThrow::canThrow() const {
    if (KoopaChaseFunction::isCurrentPointJump(mHost) || KoopaChaseFunction::isCurrentPointWarp(mHost))
        return mThrowDuration < KoopaChaseFunction::calcDistanceToCurrentPoint(mHost) / mThrowSpeed;
    return true;
}
/** @brief Chooses the first hand and starts a throw sequence. @param isForce Enable the fuse on the final bomb. @return Whether the sequence started. */
bool KoopaChaseStateThrow::tryStartThrow(bool isForce) {
    if (!KoopaChaseFunction::getTargetPlayer(mHost)) return false;
    if (!canThrow()) return false;
    mThrowCount = 0;
    mIsForce = isForce;
    auto* point = mHost->mMover->mPreviousPoint;
    mThrowMode = point && static_cast<unsigned>(point->mValue24) < 2 ? point->mValue24 : 0;
    auto* koopa = KoopaChaseFunction::getKoopa(mHost);
    sead::Vector3f delta = al::getTrans(KoopaChaseFunction::getTargetPlayer(mHost)) - al::getTrans(koopa);
    sead::Vector3f front;
    al::calcFrontDir(&front, koopa);
    bool isLeft = delta.x * front.z - delta.z * front.x > 0.0f;
    startThrow((mThrowMode == 0) ^ isLeft);
    return true;
}
/** @brief Selects an overhand or underhand animation from target distance. @param isLeft Use the left hand. */
void KoopaChaseStateThrow::startThrow(bool isLeft) {
    mBallType = BallType::Bomb;
    bool isNear = KoopaChaseFunction::getTargetPlayer(mHost) &&
        al::calcDistance(mHost, KoopaChaseFunction::getTargetPlayer(mHost)) < 1200.0f;
    if (isLeft && isNear) mThrowType = ThrowType::LeftOver;
    else if (!isLeft && isNear) mThrowType = ThrowType::RightOver;
    else if (isLeft && !isNear) mThrowType = ThrowType::LeftUnder;
    else if (!isLeft && !isNear) mThrowType = ThrowType::RightUnder;
    al::setNerve(this, &NrvKoopaChaseStateThrowThrowSign);
}
/** @brief Vanishes every active bomb in the pool. */
void KoopaChaseStateThrow::vanishAll() {
    for (int i = 0; i < mBombs->getMaxActorCount(); i++)
        if (!al::isDead(mBombs->getDeriveActor(i))) mBombs->getDeriveActor(i)->vanish();
}
/** @brief Launches the selected projectile. @param type Throw animation type. @param ball Projectile type. */
void KoopaChaseStateThrow::doThrow(ThrowType type, BallType ball) {
    if (ball == BallType::Bomb) doThrowBomb(type, mIsForce && mThrowCount == 3, mThrowCount);
}
/** @brief Places and launches a bomb from the selected hand. @param type Throw animation type. @param isStartFuse Enable the fuse. @param throwCount Shot number in this sequence. */
void KoopaChaseStateThrow::doThrowBomb(ThrowType type, bool isStartFuse, int throwCount) {
    auto* bomb = mBombs->tryFindDeadDeriveActor();
    if (!bomb) return;
    bool isUnder = type == ThrowType::LeftUnder || type == ThrowType::RightUnder;
    BombBound::Type bombType = getBombType(type);
    al::copyPose(bomb, KoopaChaseFunction::getKoopa(mHost));
    sead::Vector3f front = al::getFront(mHost);
    sead::Vector3f side;
    al::calcSideDir(&side, mHost);
    float x, y;
    calcThrowOffset(&x, &y, type);
    auto* koopa = KoopaChaseFunction::getKoopa(mHost);
    sead::Vector3f offset = calcThrowPosition(koopa, sead::Vector3f(x, y, 300.0f));
    auto* pos = al::getTransPtr(bomb);
    pos->set(offset + al::getTrans(koopa));
    al::rotateVectorDegree(&front, front, side, isUnder ? 0.0f : 45.0f);
    if (throwCount != 3 && mThrowMode == 0) {
        if (type == ThrowType::LeftOver || type == ThrowType::LeftUnder)
            al::rotateVectorDegree(&front, front, sead::Vector3f::ey, -30.0f);
        else al::rotateVectorDegree(&front, front, sead::Vector3f::ey, 30.0f);
    }
    al::makeQuatFrontSide(al::getQuatPtr(bomb), front, side);
    float speed = al::calcSpeedH(mHost) < 2.0f ? 8.0f : 10.0f;
    bomb->thrown(bombType, front * (isUnder ? speed : 6.0f), isStartFuse);
    al::getTrans(bomb);
}
/** @brief Shows the dummy bomb during windup, then releases the real bomb. */
void KoopaChaseStateThrow::exeThrowSign() {
    if (al::isFirstStep(this)) {
        if (!canThrow()) { kill(); return; }
        const char* joint = mThrowType == ThrowType::LeftOver || mThrowType == ThrowType::LeftUnder ? "WristL" : "WristR";
        al::getJointMtxPtr(KoopaChaseFunction::getKoopa(mHost), joint);
        auto* dummy = al::getSubActor(KoopaChaseFunction::getKoopa(mHost),
            mThrowType == ThrowType::LeftOver || mThrowType == ThrowType::LeftUnder ? "爆弾ダミーL" : "爆弾ダミーR");
        dummy->appear();
        al::startAction(dummy, getThrowSign(mThrowType));
        al::startAction(KoopaChaseFunction::getKoopa(mHost), getThrowSign(mThrowType));
    }
    if (al::isActionEnd(KoopaChaseFunction::getKoopa(mHost))) {
        al::startAction(KoopaChaseFunction::getKoopa(mHost), getThrowAction(mThrowType));
        ++mThrowCount;
        doThrow(mThrowType, mBallType);
        al::setNerve(this, &NrvKoopaChaseStateThrowThrow);
    }
}
/** @brief Hides the dummy and alternates hands until three throws finish. */
void KoopaChaseStateThrow::exeThrow() {
    if (al::isFirstStep(this)) {
        auto* dummy = al::getSubActor(KoopaChaseFunction::getKoopa(mHost),
            mThrowType == ThrowType::LeftOver || mThrowType == ThrowType::LeftUnder ? "爆弾ダミーL" : "爆弾ダミーR");
        dummy->kill();
    }
    if (al::isActionEnd(KoopaChaseFunction::getKoopa(mHost))) {
        if (mThrowCount >= 3) { kill(); return; }
        if (!KoopaChaseFunction::getTargetPlayer(mHost)) { kill(); return; }
        startThrow(!isLeftThrow(mThrowType));
    }
}
