#include "Player/Normal/PlayerCollider.hpp"

#include <attributes.h>
#include <cmath>
#include <math/seadMathCalcCommon.h>
#include <math/seadQuat.h>
#include <prim/seadSafeString.h>

#include "Library/Collision/CollisionMultiSphere.hpp"
#include "Library/Collision/CollisionPartsKeeperUtil.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Player/IUseCollisionPartsMtx.hpp"
#include "Player/IUsePlayerAnimator.hpp"
#include "Player/IUsePlayerCollisionCheckArrow.hpp"
#include "Player/IUsePlayerCollisionCheckSphere.hpp"
#include "Player/IUsePlayerInput.hpp"
#include "Player/Normal/PlayerConstParam.hpp"
#include "Player/Normal/PlayerGroundFollower.hpp"
#include "Player/Normal/PlayerProperty.hpp"
#include "Player/Normal/PlayerSnapWallInfo.hpp"
#include "Player/PlayerCollisionFunc.hpp"
#include "Project/AreaObj/AreaObj.hpp"
#include "Project/AreaObj/AreaObjUtil.hpp"
#include "Project/AreaObj/AreaShape.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Collision/CollisionParts.hpp"
#include "Project/Collision/CollisionUtil.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/PlayerUtil.hpp"

namespace {
/// The extra spheres: [0] bounds the others, [1] and up follow the climbing animation.
const al::CollisionMultiSphereBase::Sphere cExSphereTable[3] = {
    {{0.0f, 50.0f, 50.0f}, 40.0f},
    {{0.0f, 50.0f, 50.0f}, 40.0f},
    {{0.0f, 50.0f, -30.0f}, 40.0f},
};

/// What the collider recorded about one side this frame: the deepest hit.
class PlayerCollisionInfo : public PlayerCollisionInfoBase {
public:
    /**
     * Forgets the recorded hit.
     */
    void clear() override { mIsValid = false; }

    /**
     * Records a hit if nothing was recorded yet or it is deeper than the recorded one.
     * @param rNormal the hit's normal
     * @param depth how deep the player is in the hit
     * @param pParts the collision parts that was hit
     * @param pMapCode the hit's map code
     * @param pWallCode the hit's wall code
     * @param pMaterialCode the hit's material code
     */
    void record(const sead::Vector3f& rNormal, f32 depth, const al::CollisionParts* pParts,
                const char* pMapCode, const char* pWallCode, const char* pMaterialCode) override {
        if (!mIsValid || mDepth < depth) {
            mNormal.set(rNormal);
            mDepth = depth;
            mParts = pParts;
            mMapCode = pMapCode;
            mWallCode = pWallCode;
            mMaterialCode = pMaterialCode;
            mIsValid = true;
        }
    }

    /**
     * @return whether a hit was recorded
     */
    bool isValid() const override { return mIsValid; }

    /**
     * @return the recorded hit's normal
     */
    const sead::Vector3f& getNormal() const override { return mNormal; }

    /**
     * @return how deep the player is in the recorded hit
     */
    f32 getDepth() const override { return mDepth; }

    /**
     * @return the recorded hit's collision parts
     */
    const al::CollisionParts* getCollisionParts() const override { return mParts; }

    /**
     * @return the recorded hit's map code
     */
    const char* getMapCodeName() const override { return mMapCode; }

    /**
     * @return the recorded hit's wall code
     */
    const char* getWallCodeName() const override { return mWallCode; }

    /**
     * @return the recorded hit's material code
     */
    const char* getMaterialCodeName() const override { return mMaterialCode; }

private:
    bool mIsValid = false;                       // 0x8
    sead::Vector3f mNormal = {0.0f, 0.0f, 0.0f};  // 0xc
    f32 mDepth = 0.0f;                           // 0x18
    const al::CollisionParts* mParts = nullptr;  // 0x20
    const char* mMapCode = nullptr;              // 0x28
    const char* mWallCode = nullptr;             // 0x30
    const char* mMaterialCode = nullptr;         // 0x38
};

/// The depths the legs hit this frame, with the collision parts they hit.
struct LegHitList {
    /**
     * Adds a hit.
     * @param depth how deep the leg is in the hit
     * @param pParts the collision parts that was hit
     */
    void add(f32 depth, const al::CollisionParts* pParts) {
        mDepths[mNum] = depth;
        mParts[mNum] = pParts;
        mNum++;
    }

    /**
     * Averages the depths if every hit is on the same (or a moving) parts, else takes the deepest.
     * @return the depth to push the player up by
     */
    f32 calcDepth() const {
        if (mNum == 0) {
            return 0.0f;
        }

        bool isSameParts = true;

        for (u32 i = 1; i < mNum; i++) {
            if (mParts[0] != mParts[i]) {
                isSameParts = false;
                break;
            }
        }

        if (!isSameParts) {
            for (u32 i = 0; i < mNum; i++) {
                if (mParts[i]->_154 == 0) {
                    f32 depth = mDepths[0];

                    for (u32 j = 1; j < mNum; j++) {
                        if (depth < mDepths[j]) {
                            depth = mDepths[j];
                        }
                    }

                    return depth;
                }
            }
        }

        f32 sum = 0.0f;

        for (u32 i = 0; i < mNum; i++) {
            sum += mDepths[i];
        }

        return (1.0f / mNum) * sum;
    }

    LegHitList() : mNum(0) { mParts[0] = nullptr; }

    f32 mDepths[5];                       // 0x0
    const al::CollisionParts* mParts[5];  // 0x18
    u32 mNum;                             // 0x40
};

/**
 * Copies a recorded hit to an info.
 * @param pInfo the info to fill
 * @param pCollisionInfo the recorded hit
 */
inline void copyCollisionInfo(IUsePlayerCollision::Info* pInfo,
                              const PlayerCollisionInfoBase* pCollisionInfo) {
    const sead::Vector3f& normal = pCollisionInfo->getNormal();
    const char* mapCode = pCollisionInfo->getMapCodeName();
    const char* wallCode = pCollisionInfo->getWallCodeName();
    const char* materialCode = pCollisionInfo->getMaterialCodeName();
    const al::CollisionParts* parts = pCollisionInfo->getCollisionParts();
    pInfo->mNormal.set(normal);
    pInfo->mMapCode = mapCode;
    pInfo->mWallCode = wallCode;
    pInfo->mMaterialCode = materialCode;
    pInfo->mParts = parts;
}

/**
 * @param pParts a collision parts
 * @return whether the parts moves on its own (its push is handled separately)
 */
inline bool isMovingParts(const al::CollisionParts* pParts) {
    return pParts != nullptr && pParts->_154 != 0 && pParts->mPriority == -1;
}

void checkSubLegArrow(PlayerCollider* pCollider, const sead::Vector3f& rPos,
                      const sead::Vector3f& rDir, const sead::Vector3f& rLegStart,
                      LegHitList* pHitList, LegHitList* pMovingHitList, f32 legLength,
                      f32 legOffset);
}  // namespace

/**
 * Creates the collision infos, the parts arrays and the extra spheres.
 * @param pActor the player actor
 * @param pCollisionPartsMtx keeps the matrices of the parts the player follows
 * @param pConstParam the player's tuning values
 * @param pAnimator the player's animator
 * @param isLongStep whether the player moves in longer steps (27 instead of 9 units)
 */
PlayerCollider::PlayerCollider(al::LiveActor* pActor, IUseCollisionPartsMtx* pCollisionPartsMtx,
                               const PlayerConstParam* pConstParam,
                               const IUsePlayerAnimator* pAnimator, bool isLongStep)
    : mActor(pActor), mAnimator(pAnimator), mIsLongStep(isLongStep),
      mFloorInfo(new PlayerCollisionInfo), mCeilingInfo(new PlayerCollisionInfo),
      mFrontWallInfo(new PlayerCollisionInfo), mBackWallInfo(new PlayerCollisionInfo),
      mLeftWallInfo(new PlayerCollisionInfo), mRightWallInfo(new PlayerCollisionInfo),
      mFrontWallAnyInfo(new PlayerCollisionInfo),
      mGroundFollower(new PlayerGroundFollower(pCollisionPartsMtx)),
      mCollisionPartsMtx(pCollisionPartsMtx), mPushMin(sead::Vector3f::zero),
      mPushMax(sead::Vector3f::zero), mJumpFollowVel(sead::Vector3f::zero),
      mFollowVel(sead::Vector3f::zero), mFollowRotate(sead::Vector3f::zero),
      mFollowFront(sead::Vector3f::zero), mFloorPartsArray(new CollisionPartsArray()),
      mFrontPartsArray(new CollisionPartsArray()), mCeilingPartsArray(new CollisionPartsArray()),
      mFrontHemispherePartsArray(new CollisionPartsArray()),
      mBackHemispherePartsArray(new CollisionPartsArray()),
      mSnapWallInfo(new PlayerSnapWallInfo()), mPushVec(sead::Vector3f::zero),
      mMovingPushVec(sead::Vector3f::zero), mConstParam(pConstParam) {
    mFloorPartsArray->allocBuffer(16, nullptr);
    mFrontPartsArray->allocBuffer(16, nullptr);
    mCeilingPartsArray->allocBuffer(16, nullptr);
    mFrontHemispherePartsArray->allocBuffer(16, nullptr);
    mBackHemispherePartsArray->allocBuffer(16, nullptr);

    ExSphereInfo* exSphere = new ExSphereInfo;
    exSphere->mSpheres = new Sphere[3];
    exSphere->mAnimName = "ClimbMove";
    mExSphere = exSphere;

    for (s32 i = 0; i < 3; i++) {
        exSphere->mSpheres[i] = cExSphereTable[i];
    }

    exSphere->mSphereNum = 3;

    for (u32 i = 2; i < exSphere->mSphereNum; i++) {
        sead::Vector3f diff = exSphere->mSpheres[i].mPos - exSphere->mSpheres[0].mPos;
        f32 dist = diff.length();

        if (al::isNearZero(dist, 0.001f)) {
            if (exSphere->mSpheres[i].mRadius > exSphere->mSpheres[0].mRadius) {
                exSphere->mSpheres[0].mRadius = exSphere->mSpheres[i].mRadius;
            }
        } else if (dist + exSphere->mSpheres[i].mRadius > exSphere->mSpheres[0].mRadius) {
            sead::Vector3f dir = diff;
            al::normalize(&dir);
            const Sphere& sphere = exSphere->mSpheres[i];
            Sphere& bound = exSphere->mSpheres[0];
            sead::Vector3f far = dir * sphere.mRadius + sphere.mPos;
            sead::Vector3f near = bound.mPos - dir * bound.mRadius;
            sead::Vector3f span = far - near;
            bound.mPos = near + span * 0.5f;
            bound.mRadius = span.length() * 0.5f;
        }
    }
}

/**
 * @param pProperty the player's physical state
 */
void PlayerCollider::setProperty(PlayerProperty* pProperty) {
    mProperty = pProperty;
}

/**
 * @param pFigureDirector the player's figure (form) director
 */
void PlayerCollider::setFigureDirector(const PlayerFigureDirector* pFigureDirector) {
    mSnapWallInfo->setFigureDirector(pFigureDirector);
}

/**
 * Forgets everything touched this frame.
 */
void PlayerCollider::clear() {
    clearCollisionInfo();
}

/**
 * Forgets the recorded hits, the touched parts and the snapped wall.
 */
void PlayerCollider::clearCollisionInfo() {
    mFloorInfo->clear();
    mCeilingInfo->clear();
    mFrontWallInfo->clear();
    mBackWallInfo->clear();
    mLeftWallInfo->clear();
    mRightWallInfo->clear();
    mFrontWallAnyInfo->clear();
    mFloorPartsArray->clear();
    mFrontPartsArray->clear();
    mCeilingPartsArray->clear();
    mFrontHemispherePartsArray->clear();
    mBackHemispherePartsArray->clear();
    mSnapWallInfo->clear();
    mIsHeadOnFrontWall = false;
    mIsCenterOnFloor = false;
}

/**
 * Moves the player by its velocity without checking the map.
 * @param isClear whether to forget what was touched
 */
void PlayerCollider::moveSimple(bool isClear) {
    if (isClear) {
        clearCollisionInfo();
    }

    updateExSphere();
    mJumpFollowVel = {0.0f, 0.0f, 0.0f};
    mFollowVel = {0.0f, 0.0f, 0.0f};
    mProperty->mTrans = mProperty->mTrans + mProperty->mVelocity;
    clearExPush();
}

/**
 * Grows the extra spheres out over five frames while their animation plays.
 */
void PlayerCollider::updateExSphere() {
    if (!mIsValidExSphere ||
        !mAnimator->isAnim(sead::SafeString(mExSphere->mAnimName))) {
        mExSphereFrame = 0;
        return;
    }

    s32 sphereNum = mExSphere->mSphereNum;
    f32 rate = sead::Mathf::clampMax(mExSphereFrame / 5.0f, 1.0f);

    if (sphereNum >= 2) {
        Sphere& sphere = mExSphere->mSpheres[1];
        sphere.mPos.x = rate * cExSphereTable[1].mPos.x;
        sphere.mPos.z = rate * cExSphereTable[1].mPos.z;

        for (s32 i = 2; i < sphereNum; i++) {
            Sphere& exSphere = mExSphere->mSpheres[i];
            exSphere.mPos.x = rate * cExSphereTable[i].mPos.x;
            exSphere.mPos.z = rate * cExSphereTable[i].mPos.z;
        }
    }

    mExSphereFrame++;
}

/**
 * Forgets the pushes from outside.
 */
void PlayerCollider::clearExPush() {
    mPushMin = {0.0f, 0.0f, 0.0f};
    mPushMax = {0.0f, 0.0f, 0.0f};
}

/**
 * Moves the player in the air, then damps the velocity inherited from the ground.
 */
void PlayerCollider::solveAir() {
    clearCollisionInfo();
    updateExSphere();
    mFollowVel = {0.0f, 0.0f, 0.0f};

    if (rc::isInAreaObj(mActor, rc::AreaObjType::PlayerNoAirFollowArea)) {
        mJumpFollowVel = {0.0f, 0.0f, 0.0f};
    }

    sead::Vector3f vel = mProperty->mVelocity + mJumpFollowVel + mPushMin + mPushMax;
    applyVelocity(vel, 0.0f, false);

    sead::Vector3f velDir;
    al::verticalizeVec(&velDir, mProperty->mUpDir, mProperty->mVelocity);

    if (al::normalizeOrZero(&velDir)) {
        velDir.set(mJumpFollowVel);
        al::normalizeOrZero(&velDir);
    }

    sead::Vector3f followDir = mJumpFollowVel;
    al::normalizeOrZero(&followDir);
    f32 rate = (followDir.dot(velDir) + 1.0f) * 0.5f;
    f32 damper = mConstParam->getFollowFrontDamper() * rate +
                 (1.0f - rate) * mConstParam->getFollowBackDamper();
    mJumpFollowVel *= damper;
    clearPush();
}

/**
 * Moves the player in steps, pushing it out of the map after each one.
 * @param rVel the movement this frame
 * @param legOffset how far below the leg the player still counts as standing
 * @param isSkipLeg whether to skip the leg (floor) check
 */
void PlayerCollider::applyVelocity(const sead::Vector3f& rVel, f32 legOffset, bool isSkipLeg) {
    if (mGrowTimer != 0) {
        mGrowTimer--;
    }

    if (mHeadLowerTimer != 0) {
        mHeadLowerTimer--;
    }

    sead::Vector3f trans = mProperty->mTrans;
    bool isSwimSlow = false;

    if (rc::isInWaterArea(mActor, mProperty->mTrans)) {
        isSwimSlow = mProperty->mFront.dot(mProperty->mVelocity) < 5.0f;
    }

    f32 stepLength = mIsLongStep ? 27.0f : 9.0f;

    if (rc::isUsingOldPlayerParams()) {
        stepLength = 27.0f;
    }

    u32 stepNum = static_cast<u32>(rVel.length() / stepLength) + 1;
    sead::Vector3f step = rVel;
    step *= 1.0f / stepNum;

    sead::Vector3f legDir = mProperty->mGroundUp;
    legDir *= -(PlayerCollisionFunc::calcTall(mProperty, mConstParam) * 0.42f + legOffset +
                mProperty->_78 * 10.0f);

    sead::Vector3f side;
    side.setCross(mProperty->mFront, mProperty->mGroundUp);
    al::normalize(&side);

    for (u32 i = 0; i < stepNum; i++) {
        applyVelocityCore(trans, step, false, legDir, legOffset, side, isSkipLeg, isSwimSlow);

        if (mPushVec.squaredLength() > stepLength * stepLength) {
            u32 subStepNum = static_cast<u32>(mPushVec.length() / stepLength) + 1;
            sead::Vector3f subStep = mPushVec;
            subStep *= 1.0f / subStepNum;

            for (u32 j = 0; j < subStepNum; j++) {
                applyVelocityCore(trans, subStep, false, legDir, legOffset, side, isSkipLeg,
                                  isSwimSlow);
                trans += mMovingPushVec;
            }
        } else {
            trans += mPushVec;
        }
    }

    mProperty->mTrans = trans;
    keepOutRestrictedArea();
}

/**
 * Moves the player in the air without following anything (no floor check).
 */
void PlayerCollider::solveAirNoFloor() {
    clearCollisionInfo();
    updateExSphere();
    mFollowVel = {0.0f, 0.0f, 0.0f};

    if (rc::isInAreaObj(mActor, rc::AreaObjType::PlayerNoAirFollowArea)) {
        mJumpFollowVel = {0.0f, 0.0f, 0.0f};
    }

    sead::Vector3f vel = mProperty->mVelocity + mJumpFollowVel + mPushMin + mPushMax;
    applyVelocity(vel, 0.0f, true);

    sead::Vector3f velDir;
    al::verticalizeVec(&velDir, mProperty->mUpDir, mProperty->mVelocity);

    if (al::normalizeOrZero(&velDir)) {
        velDir.set(mJumpFollowVel);
        al::normalizeOrZero(&velDir);
    }

    sead::Vector3f followDir = mJumpFollowVel;
    al::normalizeOrZero(&followDir);
    f32 rate = (followDir.dot(velDir) + 1.0f) * 0.5f;
    f32 damper = mConstParam->getFollowFrontDamper() * rate +
                 (1.0f - rate) * mConstParam->getFollowBackDamper();
    mJumpFollowVel *= damper;
    clearPush();
}

/**
 * Moves the player on the ground, following the floor's collision parts.
 */
void PlayerCollider::snapGround() {
    sead::Vector3f followVec = {0.0f, 0.0f, 0.0f};

    if (isOnFloor() && !al::isEqualString(mFloorInfo->getWallCodeName(), "NoAction")) {
        PlayerProperty* property = mProperty;
        sead::Vector3f prevTrans = property->mTrans;
        mGroundFollower->followGround(property, &mFollowVel, &mFollowRotate, &mFollowFront,
                                      mFloorInfo->getCollisionParts());
        sead::Vector3f trans = mProperty->mTrans;

        if (!isForceFollowing()) {
            followVec = trans - prevTrans;

            if (followVec.length() < 200.0f) {
                mProperty->mTrans = prevTrans;
            } else {
                f32 length = followVec.length();

                if (length > 0.0f) {
                    followVec *= 200.0f / length;
                }

                mProperty->mTrans -= followVec;
            }
        }
    }

    clearCollisionInfo();
    updateExSphere();
    sead::Vector3f vel = followVec + (mProperty->mVelocity + mPushMin + mPushMax);
    applyVelocity(vel, mProperty->_78 * 30.0f, false);
    clearExPush();
}

/**
 * @return whether the player must move with the floor (balance trucks, bone roller coasters)
 */
bool PlayerCollider::isForceFollowing() const {
    if (rc::isInAreaObj(mActor, rc::AreaObjType::BugFixBalanceTruckArea, mProperty->mTrans) &&
        mFloorInfo->getCollisionParts() != nullptr &&
        mFloorInfo->getCollisionParts()->getSensor() != nullptr &&
        al::isSensorHostName(mFloorInfo->getCollisionParts()->getSensor(), "BalanceTruck")) {
        return true;
    }

    if (mFloorInfo->getCollisionParts() != nullptr &&
        mFloorInfo->getCollisionParts()->getSensor() != nullptr &&
        al::isSensorHostName(mFloorInfo->getCollisionParts()->getSensor(),
                             "BoneRollerCoasterParts")) {
        return true;
    }

    return false;
}

/**
 * Moves the player while it clings to a wall, keeping it snapped to the wall.
 * @param isBack whether the wall is behind the player
 */
void PlayerCollider::snapWall(bool isBack) {
    sead::Vector3f followVec = {0.0f, 0.0f, 0.0f};

    if (isBack) {
        if (isOnBackWall()) {
            calcFollowVec(&followVec, mProperty->mTrans, mBackWallInfo->getCollisionParts());
        }
    } else if (isOnFrontWall()) {
        calcFollowVec(&followVec, mProperty->mTrans, mFrontWallInfo->getCollisionParts());
    }

    sead::Vector3f vel = mProperty->mVelocity + mPushMin + mPushMax;
    clearCollisionInfo();
    updateExSphere();
    mFollowVel = {0.0f, 0.0f, 0.0f};
    applyVelocity(vel + followVec, 0.0f, false);
    clearExPush();

    f32 snapLength;
    sead::Vector3f checkPos;
    sead::Vector3f checkDir;

    if (isBack) {
        if (isOnBackWall() && mSnapWallInfo->isExist()) {
            return;
        }

        mSnapWallInfo->clear();
        PlayerProperty* property = mProperty;
        PlayerCollisionFunc::calcWallCheckBodyPos(
            &checkPos, property->mTrans, property->mGroundUp,
            PlayerCollisionFunc::calcTall(property, mConstParam));
        checkDir = -mProperty->mFront * mConstParam->getWallSnapDistance();
        checkSnapWall(checkPos, checkDir, true);

        if (!mSnapWallInfo->isExist()) {
            return;
        }

        snapLength = (-(mSnapWallInfo->getPos() - checkPos)).dot(mSnapWallInfo->getNormal());
    } else {
        if (isOnFrontWall() && mSnapWallInfo->isExist()) {
            return;
        }

        mSnapWallInfo->clear();
        PlayerProperty* property = mProperty;
        PlayerCollisionFunc::calcWallCheckBodyPos(
            &checkPos, property->mTrans, property->mGroundUp,
            PlayerCollisionFunc::calcTall(property, mConstParam));
        checkDir = mProperty->mFront * mConstParam->getWallSnapDistance();
        checkSnapWall(checkPos, checkDir, false);

        if (!mSnapWallInfo->isExist()) {
            return;
        }

        snapLength =
            (-(mSnapWallInfo->getPos() - checkPos)).dot(mSnapWallInfo->getNormal()) * 1.1f;
    }

    snapLength = sead::Mathf::clampMin(
        snapLength - PlayerCollisionFunc::calcChestRadius(mProperty, mConstParam), 1.0f);
    mFrontWallInfo->clear();
    mBackWallInfo->clear();
    mLeftWallInfo->clear();
    mRightWallInfo->clear();
    sead::Vector3f push = mSnapWallInfo->getNormal() * -snapLength;
    applyVelocity(push, 0.0f, true);
}

/**
 * Calculates how far a point moved with a collision parts since last frame.
 * @param pOut the movement
 * @param rPos the point
 * @param pParts the collision parts
 */
NOINLINE void PlayerCollider::calcFollowVec(sead::Vector3f* pOut, const sead::Vector3f& rPos,
                                   const al::CollisionParts* pParts) {
    if (!mCollisionPartsMtx->isRegistered(pParts)) {
        return;
    }

    sead::Matrix34f prevInvMtx;
    prevInvMtx.setInverse(*mCollisionPartsMtx->getPrevBaseMtx(pParts));
    sead::Matrix34f moveMtx;
    moveMtx.setMul(*mCollisionPartsMtx->getBaseMtx(pParts), prevInvMtx);
    sead::Vector3f movedPos = moveMtx * rPos;
    pOut->setSub(movedPos, rPos);
}

/**
 * Looks for a wall to snap to and records it.
 * @param rPos where to look from
 * @param rDir where to look
 * @param isBack whether to look for a wall behind the player
 */
void PlayerCollider::checkSnapWall(const sead::Vector3f& rPos, const sead::Vector3f& rDir,
                                   bool isBack) {
    if (!mCheckArrow->checkArrow(rPos, rDir)) {
        return;
    }

    u32 index = PlayerCollisionFunc::findNearestArrowCollision(mCheckArrow, rPos);
    sead::Vector3f normal = mCheckArrow->getNormal(index);
    f32 depth = mConstParam->getWallSnapDistance() - (mCheckArrow->getPos(index) - rPos).length();

    if (depth <= 0.0f) {
        return;
    }

    sead::Vector3f horizontalNormal = normal;
    al::verticalizeVec(&horizontalNormal, mProperty->mGroundUp, horizontalNormal);
    al::normalize(&horizontalNormal);

    if (!isWall(normal)) {
        return;
    }

    if (isBack) {
        if (!PlayerCollisionFunc::isBackWall(horizontalNormal, mProperty->mFront)) {
            return;
        }

        mSnapWallInfo->record(mProperty, mCheckArrow->getPos(index), normal,
                              mCheckArrow->getWallCodeName(index));
        mBackWallInfo->record(normal, depth, mCheckArrow->getCollisionParts(index),
                              mCheckArrow->getMapCodeName(index),
                              mCheckArrow->getWallCodeName(index),
                              mCheckArrow->getMaterialCodeName(index));
        registerPartsArray(mBackHemispherePartsArray, mCheckArrow->getCollisionParts(index));
    } else {
        if (!PlayerCollisionFunc::isFrontWall(horizontalNormal, mProperty->mFront)) {
            return;
        }

        mSnapWallInfo->record(mProperty, mCheckArrow->getPos(index), normal,
                              mCheckArrow->getWallCodeName(index));
        mFrontWallInfo->record(normal, depth, mCheckArrow->getCollisionParts(index),
                               mCheckArrow->getMapCodeName(index),
                               mCheckArrow->getWallCodeName(index),
                               mCheckArrow->getMaterialCodeName(index));
        registerPartsArray(mFrontPartsArray, mCheckArrow->getCollisionParts(index));
        registerPartsArray(mFrontHemispherePartsArray, mCheckArrow->getCollisionParts(index));
    }
}

/**
 * Adds a push from outside (it is applied on the next move).
 * @param rPush the push
 */
void PlayerCollider::push(const sead::Vector3f& rPush) {
    calcMinMax(&mPushMin, &mPushMax, rPush);
}

/**
 * Grows a box (given as its min and max corners) to contain a vector.
 * @param pMin the box's min corner
 * @param pMax the box's max corner
 * @param rVec the vector
 */
void PlayerCollider::calcMinMax(sead::Vector3f* pMin, sead::Vector3f* pMax,
                                const sead::Vector3f& rVec) const {
    if (pMin->x > rVec.x) {
        pMin->x = rVec.x;
    }

    if (pMin->y > rVec.y) {
        pMin->y = rVec.y;
    }

    if (pMin->z > rVec.z) {
        pMin->z = rVec.z;
    }

    if (pMax->x < rVec.x) {
        pMax->x = rVec.x;
    }

    if (pMax->y < rVec.y) {
        pMax->y = rVec.y;
    }

    if (pMax->z < rVec.z) {
        pMax->z = rVec.z;
    }
}

/**
 * Forgets the pushes from outside.
 */
void PlayerCollider::clearPush() {
    mPushMin = {0.0f, 0.0f, 0.0f};
    mPushMax = {0.0f, 0.0f, 0.0f};
}

/**
 * Keeps the horizontal part of the floor's movement as the velocity for a jump.
 */
void PlayerCollider::arrangeJumpFollowVel() {
    mJumpFollowVel = mFollowVel;
    al::verticalizeVec(&mJumpFollowVel, mProperty->mUpDir, mJumpFollowVel);
    mFollowVel = {0.0f, 0.0f, 0.0f};
}

/**
 * Forgets the velocities inherited from the floor.
 */
void PlayerCollider::clearJumpFollowVel() {
    mJumpFollowVel = {0.0f, 0.0f, 0.0f};
    mFollowVel = {0.0f, 0.0f, 0.0f};
}

/**
 * @return whether the player stands on a floor
 */
bool PlayerCollider::isOnFloor() const {
    return mFloorInfo->isValid();
}

/**
 * @return whether the player touches a wall in front
 */
bool PlayerCollider::isOnFrontWall() const {
    return mFrontWallInfo->isValid();
}

/**
 * @return whether the player touches a wall behind
 */
bool PlayerCollider::isOnBackWall() const {
    return mBackWallInfo->isValid();
}

/**
 * @return whether the player touches a ceiling
 */
bool PlayerCollider::isOnCeiling() const {
    return mCeilingInfo->isValid();
}

/**
 * @return whether the player touches a wall on its right
 */
bool PlayerCollider::isOnRightWall() const {
    return mRightWallInfo->isValid();
}

/**
 * @return whether the player touches a wall on its left
 */
bool PlayerCollider::isOnLeftWall() const {
    return mLeftWallInfo->isValid();
}

/**
 * @return whether the player's head touches a wall in front
 */
bool PlayerCollider::isHeadOnFrontWall() const {
    return mIsHeadOnFrontWall;
}

/**
 * @return whether the player touches any wall (never with the old player parameters)
 */
bool PlayerCollider::isOnAnyWall() const {
    if (rc::isUsingOldPlayerParams()) {
        return false;
    }

    return mIsHeadOnFrontWall || mRightWallInfo->isValid() || mLeftWallInfo->isValid() ||
           mFrontWallInfo->isValid() || mBackWallInfo->isValid();
}

/**
 * @param pInfo filled with the floor the player stands on
 */
void PlayerCollider::getFloorInfo(Info* pInfo) const {
    copyCollisionInfo(pInfo, mFloorInfo);
}

/**
 * @param pInfo filled with the wall in front of the player
 */
void PlayerCollider::getFrontWallInfo(Info* pInfo) const {
    copyCollisionInfo(pInfo, mFrontWallInfo);
}

/**
 * @param pInfo filled with the wall behind the player
 */
void PlayerCollider::getBackWallInfo(Info* pInfo) const {
    copyCollisionInfo(pInfo, mBackWallInfo);
}

/**
 * @param pInfo filled with the ceiling above the player
 */
void PlayerCollider::getCeilingInfo(Info* pInfo) const {
    copyCollisionInfo(pInfo, mCeilingInfo);
}

/**
 * @param pInfo filled with the wall on the player's right
 */
void PlayerCollider::getRightWallInfo(Info* pInfo) const {
    copyCollisionInfo(pInfo, mRightWallInfo);
}

/**
 * @param pInfo filled with the wall on the player's left
 */
void PlayerCollider::getLeftWallInfo(Info* pInfo) const {
    copyCollisionInfo(pInfo, mLeftWallInfo);
}

/**
 * Makes the body short (squatting).
 */
void PlayerCollider::shrinkBody() {
    mIsShrink = true;
}

/**
 * Makes the body tall again.
 */
void PlayerCollider::growBody() {
    mIsShrink = false;
    mGrowTimer = 1;
}

/**
 * Cuts the velocity into the touched floor, ceiling and walls.
 * @param flags the sides to keep the velocity for (bit 0 floor, 1 ceiling, 2 front, 3 back,
 * 4 left, 5 right)
 */
void PlayerCollider::cutVelocity(u32 flags) {
    f32 limit = mConstParam->getCutVelLimit();
    f32 rate = mConstParam->getCutVelRate();
    f32 speed = mProperty->mUpDir.dot(mProperty->mVelocity);

    if (!(flags & 1) && isOnFloor() && speed < -limit) {
        al::verticalizeVec(&mProperty->mVelocity, mProperty->mUpDir, mProperty->mVelocity);
        mProperty->mVelocity += mProperty->mUpDir * speed * rate;
    }

    if (!(flags & 2) && isOnCeiling() && speed > limit) {
        al::verticalizeVec(&mProperty->mVelocity, mProperty->mUpDir, mProperty->mVelocity);
        mProperty->mVelocity += mProperty->mUpDir * speed * rate;
    }

    speed = mProperty->mFront.dot(mProperty->mVelocity);

    if (!(flags & 4) && isOnFrontWall() && speed > limit) {
        al::verticalizeVec(&mProperty->mVelocity, mProperty->mFront, mProperty->mVelocity);
        mProperty->mVelocity += mProperty->mFront * speed * rate;
    }

    if (!(flags & 8) && isOnBackWall() && speed < -limit) {
        al::verticalizeVec(&mProperty->mVelocity, mProperty->mFront, mProperty->mVelocity);
        mProperty->mVelocity += mProperty->mFront * speed * rate;
    }

    sead::Vector3f side;
    side.setCross(mProperty->mFront, mProperty->mGroundUp);
    al::normalize(&side);
    speed = side.dot(mProperty->mVelocity);

    if (!(flags & 0x20) && mRightWallInfo->isValid() && speed > limit) {
        al::verticalizeVec(&mProperty->mVelocity, side, mProperty->mVelocity);
        mProperty->mVelocity += side * speed * rate;
    }

    if (!(flags & 0x10) && mLeftWallInfo->isValid() && speed < -limit) {
        al::verticalizeVec(&mProperty->mVelocity, side, mProperty->mVelocity);
        mProperty->mVelocity += side * speed * rate;
    }

    if (isOnCeiling() && isOnFloor()) {
        sead::Vector3f pushDir;
        al::verticalizeVec(&pushDir, mProperty->mGroundUp, mMovingPushVec);

        if (!al::normalizeOrZero(&pushDir)) {
            f32 dot = pushDir.dot(mProperty->mVelocity);

            if (dot < 0.0f) {
                mProperty->mVelocity -= pushDir * dot;
            }
        }
    }
}

/**
 * @return whether the player snapped to a wall
 */
bool PlayerCollider::isSnapWallExist() const {
    return mSnapWallInfo->isExist();
}

/**
 * @return the normal of the wall the player snapped to last
 */
const sead::Vector3f& PlayerCollider::getSnapWallLastNormal() const {
    return mSnapWallInfo->getLastNormal();
}

/**
 * @return the normal of the wall the player snapped to
 */
const sead::Vector3f& PlayerCollider::getSnapWallNormal() const {
    return mSnapWallInfo->getNormal();
}

/**
 * @return where the player snapped to the wall
 */
const sead::Vector3f& PlayerCollider::getSnapWallPos() const {
    return mSnapWallInfo->getPos();
}

/**
 * Calculates the center of the body sphere.
 * @param pOut the center
 * @param rTrans the player's position
 * @param isAddHover whether to lift it by the hover height
 */
void PlayerCollider::calcBodyPos(sead::Vector3f* pOut, const sead::Vector3f& rTrans,
                                 bool isAddHover) const {
    PlayerCollisionFunc::calcCollisionBodyPos(
        pOut, rTrans, mProperty->mGroundUp,
        PlayerCollisionFunc::calcChestRadius(mProperty, mConstParam));

    if (isAddHover) {
        *pOut += mProperty->mGroundUp * 10.0f * mProperty->_78;
    }
}

/**
 * Calculates the center of the head sphere, lowered towards the body while squatting.
 * @param pOut the center
 * @param rTrans the player's position
 * @param isAddHover whether to lift it by the hover height
 */
void PlayerCollider::calcHeadPos(sead::Vector3f* pOut, const sead::Vector3f& rTrans,
                                 bool isAddHover) const {
    PlayerCollisionFunc::calcCollisionHeadPos(
        pOut, rTrans, mProperty->mGroundUp,
        PlayerCollisionFunc::calcChestRadius(mProperty, mConstParam),
        PlayerCollisionFunc::calcTall(mProperty, mConstParam));
    f32 rate = calcSquatRate();

    if (mHeadLowerTimer != 0 && mHeadLowerFrame != 0) {
        rate = sead::Mathf::max(rate, sead::Mathf::clampMax(static_cast<f32>(mHeadLowerTimer) /
                                                                static_cast<f32>(mHeadLowerFrame),
                                                            1.0f));
    }

    if (rate > 0.0f) {
        sead::Vector3f bodyPos;
        calcBodyPos(&bodyPos, rTrans, isAddHover);
        pOut->x = (1.0f - rate) * pOut->x + rate * bodyPos.x;
        pOut->y = (1.0f - rate) * pOut->y + rate * bodyPos.y;
        pOut->z = (1.0f - rate) * pOut->z + rate * bodyPos.z;
    }
}

/**
 * @return how far the head is lowered towards the body (1 while squatting)
 */
f32 PlayerCollider::calcSquatRate() const {
    f32 rate = sead::Mathf::clampMax(static_cast<f32>(mGrowTimer), 1.0f);
    return mIsShrink ? 1.0f : rate;
}

/**
 * @return the length of the leg (from the feet to where the leg arrow starts)
 */
f32 PlayerCollider::calcLegLength() const {
    return PlayerCollisionFunc::calcTall(mProperty, mConstParam) * 0.42f;
}

/**
 * Moves the player one step and calculates how to push it out of the map.
 * @param rTrans the player's position, moved
 * @param rVel the step
 * @param isUnused unused
 * @param rLegDir the leg arrow
 * @param legOffset how far below the leg the player still counts as standing
 * @param rSide the player's side direction
 * @param isSkipLeg whether to skip the leg (floor) check
 * @param isSwimSlow whether the player swims slowly (checks the floor behind too)
 */
void PlayerCollider::applyVelocityCore(sead::Vector3f& rTrans, const sead::Vector3f& rVel,
                                       bool isUnused, const sead::Vector3f& rLegDir,
                                       f32 legOffset, const sead::Vector3f& rSide,
                                       bool isSkipLeg, bool isSwimSlow) {
    rTrans += rVel;
    sead::Vector3f legPush = {0.0f, 0.0f, 0.0f};
    sead::Vector3f legMovingPush = {0.0f, 0.0f, 0.0f};
    sead::Vector3f legSidePush = {0.0f, 0.0f, 0.0f};
    sead::Vector3f legMovingSidePush = {0.0f, 0.0f, 0.0f};

    if (!isSkipLeg) {
        sead::Vector3f legPos;
        calcLegPos(&legPos, rTrans);
        checkLegArrow(&legPush, &legMovingPush, &legSidePush, &legMovingSidePush, legPos, rLegDir,
                      legOffset);
    }

    sead::Vector3f bodyPos;
    calcBodyPos(&bodyPos, rTrans, true);
    sead::Vector3f bodyPush = {0.0f, 0.0f, 0.0f};
    sead::Vector3f bodyMovingPush = {0.0f, 0.0f, 0.0f};
    checkBodySphere(&bodyPush, &bodyMovingPush, bodyPos, rSide, false, isSwimSlow);

    sead::Vector3f headPos;
    calcHeadPos(&headPos, rTrans, true);

    if ((headPos - bodyPos).dot(mProperty->mGroundUp) < 0.0f) {
        headPos.set(bodyPos);
    }

    sead::Vector3f headPush = {0.0f, 0.0f, 0.0f};
    sead::Vector3f headMovingPush = {0.0f, 0.0f, 0.0f};
    sead::Vector3f headSidePush = {0.0f, 0.0f, 0.0f};
    sead::Vector3f headMovingSidePush = {0.0f, 0.0f, 0.0f};
    checkHeadSphere(&headPush, &headMovingPush, &headSidePush, &headMovingSidePush, headPos,
                    rSide);

    sead::Vector3f exPush = {0.0f, 0.0f, 0.0f};
    sead::Vector3f exMovingPush = {0.0f, 0.0f, 0.0f};
    checkExSphere(&exPush, &exMovingPush, rTrans, rSide);

    sead::Vector3f min = {0.0f, 0.0f, 0.0f};
    sead::Vector3f max = {0.0f, 0.0f, 0.0f};
    calcMinMax(&min, &max, bodyPush);
    calcMinMax(&min, &max, headPush);
    calcMinMax(&min, &max, legPush);
    calcMinMax(&min, &max, exPush);

    sead::Vector3f movingMin = {0.0f, 0.0f, 0.0f};
    sead::Vector3f movingMax = {0.0f, 0.0f, 0.0f};
    calcMinMax(&movingMin, &movingMax, bodyMovingPush);
    calcMinMax(&movingMin, &movingMax, headMovingPush);
    calcMinMax(&movingMin, &movingMax, legMovingPush);
    calcMinMax(&movingMin, &movingMax, exMovingPush);

    if (isOnCeiling() && isOnFloor()) {
        calcMinMax(&min, &max, legSidePush);
        calcMinMax(&min, &max, headSidePush);
        calcMinMax(&movingMin, &movingMax, legMovingSidePush);
        calcMinMax(&movingMin, &movingMax, headMovingSidePush);
    }

    sead::Vector3f push = min + max;
    sead::Vector3f movingPush = movingMin + movingMax;

    if (!al::isNearZero(movingPush, 0.00001f)) {
        sead::Vector3f movingDir = movingPush * (1.0f / movingPush.length());
        f32 dot = push.dot(movingDir);

        if (movingPush.length() > dot) {
            push = push - movingDir * dot + movingPush;
        }
    }

    mPushVec = push;
    mMovingPushVec.set(movingPush);
}

/**
 * Keeps the player on its restricted plane (PlayerRestrictedPlane areas).
 */
void PlayerCollider::keepOutRestrictedArea() {
    al::AreaObj* area =
        rc::tryFindAreaObj(mActor, rc::AreaObjType::PlayerRestrictedPlane, mProperty->mTrans);

    if (area == nullptr) {
        return;
    }

    const sead::Matrix34f& baseMtx = al::getAreaObjBaseMtx(area);
    sead::Vector3f normal;
    baseMtx.getBase(normal, 1);

    if (al::normalizeOrZero(&normal)) {
        return;
    }

    f32 offset = area->getAreaShape()->mScale.y * 1000.0f;
    sead::Vector3f planePos = {baseMtx.m[0][3] + offset * normal.x,
                               baseMtx.m[1][3] + offset * normal.y,
                               baseMtx.m[2][3] + offset * normal.z};
    sead::Vector3f& trans = mProperty->mTrans;
    f32 dist = normal.dot(trans - planePos);
    trans -= normal * dist;
}

/**
 * Calculates where the leg arrow starts.
 * @param pOut where the leg arrow starts
 * @param rTrans the player's position
 */
void PlayerCollider::calcLegPos(sead::Vector3f* pOut, const sead::Vector3f& rTrans) const {
    *pOut = mProperty->mGroundUp * calcLegLength() + rTrans;
}

/**
 * Casts the legs down and records the floor they stand on.
 * @param pPush the push up out of the floor
 * @param pMovingPush the push up out of moving floors
 * @param pSidePush the push away from slopes
 * @param pMovingSidePush the push away from moving slopes
 * @param rStart where the leg arrow starts
 * @param rDir the leg arrow
 * @param legOffset how far below the leg the player still counts as standing
 * @return whether a leg stands on a floor
 */
bool PlayerCollider::checkLegArrow(sead::Vector3f* pPush, sead::Vector3f* pMovingPush,
                                   sead::Vector3f* pSidePush, sead::Vector3f* pMovingSidePush,
                                   const sead::Vector3f& rStart, const sead::Vector3f& rDir,
                                   f32 legOffset) {
    LegHitList hitList;
    LegHitList movingHitList;
    f32 legLength = rDir.length();
    mCheckArrow->checkArrow(rStart, rDir);

    sead::Vector3f min = {0.0f, 0.0f, 0.0f};
    sead::Vector3f max = {0.0f, 0.0f, 0.0f};
    sead::Vector3f movingMin = {0.0f, 0.0f, 0.0f};
    sead::Vector3f movingMax = {0.0f, 0.0f, 0.0f};

    if (mCheckArrow->getNum() != 0) {
        u32 index = findNearestLegCollision(rStart);
        const sead::Vector3f& normal = mCheckArrow->getNormal(index);

        if (isGround(normal)) {
            f32 dist = (rStart + rDir - mCheckArrow->getPos(index)).length();
            f32 hover = mProperty->_78 * 10.0f;
            bool isMoving = false;

            if (dist - hover > 0.0f) {
                f32 depth = dist - (hover + legOffset);
                const al::CollisionParts* parts = mCheckArrow->getCollisionParts(index);
                hitList.add(depth, parts);

                if (isMovingParts(parts)) {
                    movingHitList.add(depth, parts);
                    isMoving = true;
                }

                recordFloorCollisionInfo(index, legLength - depth);

                sead::Vector3f sidePush;
                al::verticalizeVec(&sidePush, mProperty->mGroundUp, normal);

                if (!al::normalizeOrZero(&sidePush)) {
                    f32 upDepth = depth * normal.dot(mProperty->mGroundUp);
                    f32 depthSq = depth * depth;
                    f32 sideDepth = sqrtf(fmaxf(depthSq - upDepth * upDepth, 0.0f));

                    if (sideDepth > 0.0f) {
                        sidePush *= fminf(depthSq / sideDepth, 40.0f);
                    } else {
                        sidePush = {0.0f, 0.0f, 0.0f};
                    }
                }

                calcMinMax(&min, &max, sidePush);

                if (isMoving) {
                    calcMinMax(&movingMin, &movingMax, sidePush);
                }
            }

            registerPartsArray(mFloorPartsArray, mCheckArrow->getCollisionParts(index));

            if (isMoving) {
                mIsCenterOnFloor = true;
            }
        }
    }

    if (mIsCheckSubLeg) {
        f32 radius = PlayerCollisionFunc::calcChestRadius(mProperty, mConstParam) * 0.75f;
        sead::Vector3f offset = mProperty->mFront * radius;
        sead::Quatf rotate;
        rotate.setAxisAngle(mProperty->mGroundUp, 120.0f);
        sead::Matrix34f rotateMtx;
        rotateMtx.makeQT(rotate, {0.0f, 0.0f, 0.0f});

        sead::Vector3f subPos = rStart + offset;
        checkSubLegArrow(this, subPos, rDir, rStart, &hitList, &movingHitList, legLength,
                         legOffset);
        offset = rotateMtx * offset;
        subPos = rStart + offset;
        checkSubLegArrow(this, subPos, rDir, rStart, &hitList, &movingHitList, legLength,
                         legOffset);
        offset = rotateMtx * offset;
        subPos = rStart + offset;
        checkSubLegArrow(this, subPos, rDir, rStart, &hitList, &movingHitList, legLength,
                         legOffset);

        sead::Vector3f horizontalVel;
        al::verticalizeVec(&horizontalVel, mProperty->mUpDir, mProperty->mVelocity);
        f32 speed = horizontalVel.length();
        bool isCheckDash;

        if (rc::isUsingOldPlayerParams()) {
            isCheckDash = true;
        } else {
            isCheckDash = !(mIsDisableLegCheck && isOnAnyWall()) && !mIsDisableLegCheckForce;
        }

        if (isCheckDash && mInput != nullptr && mInput->isDashButtonOn() &&
            speed > mConstParam->getNormalMaxSpeed()) {
            f32 rate = (speed - mConstParam->getNormalMaxSpeed()) /
                       (mConstParam->getDashMaxSpeed() - mConstParam->getNormalMaxSpeed());
            f32 dashRate;

            if (rate < 0.0f) {
                dashRate = 0.0f;
            } else if (rate > 1.0f) {
                dashRate = 1.0f;
            } else {
                dashRate = rate;
            }

            f32 dashRadius = PlayerCollisionFunc::calcDashCheckRadius(mProperty, mConstParam);
            sead::Vector3f dashPos = rStart - mProperty->mFront * (dashRate * dashRadius);
            checkSubLegArrow(this, dashPos, rDir, rStart, &hitList, &movingHitList, legLength,
                             legOffset);
        }
    }

    pPush->set(mProperty->mGroundUp);
    *pPush *= hitList.calcDepth();

    if (movingHitList.calcDepth() > 0.0f) {
        pMovingPush->set(mProperty->mGroundUp);
        *pMovingPush *= movingHitList.calcDepth();
    }

    *pSidePush = min + max;
    *pMovingSidePush = movingMin + movingMax;
    return hitList.mNum != 0;
}

/**
 * Checks the body sphere against the map and records the walls (and floors) it touches.
 * @param pPush the push out of the map
 * @param pMovingPush the push out of moving parts
 * @param rPos the sphere's center
 * @param rSide the player's side direction
 * @param isCheckFloor whether to record floors too
 * @param isCheckFloorBack whether to record moving floors behind the player too
 */
void PlayerCollider::checkBodySphere(sead::Vector3f* pPush, sead::Vector3f* pMovingPush,
                                     const sead::Vector3f& rPos, const sead::Vector3f& rSide,
                                     bool isCheckFloor, bool isCheckFloorBack) {
    f32 radius = PlayerCollisionFunc::calcChestRadius(mProperty, mConstParam);
    mCheckSphere->checkSphere(rPos, radius);
    sead::Vector3f min = {0.0f, 0.0f, 0.0f};
    sead::Vector3f max = {0.0f, 0.0f, 0.0f};
    sead::Vector3f movingMin = {0.0f, 0.0f, 0.0f};
    sead::Vector3f movingMax = {0.0f, 0.0f, 0.0f};

    for (u32 i = 0; i < mCheckSphere->getNum(); i++) {
        if (isWall(mCheckSphere->getNormal(i))) {
            calcWallInfo(min, max, movingMin, movingMax, i, rSide);
        }

        if (!isCheckFloor) {
            continue;
        }

        sead::Vector3f hitPos = mCheckSphere->getPos(i);

        if (!isGround(mCheckSphere->getNormal(i))) {
            continue;
        }

        if (mCheckSphere->getCollisionParts(i)->_154 != 0 &&
            !((hitPos - rPos).dot(mProperty->mFront) > radius * -0.4f) && !isCheckFloorBack) {
            continue;
        }

        const sead::Vector3f& normal = mCheckSphere->getNormal(i);
        f32 depth = fmaxf(mCheckSphere->getDepth(i) - 1.0f, 0.0f);
        sead::Vector3f push = normal * depth + mCheckSphere->getMoveVec(i);
        calcMinMax(&min, &max, push);

        if (isMovingParts(mCheckSphere->getCollisionParts(i))) {
            calcMinMax(&movingMin, &movingMax, push);
        }

        mFloorInfo->record(normal, depth, mCheckSphere->getCollisionParts(i),
                           mCheckSphere->getMapCodeName(i), mCheckSphere->getWallCodeName(i),
                           mCheckSphere->getMaterialCodeName(i));
        registerPartsArray(mFloorPartsArray, mCheckSphere->getCollisionParts(i));
    }

    *pPush = min + max;
    *pMovingPush = movingMin + movingMax;
}

/**
 * Checks the head sphere against the map and records the walls and ceilings it touches.
 * @param pPush the push out of the map
 * @param pMovingPush the push out of moving parts
 * @param pSidePush the push away from sloped ceilings
 * @param pMovingSidePush the push away from moving sloped ceilings
 * @param rPos the sphere's center
 * @param rSide the player's side direction
 */
void PlayerCollider::checkHeadSphere(sead::Vector3f* pPush, sead::Vector3f* pMovingPush,
                                     sead::Vector3f* pSidePush, sead::Vector3f* pMovingSidePush,
                                     const sead::Vector3f& rPos, const sead::Vector3f& rSide) {
    mCheckSphere->checkSphere(rPos, PlayerCollisionFunc::calcChestRadius(mProperty, mConstParam));
    sead::Vector3f min = {0.0f, 0.0f, 0.0f};
    sead::Vector3f max = {0.0f, 0.0f, 0.0f};
    sead::Vector3f movingMin = {0.0f, 0.0f, 0.0f};
    sead::Vector3f movingMax = {0.0f, 0.0f, 0.0f};
    sead::Vector3f sideMin = {0.0f, 0.0f, 0.0f};
    sead::Vector3f sideMax = {0.0f, 0.0f, 0.0f};
    sead::Vector3f movingSideMin = {0.0f, 0.0f, 0.0f};
    sead::Vector3f movingSideMax = {0.0f, 0.0f, 0.0f};

    for (u32 i = 0; i < mCheckSphere->getNum(); i++) {
        const sead::Vector3f& normal = mCheckSphere->getNormal(i);
        sead::Vector3f moveVec = {0.0f, 0.0f, 0.0f};

        if (normal.dot(mCheckSphere->getMoveVec(i)) > 0.0f) {
            moveVec = mCheckSphere->getMoveVec(i);
        }

        if (isWallForHead(normal)) {
            sead::Vector3f push = moveVec + mCheckSphere->getDepth(i) * normal;
            calcMinMax(&min, &max, push);

            if (isMovingParts(mCheckSphere->getCollisionParts(i))) {
                calcMinMax(&movingMin, &movingMax, push);
            }

            recordWallCollisionInfo(i, rSide);

            if (PlayerCollisionFunc::isFrontWall(normal, mProperty->mFront)) {
                mIsHeadOnFrontWall = true;
            }
        } else if (isCeiling(normal)) {
            f32 depth = mCheckSphere->getDepth(i);
            sead::Vector3f push = moveVec + depth * normal;
            calcMinMax(&min, &max, push);
            bool isMoving = isMovingParts(mCheckSphere->getCollisionParts(i));

            if (isMoving) {
                calcMinMax(&movingMin, &movingMax, push);
            }

            recordCeilingCollisionInfo(i);

            sead::Vector3f sidePush;
            al::verticalizeVec(&sidePush, mProperty->mGroundUp, normal);

            if (!al::normalizeOrZero(&sidePush)) {
                f32 upDepth = depth * normal.dot(mProperty->mGroundUp);
                f32 upDepthSq = upDepth * upDepth;
                f32 sideDepth = sqrtf(fmaxf(depth * depth - upDepthSq, 0.0f));

                if (sideDepth > 0.0f) {
                    sidePush *= fminf(sideDepth + upDepthSq / sideDepth, 40.0f);
                } else {
                    sidePush = {0.0f, 0.0f, 0.0f};
                }
            }

            calcMinMax(&sideMin, &sideMax, sidePush);

            if (isMoving) {
                calcMinMax(&movingSideMin, &movingSideMax, sidePush);
            }
        }
    }

    *pPush = min + max;
    *pMovingPush = movingMin + movingMax;
    *pSidePush = sideMin + sideMax;
    *pMovingSidePush = movingSideMin + movingSideMax;
}

/**
 * Checks the extra spheres against the map while their animation plays.
 * @param pPush the push out of the map
 * @param pMovingPush the push out of moving parts
 * @param rTrans the player's position
 * @param rSide the player's side direction
 */
void PlayerCollider::checkExSphere(sead::Vector3f* pPush, sead::Vector3f* pMovingPush,
                                   const sead::Vector3f& rTrans, const sead::Vector3f& rSide) {
    if (!mIsValidExSphere || !mAnimator->isAnim(sead::SafeString(mExSphere->mAnimName))) {
        return;
    }

    al::CollisionMultiSphere<64> multiSphere(mActor, mExSphere->mSpheres, mExSphere->mSphereNum,
                                             sizeof(Sphere));
    sead::Vector3f side;
    side.setCross(mProperty->mGroundUp, mProperty->mFront);
    mExSphereMtx.setBase(0, side);
    mExSphereMtx.setBase(1, mProperty->mGroundUp);
    mExSphereMtx.setBase(2, mProperty->mFront);
    mExSphereMtx.setBase(3, rTrans);
    multiSphere.setBaseMtx(&mExSphereMtx, nullptr);
    multiSphere.check();

    sead::Vector3f min = {0.0f, 0.0f, 0.0f};
    sead::Vector3f max = {0.0f, 0.0f, 0.0f};
    sead::Vector3f movingMin = {0.0f, 0.0f, 0.0f};
    sead::Vector3f movingMax = {0.0f, 0.0f, 0.0f};

    for (u32 i = 0; i < multiSphere.getHitNum(); i++) {
        const al::HitInfo* hitInfo = multiSphere.getHitInfo(i);

        if (!(hitInfo->_70 >= 1.0f) || isGround(*hitInfo->mTriangle.getNormal(0)) ||
            !((hitInfo->mPos - mProperty->mTrans).dot(*hitInfo->mTriangle.getNormal(0)) < 0.0f) ||
            isBlockBorder(hitInfo->mPos, *hitInfo->mTriangle.getNormal(0))) {
            continue;
        }

        sead::Vector3f push = *hitInfo->mTriangle.getNormal(0) * (hitInfo->_70 - 1.0f);
        calcMinMax(&min, &max, push);

        if (isMovingParts(hitInfo->mTriangle.mCollisionParts)) {
            calcMinMax(&movingMin, &movingMax, push);
        }

        if (isWall(*hitInfo->mTriangle.getNormal(0))) {
            mSnapWallInfo->record(mProperty, hitInfo->mPos, *hitInfo->mTriangle.getNormal(0),
                                  al::getWallCodeName(hitInfo->mTriangle));
            recordWallCollisionInfoCommon(hitInfo->mPos, *hitInfo->mTriangle.getNormal(0),
                                          hitInfo->_70, hitInfo->mTriangle.mCollisionParts,
                                          al::getFloorCodeName(hitInfo->mTriangle),
                                          al::getWallCodeName(hitInfo->mTriangle),
                                          al::getMaterialCodeName(hitInfo->mTriangle), rSide);
        }
    }

    *pPush = min + max;
    *pMovingPush = movingMin + movingMax;
}

/**
 * @param rPos where the leg arrow starts
 * @return the index of the leg arrow hit nearest to the start
 */
u32 PlayerCollider::findNearestLegCollision(const sead::Vector3f& rPos) const {
    f32 nearestDist = (mCheckArrow->getPos(0) - rPos).length();
    u32 nearestIndex = 0;

    for (u32 i = 1; i < mCheckArrow->getNum(); i++) {
        f32 dist = (mCheckArrow->getPos(i) - rPos).length();

        if (dist < nearestDist) {
            nearestDist = dist;
            nearestIndex = i;
        }
    }

    return nearestIndex;
}

/**
 * @param rNormal a hit's normal
 * @return whether the hit is a floor
 */
bool PlayerCollider::isGround(const sead::Vector3f& rNormal) const {
    return mProperty->mUpDir.dot(rNormal) >= 0.5f;
}

/**
 * Records a leg arrow hit as the floor.
 * @param index the hit's index
 * @param depth how deep the player is in the floor
 */
void PlayerCollider::recordFloorCollisionInfo(u32 index, f32 depth) {
    mFloorInfo->record(mCheckArrow->getNormal(index), depth,
                       mCheckArrow->getCollisionParts(index), mCheckArrow->getMapCodeName(index),
                       mCheckArrow->getWallCodeName(index),
                       mCheckArrow->getMaterialCodeName(index));
}

/**
 * Adds a collision parts to an array unless it is already in it.
 * @param pArray the array
 * @param pParts the collision parts
 */
void PlayerCollider::registerPartsArray(CollisionPartsArray* pArray,
                                        const al::CollisionParts* pParts) {
    if (pArray->find(pParts) == nullptr) {
        pArray->pushBack(const_cast<al::CollisionParts*>(pParts));
    }
}

namespace {
/**
 * Casts an extra leg arrow and records the floor it hits.
 * @param pCollider the collider
 * @param rPos where the arrow starts
 * @param rDir the arrow
 * @param rLegStart where the main leg arrow starts
 * @param pHitList the list the depth is added to
 * @param pMovingHitList the list the depth is added to if the floor moves
 * @param legLength the arrow's length
 * @param legOffset how far below the leg the player still counts as standing
 */
void checkSubLegArrow(PlayerCollider* pCollider, const sead::Vector3f& rPos,
                      const sead::Vector3f& rDir, const sead::Vector3f& rLegStart,
                      LegHitList* pHitList, LegHitList* pMovingHitList, f32 legLength,
                      f32 legOffset) {
    IUsePlayerCollisionCheckArrow* checkArrow = pCollider->getCheckArrow();
    checkArrow->checkArrow(rPos, rDir);

    if (pCollider->getCheckArrow()->getNum() == 0) {
        return;
    }

    u32 index = pCollider->findNearestLegCollision(rPos);

    if (!pCollider->isGround(pCollider->getCheckArrow()->getNormal(index))) {
        return;
    }

    const sead::Vector3f& normal = pCollider->getCheckArrow()->getNormal(index);
    sead::Vector3f diff = rLegStart - pCollider->getCheckArrow()->getPos(index);
    f32 startDist = diff.dot(normal);
    f32 endDist = (diff + rDir).dot(normal);
    f32 dist = -(endDist * legLength) / (startDist - endDist);
    f32 hover = pCollider->getProperty()->_78 * 10.0f;

    if (dist - hover > 0.0f) {
        f32 depth = dist - (hover + legOffset);
        const al::CollisionParts* parts = pCollider->getCheckArrow()->getCollisionParts(index);
        pHitList->add(depth, parts);

        if (isMovingParts(parts)) {
            pMovingHitList->add(depth, parts);
        }

        pCollider->recordFloorCollisionInfo(index, legLength - depth);
    }

    pCollider->registerPartsArray(pCollider->getFloorPartsArrayMutable(),
                                  pCollider->getCheckArrow()->getCollisionParts(index));
}
}  // namespace

/**
 * @param rNormal a hit's normal
 * @return whether the hit is a wall
 */
bool PlayerCollider::isWall(const sead::Vector3f& rNormal) const {
    return !isGround(rNormal) && !isCeiling(rNormal);
}

/**
 * Adds a body sphere wall hit to the pushes and records it.
 * @param rMin the push's min corner
 * @param rMax the push's max corner
 * @param rMovingMin the moving parts push's min corner
 * @param rMovingMax the moving parts push's max corner
 * @param index the hit's index
 * @param rSide the player's side direction
 */
void PlayerCollider::calcWallInfo(sead::Vector3f& rMin, sead::Vector3f& rMax,
                                  sead::Vector3f& rMovingMin, sead::Vector3f& rMovingMax,
                                  u32 index, const sead::Vector3f& rSide) {
    f32 moveDot = mCheckSphere->getMoveVec(index).dot(mCheckSphere->getNormal(index));
    const sead::Vector3f& normal = mCheckSphere->getNormal(index);
    sead::Vector3f push = mCheckSphere->getDepth(index) * normal;

    if (moveDot > 0.0f) {
        push += mCheckSphere->getMoveVec(index);
    }

    calcMinMax(&rMin, &rMax, push);

    if (isMovingParts(mCheckSphere->getCollisionParts(index))) {
        calcMinMax(&rMovingMin, &rMovingMax, push);
    }

    recordWallCollisionInfo(index, rSide);
}

/**
 * @param rNormal a hit's normal
 * @return whether the hit is a wall for the head sphere (steeper while not squatting)
 */
bool PlayerCollider::isWallForHead(const sead::Vector3f& rNormal) const {
    f32 squatRate = calcSquatRate();
    f32 dot = mProperty->mUpDir.dot(rNormal);
    if (squatRate == 1.0f) {
        if (!(dot < 0.5f)) {
            return false;
        }
    } else if (!(dot < 0.70710677f)) {
        return false;
    }

    return !isCeiling(rNormal);
}

/**
 * Records a sphere hit as a wall (and as the wall to snap to).
 * @param index the hit's index
 * @param rSide the player's side direction
 */
void PlayerCollider::recordWallCollisionInfo(u32 index, const sead::Vector3f& rSide) {
    const sead::Vector3f& pos = mCheckSphere->getPos(index);
    const sead::Vector3f& normal = mCheckSphere->getNormal(index);
    f32 depth = mCheckSphere->getDepth(index);
    mSnapWallInfo->record(mProperty, pos, normal, mCheckSphere->getWallCodeName(index));
    recordWallCollisionInfoCommon(pos, normal, depth, mCheckSphere->getCollisionParts(index),
                                  mCheckSphere->getMapCodeName(index),
                                  mCheckSphere->getWallCodeName(index),
                                  mCheckSphere->getMaterialCodeName(index), rSide);
}

/**
 * @param rNormal a hit's normal
 * @return whether the hit is a ceiling
 */
bool PlayerCollider::isCeiling(const sead::Vector3f& rNormal) const {
    return mProperty->mUpDir.dot(rNormal) < -0.8660254f;
}

/**
 * Records a sphere hit as the ceiling.
 * @param index the hit's index
 */
void PlayerCollider::recordCeilingCollisionInfo(u32 index) {
    mCeilingInfo->record(mCheckSphere->getNormal(index), mCheckSphere->getDepth(index),
                         mCheckSphere->getCollisionParts(index),
                         mCheckSphere->getMapCodeName(index),
                         mCheckSphere->getWallCodeName(index),
                         mCheckSphere->getMaterialCodeName(index));
    registerPartsArray(mCeilingPartsArray, mCheckSphere->getCollisionParts(index));

    if ((mCheckSphere->getPos(index) - mProperty->mTrans).dot(mProperty->mFront) > 0.0f) {
        registerPartsArray(mFrontHemispherePartsArray, mCheckSphere->getCollisionParts(index));
    } else {
        registerPartsArray(mBackHemispherePartsArray, mCheckSphere->getCollisionParts(index));
    }
}

/**
 * @param rPos a hit's position
 * @param rNormal the hit's normal
 * @return whether the hit is on a border between two blocks (the map continues past it)
 */
bool PlayerCollider::isBlockBorder(const sead::Vector3f& rPos,
                                   const sead::Vector3f& rNormal) const {
    return alCollisionUtil::checkStrikeArrow(mActor, rPos - rNormal * 5.0f, rNormal * 10.0f,
                                             nullptr, nullptr) != 0;
}

/**
 * Records a wall hit on the side of the player it is on.
 * @param rPos the hit's position
 * @param rNormal the hit's normal
 * @param depth how deep the player is in the wall
 * @param pParts the collision parts that was hit
 * @param pMapCode the hit's map code
 * @param pWallCode the hit's wall code
 * @param pMaterialCode the hit's material code
 * @param rSide the player's side direction
 */
void PlayerCollider::recordWallCollisionInfoCommon(const sead::Vector3f& rPos,
                                                   const sead::Vector3f& rNormal, f32 depth,
                                                   const al::CollisionParts* pParts,
                                                   const char* pMapCode, const char* pWallCode,
                                                   const char* pMaterialCode,
                                                   const sead::Vector3f& rSide) {
    sead::Vector3f horizontalNormal = rNormal;
    al::verticalizeVec(&horizontalNormal, mProperty->mUpDir, horizontalNormal);
    al::normalize(&horizontalNormal);

    if (PlayerCollisionFunc::isFrontWall(horizontalNormal, mProperty->mFront)) {
        if (isBlockBorder(rPos, rNormal)) {
            return;
        }

        mFrontWallInfo->record(rNormal, depth, pParts, pMapCode, pWallCode, pMaterialCode);
        mFrontWallAnyInfo->record(rNormal, depth, pParts, pMapCode, pWallCode, pMaterialCode);
        registerPartsArray(mFrontPartsArray, pParts);
        registerPartsArray(mFrontHemispherePartsArray, pParts);
    } else if (PlayerCollisionFunc::isBackWall(horizontalNormal, mProperty->mFront)) {
        mBackWallInfo->record(rNormal, depth, pParts, pMapCode, pWallCode, pMaterialCode);
        registerPartsArray(mBackHemispherePartsArray, pParts);
    } else if (isRightWall(horizontalNormal, rSide)) {
        if (isBlockBorder(rPos, rNormal)) {
            return;
        }

        mRightWallInfo->record(rNormal, depth, pParts, pMapCode, pWallCode, pMaterialCode);

        if ((rPos - mProperty->mTrans).dot(mProperty->mFront) > 0.0f) {
            registerPartsArray(mFrontHemispherePartsArray, pParts);
        } else {
            registerPartsArray(mBackHemispherePartsArray, pParts);
        }
    } else {
        if (isBlockBorder(rPos, rNormal)) {
            return;
        }

        mLeftWallInfo->record(rNormal, depth, pParts, pMapCode, pWallCode, pMaterialCode);

        if ((rPos - mProperty->mTrans).dot(mProperty->mFront) > 0.0f) {
            registerPartsArray(mFrontHemispherePartsArray, pParts);
        } else {
            registerPartsArray(mBackHemispherePartsArray, pParts);
        }
    }
}

/**
 * @param rNormal a wall's horizontal normal
 * @param rSide the player's side direction
 * @return whether the wall is on the player's left
 */
bool PlayerCollider::isLeftWall(const sead::Vector3f& rNormal, const sead::Vector3f& rSide) const {
    return rSide.dot(rNormal) > 0.0f;
}

/**
 * @param rNormal a wall's horizontal normal
 * @param rSide the player's side direction
 * @return whether the wall is on the player's right
 */
bool PlayerCollider::isRightWall(const sead::Vector3f& rNormal,
                                 const sead::Vector3f& rSide) const {
    return rSide.dot(rNormal) <= 0.0f;
}

/**
 * @return whether the player's center stands on a moving floor
 */
bool PlayerCollider::isCenterOnFloor() const {
    return mIsCenterOnFloor;
}

/**
 * @param isDisable whether to skip the dash leg check while touching a wall
 */
void PlayerCollider::setDisableLegCheck(bool isDisable) {
    mIsDisableLegCheck = isDisable;
}

/**
 * @param isDisable whether to always skip the dash leg check
 */
void PlayerCollider::setDisableLegCheckForce(bool isDisable) {
    mIsDisableLegCheckForce = isDisable;
}

/**
 * @return the floors touched this frame
 */
const CollisionPartsArray* PlayerCollider::getFloorPartsArray() const {
    return mFloorPartsArray;
}

/**
 * @return the ceilings touched this frame
 */
const CollisionPartsArray* PlayerCollider::getCeilingPartsArray() const {
    return mCeilingPartsArray;
}

/**
 * @return the walls in front touched this frame
 */
const CollisionPartsArray* PlayerCollider::getFrontPartsArray() const {
    return mFrontPartsArray;
}

/**
 * @return the parts touched in front of the player this frame
 */
const CollisionPartsArray* PlayerCollider::getFrontHemispherePartsArray() const {
    return mFrontHemispherePartsArray;
}

/**
 * @return the parts touched behind the player this frame
 */
const CollisionPartsArray* PlayerCollider::getBackHemispherePartsArray() const {
    return mBackHemispherePartsArray;
}
