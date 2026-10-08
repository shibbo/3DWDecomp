#include "Boss/KoopaLastWallClimb.hpp"
#include "Boss/KoopaLastFunction.hpp"
#include "Boss/KoopaLastStateAttackBreathFire.hpp"
#include "Boss/KoopaLastStateTop.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/KeyPose/KeyPoseKeeper.hpp"
#include "Library/KeyPose/KeyPoseKeeperUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"

namespace {
NERVE_DECL(KoopaLastWallClimb, Appear);
NERVE_DECL(KoopaLastWallClimb, Attack);
NERVE_DECL(KoopaLastWallClimb, Top);
NERVE_DECL(KoopaLastWallClimb, Climb);
NERVE_DECL(KoopaLastWallClimb, Hide);
NERVE_DECL(KoopaLastWallClimb, PowBlockDamage);
NERVE_DECL(KoopaLastWallClimb, PowBlockDamageFall);
NERVE_DECL(KoopaLastWallClimb, LastPowDamage);
NERVE_DECL(KoopaLastWallClimb, ChaseRouteDokan);
NERVE_DECL(KoopaLastWallClimb, Land);
NERVE_DECL(KoopaLastWallClimb, JumpWait);
NERVE_DECL(KoopaLastWallClimb, ClimbEnd);
NERVE_DECL(KoopaLastWallClimb, Wait);
NERVE_DECL(KoopaLastWallClimb, End);
NERVE_DECL(KoopaLastWallClimb, JumpStart);
NERVE_DECL(KoopaLastWallClimb, Jump);
NERVES_MAKE_NOSTRUCT(KoopaLastWallClimb, LastPowDamage, End, JumpStart, Jump, Appear, Attack, Top,
                     Climb, Hide, PowBlockDamage, PowBlockDamageFall, ChaseRouteDokan, Land,
                     JumpWait, ClimbEnd, Wait)
}  // namespace

typedef KoopaLastWallClimbBehaviorType BehaviorType;

/**
 * @brief Creates a wall-climbing final-battle Koopa with no route or states assigned yet.
 * @param pName Actor name.
 */
KoopaLastWallClimb::KoopaLastWallClimb(const char* pName) : al::LiveActor(pName) {}

/**
 * @brief Initializes the climbing route, clipping, behavior-specific states and arguments.
 * @param rInfo Actor placement and scene initialization information.
 */
void KoopaLastWallClimb::init(const al::ActorInitInfo& rInfo) {
    KoopaLastFunction::initActorKoopaLastCommon(this, rInfo, nullptr, nullptr);
    al::initNerve(this, &NrvKoopaLastWallClimbAppear, 2);
    mKeyPoseKeeper = al::createKeyPoseKeeper(rInfo);

    f32 clippingRadius = 0.0f;
    al::calcKeyMoveClippingInfo(&mClippingTrans, &clippingRadius, mKeyPoseKeeper, 500.0f);
    al::setClippingInfo(this, clippingRadius, &mClippingTrans);
    al::tryGetArg(reinterpret_cast<s32*>(&mBehaviorType), rInfo, "BehaviorType");

    if (mBehaviorType == BehaviorType::Attack) {
        mAttackState = new KoopaLastStateAttackBreathFire(this, rInfo);
        al::initNerveState(this, mAttackState, &NrvKoopaLastWallClimbAttack, "炎攻撃");
    } else if (mBehaviorType == BehaviorType::Top) {
        mTopState = new KoopaLastStateTop(this);
        al::initNerveState(this, mTopState, &NrvKoopaLastWallClimbTop, "頂上待機");
    }

    al::tryGetArg(&mIsUseAppear, rInfo, "IsUseAppear");
    al::tryGetArg(&mLandWaitTime, rInfo, "LandWaitTime");
    al::startAction(this, "ClimbWait");

    if (!al::tryListenStageSwitchAppear(this)) {
        appear();
    }
}

/** @brief Starts either the appear sequence or climbing, depending on IsUseAppear. */
void KoopaLastWallClimb::appear() {
    al::validateClipping(this);
    al::validateHitSensors(this);
    al::LiveActor::appear();

    if (mIsUseAppear) {
        al::hideModelIfShow(this);
        al::setNerve(this, &NrvKoopaLastWallClimbAppear);
    } else {
        al::showModelIfHide(this);
        al::setNerve(this, &NrvKoopaLastWallClimbClimb);
    }
}

/**
 * @brief Pushes the player away with the body's enemy-attack sensor.
 * @param pSelf Attacking sensor.
 * @param pOther Sensor being attacked.
 */
void KoopaLastWallClimb::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isNerve(this, &NrvKoopaLastWallClimbHide)) {
        return;
    }

    if (al::isNerve(this, &NrvKoopaLastWallClimbAppear) && al::isLessStep(this, 2)) {
        return;
    }

    if (KoopaLastFunction::attackSensorCommon(pSelf, pOther)) {
        return;
    }

    if (al::isSensorEnemyAttack(pSelf) && al::isSensorPlayer(pOther)) {
        al::sendMsgPush(pOther, pSelf);
    }
}

/**
 * @brief Handles the common final-battle messages and POW block hits.
 * @param pMsg Incoming sensor message.
 * @param pSelf Receiving sensor.
 * @param pOther Sending sensor.
 * @return True if the message was handled.
 */
bool KoopaLastWallClimb::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSelf,
                                    al::HitSensor* pOther) {
    if (KoopaLastFunction::receiveMsgCommon(pMsg, pSelf, pOther)) {
        return true;
    }

    if (al::isNerve(this, &NrvKoopaLastWallClimbPowBlockDamage)) {
        return false;
    }

    if (al::isNerve(this, &NrvKoopaLastWallClimbPowBlockDamageFall)) {
        return false;
    }

    if (KoopaLastFunction::isReceivePowBlockMsg(pMsg, pSelf, pOther)) {
        al::setNerve(this, &NrvKoopaLastWallClimbPowBlockDamage);
        return true;
    }

    return false;
}

/**
 * @brief Forwards screen-point messages to the common final-battle handler.
 * @param pMsg Incoming sensor message.
 * @param pPointer Screen pointer that sent the message.
 * @param pTarget Screen point target that received it.
 * @return True if the message was handled.
 */
bool KoopaLastWallClimb::receiveMsgScreenPoint(const al::SensorMsg* pMsg,
                                               al::ScreenPointer* pPointer,
                                               al::ScreenPointTarget* pTarget) {
    return KoopaLastFunction::receiveMsgScreenPointCommon(pMsg, pPointer, pTarget);
}

/** @brief Disables sensors and clipping and starts the final POW damage reaction. */
void KoopaLastWallClimb::startLastPowDamage() {
    al::invalidateHitSensors(this);
    al::invalidateClipping(this);
    al::setNerve(this, &NrvKoopaLastWallClimbLastPowDamage);
}

/** @brief Slides in from behind the wall onto the first key pose. */
void KoopaLastWallClimb::exeAppear() {
    if (al::isFirstStep(this)) {
        const char* actionName = mBehaviorType == BehaviorType::RouteDokan ?
                                     "RouteDokanClimbAppear" :
                                 mBehaviorType == BehaviorType::DoubleCherry ?
                                     "DoubleCherryClimbAppear" :
                                     "ClimbAppear";
        al::startAction(this, actionName);
        mAppearTrans = al::getCurrentKeyTrans(mKeyPoseKeeper);
        al::setQuat(this, al::getCurrentKeyQuat(mKeyPoseKeeper));
        al::showModelIfHide(this);
    }

    const sead::Quatf& quat = al::getCurrentKeyQuat(mKeyPoseKeeper);
    sead::Vector3f up = sead::Vector3f::ez;
    sead::Vector3f front = sead::Vector3f::ey;
    al::calcQuatFront(&front, quat);
    al::calcQuatUp(&up, quat);

    f32 upOffset = al::calcNerveEaseOutValue(this, 20, -1000.0f, 0.0f);
    f32 frontOffset = al::calcNerveEaseInValue(this, 20, -500.0f, 0.0f);
    sead::Vector3f trans = up * upOffset + mAppearTrans + front * frontOffset;
    al::setTrans(this, trans);

    if (!al::isLessStep(this, 20)) {
        if (mBehaviorType == BehaviorType::RouteDokan) {
            al::setNerve(this, &NrvKoopaLastWallClimbChaseRouteDokan);
        } else {
            al::setNerve(this, &NrvKoopaLastWallClimbLand);
        }
    }
}

/** @brief Lands on the wall and waits LandWaitTime frames before climbing. */
void KoopaLastWallClimb::exeLand() {
    if (al::isFirstStep(this)) {
        const char* actionName = mBehaviorType == BehaviorType::RouteDokan ?
                                     "RouteDokanClimbStart" :
                                 mBehaviorType == BehaviorType::DoubleCherry ?
                                     "DoubleCherryClimbStart" :
                                     "ClimbStart";
        al::startAction(this, actionName);
    }

    if (mBehaviorType != BehaviorType::RouteDokan &&
        mBehaviorType != BehaviorType::DoubleCherry &&
        al::isActionPlaying(this, "ClimbStart") && al::isActionEnd(this)) {
        al::startAction(this, "ClimbWait");
    }

    if (!al::isLessStep(this, mLandWaitTime)) {
        al::setNerve(this, &NrvKoopaLastWallClimbClimb);
    }
}

/** @brief Climbs towards the next key pose and picks the next behavior at the last one. */
void KoopaLastWallClimb::exeClimb() {
    if (al::isFirstStep(this)) {
        al::tryGetArg(&mMoveSpeed, al::getCurrentKeyPlacementInfo(mKeyPoseKeeper), "MoveSpeed");

        const char* actionName;
        if (mBehaviorType == BehaviorType::RouteDokan) {
            actionName = "RouteDokanClimb";
        } else if (mBehaviorType == BehaviorType::DoubleCherry) {
            actionName = "DoubleCherryClimb";
        } else {
            actionName = mMoveSpeed <= 6.0f ? "ClimbSlow" : "Climb";
        }

        al::tryStartActionIfNotPlaying(this, actionName);

        mMoveDistance = 0.0f;
        mKeyDistance = al::calcDistanceNextKeyTrans(mKeyPoseKeeper);
        if (al::isNearZero(mKeyDistance, 0.001f)) {
            mKeyDistance = 1.0f;
        }
    }

    al::calcSlerpKeyQuat(al::getQuatPtr(this), mKeyPoseKeeper, 1.0f);
    mMoveDistance = mMoveSpeed + mMoveDistance;
    f32 rate = mMoveDistance / mKeyDistance;
    f32 lerpRate = rate < 1.0f ? rate : 1.0f;
    al::calcLerpKeyTrans(al::getTransPtr(this), mKeyPoseKeeper, lerpRate);
    KoopaLastFunction::explosionCollision(this, "Body");

    if (rate < 1.0f) {
        return;
    }

    mKeyPoseKeeper->next();
    if (mKeyPoseKeeper->isLastKey()) {
        switch (mBehaviorType) {
        case BehaviorType::Hide:
            al::setNerve(this, &NrvKoopaLastWallClimbHide);
            break;
        case BehaviorType::Jump:
        case BehaviorType::DoubleCherry:
            al::setNerve(this, &NrvKoopaLastWallClimbJumpWait);
            break;
        case BehaviorType::RouteDokan:
            al::setNerve(this, &NrvKoopaLastWallClimbClimbEnd);
            break;
        case BehaviorType::Top:
            al::setNerve(this, &NrvKoopaLastWallClimbTop);
            break;
        case BehaviorType::Attack:
            al::setNerve(this, &NrvKoopaLastWallClimbAttack);
            break;
        default:
            break;
        }
    } else {
        al::setNerve(this, &NrvKoopaLastWallClimbWait);
    }
}

/** @brief Waits on the current key pose for its WaitTime before climbing on. */
void KoopaLastWallClimb::exeWait() {
    if (al::isFirstStep(this)) {
        al::tryGetArg(&mWaitTime, al::getCurrentKeyPlacementInfo(mKeyPoseKeeper), "WaitTime");
    }

    if (al::isGreaterEqualStep(this, mWaitTime)) {
        al::setNerve(this, &NrvKoopaLastWallClimbClimb);
    }
}

/** @brief Plays the climb-end animation, then settles into the end wait. */
void KoopaLastWallClimb::exeClimbEnd() {
    if (al::isFirstStep(this)) {
        const char* actionName = mBehaviorType == BehaviorType::RouteDokan ?
                                     "RouteDokanClimbEnd" :
                                 mBehaviorType == BehaviorType::DoubleCherry ?
                                     "DoubleCherryClimbEnd" :
                                     "ClimbEnd";
        al::startAction(this, actionName);
    }

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvKoopaLastWallClimbEnd);
    }
}

/** @brief Idles at the top of the wall. */
void KoopaLastWallClimb::exeEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(this, mBehaviorType == BehaviorType::RouteDokan ?
                                  "RouteDokanClimbEndWait" :
                                  "ClimbWait");
    }
}

/** @brief Plays the climb-end animation before jumping off the wall. */
void KoopaLastWallClimb::exeJumpWait() {
    if (al::isFirstStep(this)) {
        const char* actionName = mBehaviorType == BehaviorType::RouteDokan ?
                                     "RouteDokanClimbEnd" :
                                 mBehaviorType == BehaviorType::DoubleCherry ?
                                     "DoubleCherryClimbEnd" :
                                     "ClimbEnd";
        al::startAction(this, actionName);
    }

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvKoopaLastWallClimbJumpStart);
    }
}

/** @brief Plays the jump take-off animation, then starts the jump. */
void KoopaLastWallClimb::exeJumpStart() {
    if (al::isFirstStep(this)) {
        al::startAction(this, mBehaviorType == BehaviorType::DoubleCherry ?
                                  "DoubleCherryClimbJumpStart" :
                                  "ClimbJumpStart");
    }

    if (al::isActionEnd(this)) {
        al::invalidateClipping(this);
        al::setNerve(this, &NrvKoopaLastWallClimbJump);
    }
}

/** @brief Jumps backwards off the wall and hides once the jump is over. */
void KoopaLastWallClimb::exeJump() {
    if (al::isFirstStep(this)) {
        al::startAction(this, mBehaviorType == BehaviorType::DoubleCherry ?
                                  "DoubleCherryClimbJumpLoop" :
                                  "ClimbJumpLoop");
        mJumpVelocity = {0.0f, 30.0f, 80.0f};
    }

    const sead::Quatf& quat = al::getCurrentKeyQuat(mKeyPoseKeeper);
    sead::Vector3f up = sead::Vector3f::ez;
    sead::Vector3f front = sead::Vector3f::ey;
    al::calcQuatFront(&front, quat);
    al::calcQuatUp(&up, quat);
    sead::Vector3f trans = al::getTrans(this) - front * mJumpVelocity.y + up * mJumpVelocity.z;
    al::setTrans(this, trans);

    mJumpVelocity += {0.0f, -2.5f, -0.01f};
    if (mJumpVelocity.y < 0.0f) {
        al::invalidateHitSensors(this);
    }

    KoopaLastFunction::explosionCollision(this, "Body");

    if (!al::isLessStep(this, 45)) {
        al::setNerve(this, &NrvKoopaLastWallClimbHide);
    }
}

/** @brief Runs the breath fire attack state. */
void KoopaLastWallClimb::exeAttack() {
    al::updateNerveState(this);
}

/** @brief Runs the summit wait state. */
void KoopaLastWallClimb::exeTop() {
    al::updateNerveState(this);
}

/** @brief Plays the route-pipe climbing demo, then disappears. */
void KoopaLastWallClimb::exeChaseRouteDokan() {
    if (al::isFirstStep(this)) {
        al::invalidateClipping(this);
        al::startAction(this, "DemoRouteDokanClimb");
    }

    if (al::isActionEnd(this)) {
        kill();
    }
}

/** @brief Gets knocked away from the wall by a POW block. */
void KoopaLastWallClimb::exePowBlockDamage() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "PowBlockDamageStart");
        mDamageVelocity = {0.0f, 20.0f, 0.0f};
    }

    const sead::Quatf& quat = al::getCurrentKeyQuat(mKeyPoseKeeper);
    sead::Vector3f up = sead::Vector3f::ez;
    sead::Vector3f front = sead::Vector3f::ey;
    al::calcQuatFront(&front, quat);
    al::calcQuatUp(&up, quat);
    sead::Vector3f trans =
        al::getTrans(this) - front * mDamageVelocity.y + up * mDamageVelocity.z;
    al::setTrans(this, trans);
    mDamageVelocity += {0.0f, 0.0f, -2.0f};

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvKoopaLastWallClimbPowBlockDamageFall);
    }
}

/** @brief Falls away after a POW block hit and disappears. */
void KoopaLastWallClimb::exePowBlockDamageFall() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "PowBlockDamageLoop");
    }

    const sead::Quatf& quat = al::getCurrentKeyQuat(mKeyPoseKeeper);
    sead::Vector3f up = sead::Vector3f::ez;
    sead::Vector3f front = sead::Vector3f::ey;
    al::calcQuatFront(&front, quat);
    al::calcQuatUp(&up, quat);
    sead::Vector3f trans =
        al::getTrans(this) - front * mDamageVelocity.y + up * mDamageVelocity.z;
    al::setTrans(this, trans);
    mDamageVelocity += {0.0f, 0.0f, -2.0f};

    if (!al::isLessStep(this, 60)) {
        kill();
    }
}

/** @brief Plays the final POW defeat, clears any breath fire and disappears. */
void KoopaLastWallClimb::exeLastPowDamage() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "DownLastDummy");
        if (mAttackState != nullptr) {
            mAttackState->killAllFire();
        }
    }

    if (!al::isLessStep(this, 60)) {
        kill();
    }
}

/** @brief Hides behind the wall for WaitTime frames, then advances to the next key pose. */
void KoopaLastWallClimb::exeHide() {
    if (al::isFirstStep(this)) {
        mWaitTime = 0;
        al::tryGetArg(&mWaitTime, al::getCurrentKeyPlacementInfo(mKeyPoseKeeper), "WaitTime");
        al::startAction(this, mBehaviorType == BehaviorType::RouteDokan ?
                                  "RouteDokanClimbEndWait" :
                                  "ClimbWait");
    }

    if (al::isStep(this, mWaitTime)) {
        mKeyPoseKeeper->next();
        al::hideModelIfShow(this);
        al::validateClipping(this);
        kill();
    }
}
