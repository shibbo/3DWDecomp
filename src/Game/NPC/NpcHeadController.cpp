#include "NPC/NpcHeadController.hpp"
#include "NPC/NpcTargetFinder.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/Joint/JointControllerKeeper.hpp"
#include "Project/Base/StringUtil.hpp"
/** @brief Stores one joint tracking configuration. @param pJointName Joint name. @param rate
 * Rotation interpolation rate. @param rYaw Yaw limits. @param rPitch Pitch limits. @param rFront
 * Local forward direction. @param rUp Local up direction. */
NpcHeadControllerParam::NpcHeadControllerParam(const char* pJointName, float rate,
                                               const sead::Vector2f& rYaw,
                                               const sead::Vector2f& rPitch,
                                               const sead::Vector3f& rFront,
                                               const sead::Vector3f& rUp)
    : mJointName(pJointName), mRate(rate) {
    mYaw.x = rYaw.x;
    mYaw.y = rYaw.y;
    mPitch.x = rPitch.x;
    mPitch.y = rPitch.y;
    mFront.x = rFront.x;
    mFront.y = rFront.y;
    mFront.z = rFront.z;
    mUp.x = rUp.x;
    mUp.y = rUp.y;
    mUp.z = rUp.z;
}
/** @brief Creates action-dependent head tracking. @param pActor Host actor. @param pFirst First
 * tracked joint. @param pSecond Second tracked joint. @param maxActions Action capacity. @param
 * pFinder Target finder. @param type Accepted target categories. @param keepFrames Frames to retain
 * target rotation. @param rOffset Target position offset. */
NpcHeadController::NpcHeadController(al::LiveActor* pActor, const NpcHeadControllerParam* pFirst,
                                     const NpcHeadControllerParam* pSecond, int maxActions,
                                     NpcTargetFinder* pFinder, npc::NpcFindTargetType type,
                                     int keepFrames, const sead::Vector3f& rOffset)
    : mActor(pActor), mAction(nullptr), mFinder(pFinder), mType(type), mTarget(nullptr),
      mLook(nullptr), mUnknown40(nullptr), mKeepTimer(0), mTargetPos(sead::Vector3f::zero),
      mFirst(nullptr), mSecond(nullptr), mKeepFrames(keepFrames), mOffset(rOffset),
      mSkipFrame(false) {
    if (!al::isExistJointControllerKeeper(mActor)) {
        al::initJointControllerKeeper(mActor, 1);
    }
    mLook = al::initJointLookAtController(mActor, 2);
    int index = 0;
    if (pFirst && pFirst->mJointName) {
        al::appendJointLookAtController(mLook, mActor, pFirst->mJointName, pFirst->mRate,
                                        pFirst->mYaw, pFirst->mPitch, pFirst->mFront, pFirst->mUp);
        mFirst = mLook->getJointInfo(index++);
    }
    if (pSecond && pSecond->mJointName) {
        al::appendJointLookAtController(mLook, mActor, pSecond->mJointName, pSecond->mRate,
                                        pSecond->mYaw, pSecond->mPitch, pSecond->mFront,
                                        pSecond->mUp);
        mSecond = mLook->getJointInfo(index);
    }
    mActions.allocBuffer(maxActions, nullptr);
}
/** @brief Updates joint targeting and retains the last rotation for its grace period. */
void NpcHeadController::update() {
    if (validateLookAt() && (mAction->mFirst || mAction->mSecond)) {
        mLook->validAllJoint();
        if (mFirst && !mAction->mFirst) {
            mLook->invalidJoint(mFirst->jointIndex);
        }
        if (mSecond && !mAction->mSecond) {
            mLook->invalidJoint(mSecond->jointIndex);
        }
        mLook->requestJointLookAt(mTargetPos + mOffset);
        if ((mFirst && !mFirst->isInvalid && mFirst->isLookAt) ||
            (mSecond && !mSecond->isInvalid && mSecond->isLookAt)) {
            mLook->setKeepTargetQuat(true);
            mKeepTimer = mKeepFrames;
        } else {
            --mKeepTimer;
            if (mKeepTimer >= 0) {
                mLook->setKeepTargetQuat(true);
            } else {
                mLook->setKeepTargetQuat(false);
            }
        }
    } else {
        mLook->setKeepTargetQuat(false);
    }
    mLook->update();
}
/** @brief Resolves the current action and target position. @return Whether tracking is available
 * this frame. */
bool NpcHeadController::validateLookAt() {
    if (mSkipFrame) {
        mSkipFrame = false;
        return false;
    }
    if (!al::isExistAction(mActor)) {
        return false;
    }
    trySetActionParam(al::getActionName(mActor));
    if (!mAction) {
        return false;
    }
    if (mTarget) {
        mTargetPos.x = mTarget->x;
        mTargetPos.y = mTarget->y;
        mTargetPos.z = mTarget->z;
        return true;
    }
    if (!mFinder) {
        return false;
    }
    if ((static_cast<unsigned>(mFinder->mType) & static_cast<unsigned>(mType)) == 0) {
        return false;
    }
    if (mFinder->mHasTargetChanged) {
        mKeepTimer = mKeepFrames;
    }
    const sead::Vector3f& rTarget = mFinder->getTargetPos();
    mTargetPos.x = rTarget.x;
    mTargetPos.y = rTarget.y;
    mTargetPos.z = rTarget.z;
    return true;
}
/** @brief Adds a unique action configuration. @param pAction Action name. @param first Enable first
 * joint. @param second Enable second joint. */
void NpcHeadController::addAction(const char* pAction, bool first, bool second) {
    for (int i = 0; i < mActions.size(); ++i) {
        if (al::isEqualString(mActions.unsafeAt(i)->mName, pAction)) {
            return;
        }
    }
    mActions.pushBack(new ActionParam(pAction, first, second));
}
/** @brief Selects the configuration for an action. @param pAction Action name, or nullptr to clear.
 */
void NpcHeadController::trySetActionParam(const char* pAction) {
    if (pAction) {
        if (mAction && al::isEqualString(mAction->mName, pAction)) {
            return;
        }
        for (int i = 0; i < mActions.size(); ++i) {
            if (al::isEqualString(mActions.unsafeAt(i)->mName, pAction)) {
                mAction = mActions[i];
                return;
            }
        }
    }
    mAction = nullptr;
}
