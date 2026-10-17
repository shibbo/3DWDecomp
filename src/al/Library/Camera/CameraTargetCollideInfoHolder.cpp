#include "Library/Camera/CameraTargetCollideInfoHolder.hpp"

#include "Library/Camera/CameraTriangleFilter.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Project/Collision/Collider.hpp"
#include "Project/Collision/CollisionPartsFilterBase.hpp"
#include "Project/Collision/CollisionPartsTriangle.hpp"
#include "Project/Collision/CollisionUtil.hpp"

namespace {

al::CameraTriangleFilter sTriangleFilter;
al::CollisionPartsFilterOnlySpecialPurpose sPartsFilter2D("2DOnly");
const sead::Vector3f sDownDir = {0.0f, -1.0f, 0.0f};

}  // namespace

namespace al {

/**
 * Creates the holder without collision info.
 */
CameraTargetCollideInfoHolder::CameraTargetCollideInfoHolder() = default;

/**
 * Checks the collision under the camera target.
 * @param isValid Whether the target is valid.
 * @param rTrans Target position.
 * @param rVelocity Target velocity.
 * @param rGravity Target gravity.
 */
void CameraTargetCollideInfoHolder::update(bool isValid, const sead::Vector3f& rTrans,
                                           const sead::Vector3f& rVelocity,
                                           const sead::Vector3f& rGravity) {
    reset();
    mGravity.set(rGravity);
    mInvalidCount = isValid ? 0 : mInvalidCount + 1;

    Triangle triangle;

    if (!alCollisionUtil::getFirstPolyOnArrow(this, &mTargetCollisionPos, &triangle,
                                              rTrans - mGravity * 50.0f, mGravity * 2000.0f,
                                              mIs2D ? &sPartsFilter2D : nullptr,
                                              &sTriangleFilter)) {
        return;
    }

    mIsExistCollisionUnderTarget = true;
    mTargetCollisionNormal.set(*triangle.getNormal(0));
    normalize(&mTargetCollisionNormal);

    if (!isInRange(calcAngleDegree(-sDownDir, mTargetCollisionNormal), 15.0f, 70.0f)) {
        return;
    }

    mIsExistSlopeCollisionUnderTarget = true;
    sead::Vector3f side;
    side.setCross(mTargetCollisionNormal, sDownDir);
    normalize(&side);
    mSlopeDownDir.setCross(side, mTargetCollisionNormal);
    normalize(&mSlopeDownDir);

    sead::Vector3f velocity = rVelocity;
    parallelizeVec(&velocity, mSlopeDownDir, velocity);
    f32 dot = velocity.dot(mSlopeDownDir);
    f32 speed = velocity.length();

    if (dot > 0.0f) {
        mSlopeCollisionDownSpeed = speed;
    } else {
        mSlopeCollisionUpSpeed = speed;
    }
}

/**
 * Clears the collision info.
 */
void CameraTargetCollideInfoHolder::reset() {
    mTargetCollisionNormal = {0.0f, 0.0f, 0.0f};
    mIsExistCollisionUnderTarget = false;
    mIsExistSlopeCollisionUnderTarget = false;
    mSlopeCollisionDownSpeed = 0.0f;
    mSlopeCollisionUpSpeed = 0.0f;
}

/**
 * Returns whether the collision under the target is a wall.
 * @return Whether a wall is under the target.
 */
bool CameraTargetCollideInfoHolder::isExistUnderWall() const {
    if (!mIsExistCollisionUnderTarget) {
        return false;
    }

    return isWallPolygon(mTargetCollisionNormal, -sead::Vector3f::ey);
}

/**
 * Calculates the horizontal direction down the slope under the target.
 * @param pDir Receives the direction.
 * @return Whether the direction could be calculated.
 */
bool CameraTargetCollideInfoHolder::tryCalcSlopeDownFrontDirH(sead::Vector3f* pDir) const {
    if (!isNormalize(mGravity, 0.001f)) {
        return false;
    }

    if (isParallelDirection(mSlopeDownDir, -sDownDir, 0.01f)) {
        return false;
    }

    verticalizeVec(pDir, -sDownDir, mSlopeDownDir);
    normalize(pDir);
    return true;
}

}  // namespace al
