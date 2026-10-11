#include "Enemy/ActorJointLookController.hpp"
#include "Library/Joint/JointControllerKeeper.hpp"
#include "Library/LiveActor/Util/ActorMovementUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Util/PlayerUtil.hpp"

#include <float.h>
#include <math/seadMathCalcCommon.h>

/** @brief Constructs the default joint parameters. */
ActorJointLookControllerParam::ActorJointLookControllerParam()
    : mSpeed(2.0f), mRange(-40.0f, 40.0f), mIsVertical(false), mBaseMtx(nullptr),
      mAxisMtx(nullptr) {}

/** @brief Constructs joint parameters.
 * @param speed Rotation speed in degrees per frame.
 * @param rRange Minimum and maximum rotation in degrees.
 * @param isVertical Whether the joint follows the vertical angle instead of the horizontal one.
 * @param pBaseMtx Matrix whose translation is used as the look origin, or nullptr.
 * @param pAxisMtx Matrix whose axes are used as the look frame, or nullptr.
 */
ActorJointLookControllerParam::ActorJointLookControllerParam(f32 speed, const sead::Vector2f& rRange,
                                                             bool isVertical,
                                                             const sead::Matrix34f* pBaseMtx,
                                                             const sead::Matrix34f* pAxisMtx)
    : mSpeed(speed), mRange(rRange), mIsVertical(isVertical), mBaseMtx(pBaseMtx),
      mAxisMtx(pAxisMtx) {}

/** @brief Constructs the controller.
 * @param pActor Actor owning the joints.
 * @param jointNumMax Maximum number of joints.
 */
ActorJointLookController::ActorJointLookController(const al::LiveActor* pActor, s32 jointNumMax)
    : mActor(pActor), mTargetPlayer(nullptr), mSearchTimer(0), mIsLook(false),
      mLookTarget(0.0f, 0.0f, 0.0f), mLimitH(180.0f), mLimitV(180.0f) {
    mDegrees = new f32[jointNumMax];
    for (s32 i = 0; i < jointNumMax; i++) {
        mDegrees[i] = 0.0f;
    }

    mParams.allocBuffer(jointNumMax, nullptr);
}

/** @brief Registers a joint rotated by this controller.
 * @param pJointName Joint name.
 * @param rAxis Local rotation axis.
 * @param pParam Joint parameters.
 */
void ActorJointLookController::appendJoint(const char* pJointName, const sead::Vector3f& rAxis,
                                           const ActorJointLookControllerParam* pParam) {
    al::initJointLocalAxisRotator(mActor, rAxis, &mDegrees[mParams.size()], pJointName);
    mParams.pushBack(pParam);
}

/** @brief Moves every joint toward the look target, or back to rest when out of range. */
void ActorJointLookController::update() {
    if (!mIsLook) {
        for (s32 i = 0; i < mParams.size(); i++) {
            mDegrees[i] = al::converge(mDegrees[i], 0.0f, mParams.unsafeAt(i)->mSpeed);
        }

        return;
    }

    f32 angleH = al::calcAngleToTargetH(mActor, mLookTarget);
    f32 angleV = al::calcAngleToTargetV(mActor, mLookTarget);
    if (mLimitH < sead::Mathf::abs(angleH) || mLimitV < sead::Mathf::abs(angleV)) {
        for (s32 i = 0; i < mParams.size(); i++) {
            mDegrees[i] = al::converge(mDegrees[i], 0.0f, mParams.unsafeAt(i)->mSpeed);
        }

        return;
    }

    sead::Vector3f front = {0.0f, 0.0f, 0.0f};
    sead::Vector3f side = {0.0f, 0.0f, 0.0f};
    sead::Vector3f up = {0.0f, 0.0f, 0.0f};
    al::calcFrontDir(&front, mActor);
    al::calcSideDir(&side, mActor);
    al::calcUpDir(&up, mActor);

    for (s32 i = 0; i < mParams.size(); i++) {
        sead::Vector3f jointFront = front;
        sead::Vector3f jointSide = side;
        sead::Vector3f jointUp = up;
        if (mParams.unsafeAt(i)->mAxisMtx != nullptr) {
            mParams.unsafeAt(i)->mAxisMtx->getBase(jointSide, 0);
            mParams.unsafeAt(i)->mAxisMtx->getBase(jointUp, 1);
            mParams.unsafeAt(i)->mAxisMtx->getBase(jointFront, 2);
        }

        f32 jointH = angleH;
        f32 jointV = angleV;
        if (mParams.unsafeAt(i)->mBaseMtx != nullptr) {
            sead::Vector3f dir = {0.0f, 0.0f, 0.0f};
            dir.setSub(mLookTarget, mParams.unsafeAt(i)->mBaseMtx->getTranslation());
            jointH = 0.0f;
            jointV = 0.0f;
            if (!al::normalizeOrZero(&dir)) {
                jointH = al::calcAngleOnPlaneDegree(jointFront, dir, jointUp);
                jointV = al::calcAngleOnPlaneDegree(jointFront, dir, jointSide);
            }
        }

        const ActorJointLookControllerParam* pParam = mParams.unsafeAt(i);
        f32 clampH = sead::Mathf::clamp(jointH, pParam->mRange.x, pParam->mRange.y);
        f32 clampV = sead::Mathf::clamp(jointV, pParam->mRange.x, pParam->mRange.y);
        f32 target = pParam->mIsVertical ? clampV : clampH;
        mDegrees[i] = al::converge(mDegrees[i], target, pParam->mSpeed);
    }
}

/** @brief Periodically picks the nearest player within the look limits as the look target.
 * @param distance Maximum search distance; negative means unlimited.
 */
void ActorJointLookController::setLookAtNearestPlayer(f32 distance) {
    mSearchTimer--;
    if (mSearchTimer <= 0 || mTargetPlayer == nullptr ||
        rc::isPlayerDeadOrBubble(mTargetPlayer)) {
        const al::LiveActor* pActor = mActor;
        mSearchTimer = 90;

        al::LiveActor* pNearest = nullptr;
        f32 minDistanceSq = FLT_MAX;
        for (s32 i = 0; i < al::getPlayerNumMax(pActor); i++) {
            al::LiveActor* pPlayer = al::tryGetPlayerActor(pActor, i);
            if (pPlayer == nullptr || rc::isPlayerDeadOrBubble(pPlayer)) {
                continue;
            }

            f32 distanceSq = (al::getTrans(pPlayer) - al::getTrans(pActor)).squaredLength();
            if (distanceSq < minDistanceSq) {
                f32 angleH = al::calcAngleToTargetH(pActor, al::getTrans(pPlayer));
                f32 angleV = al::calcAngleToTargetV(pActor, al::getTrans(pPlayer));
                if (!(mLimitH < sead::Mathf::abs(angleH)) &&
                    !(mLimitV < sead::Mathf::abs(angleV))) {
                    minDistanceSq = distanceSq;
                    pNearest = pPlayer;
                }
            }
        }

        if (!(minDistanceSq <= distance * distance) && !(distance < 0.0f)) {
            pNearest = nullptr;
        }

        mTargetPlayer = pNearest;
    }

    if (mTargetPlayer != nullptr) {
        mLookTarget.set(al::getTrans(mTargetPlayer));
        mIsLook = true;
    } else {
        mTargetPlayer = nullptr;
        mIsLook = false;
    }
}

/** @brief Resets every joint rotation to zero.
 * @param isStopLook Whether to also stop looking at the current target.
 */
void ActorJointLookController::resetRotate(bool isStopLook) {
    for (s32 i = 0; i < mParams.size(); i++) {
        mDegrees[i] = 0.0f;
    }

    if (isStopLook) {
        mIsLook = false;
    }
}
