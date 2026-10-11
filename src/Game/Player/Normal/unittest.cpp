#include "Player/Normal/unittest.hpp"

#include <nerd/nerdMath.h>

#include "Library/Math/MathUtil.hpp"

namespace unittest {

    /**
     * Creates an invalid triangle with every vector zeroed.
     */
    Triangle::Triangle()
        : mIsValid(false), mPos0(sead::Vector3f::zero), mPos1(sead::Vector3f::zero),
          mPos2(sead::Vector3f::zero), mNormal(sead::Vector3f::zero), mIsHit(false),
          mHitDepth(0.0f), mHitPos(sead::Vector3f::zero), mHitRate(0.0f) {}

    /**
     * Sets the triangle's vertices and computes its face normal.
     * @param rPos0 First vertex.
     * @param rPos1 Second vertex.
     * @param rPos2 Third vertex.
     */
    void Triangle::set(const sead::Vector3f& rPos0, const sead::Vector3f& rPos1,
                       const sead::Vector3f& rPos2) {
        mPos0.set(rPos0);
        mPos1.set(rPos1);
        mPos2.set(rPos2);
        mNormal = (rPos1 - rPos0).cross(rPos2 - rPos0);
        al::normalize(&mNormal);
        mIsValid = true;
    }

    /**
     * Checks a sphere against the triangle.
     * @param rPos Sphere center.
     * @param radius Sphere radius.
     * @return Whether the sphere touches the triangle's face.
     */
    bool Triangle::check(const sead::Vector3f& rPos, f32 radius) {
        mIsHit = false;

        if (!mIsValid) {
            return false;
        }

        f32 dist = (rPos - mPos0).dot(mNormal);

        if (dist < 0.0f || dist > radius) {
            return false;
        }

        sead::Vector3f projPos = rPos - mNormal * dist;

        if (!isInTriangle(projPos)) {
            return false;
        }

        mHitDepth = radius - dist;
        mHitPos = projPos;
        mIsHit = true;
        return true;
    }

    /**
     * Checks whether a position lies inside the triangle's edges.
     * @param rPos Position to test.
     * @return Whether the position is on the inner side of every edge.
     */
    bool Triangle::isInTriangle(const sead::Vector3f& rPos) const {
        return isPosInLeftSide(mPos0, mPos1, rPos) && isPosInLeftSide(mPos1, mPos2, rPos) &&
               isPosInLeftSide(mPos2, mPos0, rPos);
    }

    /**
     * Sweeps a sphere along a movement vector against the triangle.
     * @param rPos Sphere start position.
     * @param rMove Movement vector.
     * @param radius Sphere radius.
     * @return Whether the swept sphere hits the triangle.
     */
    bool Triangle::check(const sead::Vector3f& rPos, const sead::Vector3f& rMove, f32 radius) {
        mIsHit = false;

        if (!mIsValid) {
            return false;
        }

        sead::Vector3f offset = mNormal * radius;
        sead::Vector3f startDiff = rPos - (offset + mPos0);
        f32 startDist = mNormal.dot(startDiff);

        if (startDist >= 0.0f) {
            f32 endDist = mNormal.dot(startDiff + rMove);

            if (endDist < 0.0f) {
                f32 rate = startDist / (startDist - endDist);
                mIsHit = true;
                mHitRate = rate;
                mHitPos = rPos + rMove * rate - offset;
                return true;
            }
        }

        f32 rate;
        sead::Vector3f hitPos;
        sead::Vector3f side;

        if (checkCylinderAndLine(&rate, &hitPos, &side, mPos0, mPos1 - mPos0, radius, rPos,
                                 rMove)) {
            mIsHit = true;
            mHitRate = rate;
            mHitPos = rPos + rMove * rate;
            mHitPos += side * radius;
            return true;
        }

        if (checkCylinderAndLine(&rate, &hitPos, &side, mPos1, mPos2 - mPos1, radius, rPos,
                                 rMove)) {
            mIsHit = true;
            mHitRate = rate;
            mHitPos = rPos + rMove * rate;
            mHitPos += side * radius;
            return true;
        }

        if (checkCylinderAndLine(&rate, &hitPos, &side, mPos2, mPos0 - mPos2, radius, rPos,
                                 rMove)) {
            mIsHit = true;
            mHitRate = rate;
            mHitPos = rPos + rMove * rate;
            mHitPos += side * radius;
            return true;
        }

        if (checkSphereAndLine(&rate, &hitPos, mPos0, radius, rPos, rMove)) {
            mIsHit = true;
            mHitRate = rate;
            mHitPos = mPos0;
            return true;
        }

        if (checkSphereAndLine(&rate, &hitPos, mPos1, radius, rPos, rMove)) {
            mIsHit = true;
            mHitRate = rate;
            mHitPos = mPos1;
            return true;
        }

        if (checkSphereAndLine(&rate, &hitPos, mPos2, radius, rPos, rMove)) {
            mIsHit = true;
            mHitRate = rate;
            mHitPos = mPos2;
            return true;
        }

        return false;
    }

    /**
     * Checks a line segment against a finite cylinder around a triangle edge.
     * @param pRate Receives the hit rate along the movement.
     * @param pHitPos Receives the line position at the hit.
     * @param pSide Receives the direction perpendicular to both the edge and the movement.
     * @param rStart Edge start position.
     * @param rAxis Edge vector.
     * @param radius Cylinder radius.
     * @param rPos Line start position.
     * @param rMove Line movement vector.
     * @return Whether the line hits the cylinder within the edge.
     */
    bool Triangle::checkCylinderAndLine(f32* pRate, sead::Vector3f* pHitPos,
                                        sead::Vector3f* pSide, const sead::Vector3f& rStart,
                                        const sead::Vector3f& rAxis, f32 radius,
                                        const sead::Vector3f& rPos,
                                        const sead::Vector3f& rMove) {
        *pSide = rAxis.cross(rMove);

        if (al::normalizeOrZero(pSide)) {
            return false;
        }

        sead::Vector3f diff = rStart - rPos;
        f32 sideDist = diff.dot(*pSide);
        f32 absSideDist = sideDist > 0.0f ? sideDist : -sideDist;

        if (absSideDist > radius) {
            return false;
        }

        sead::Vector3f axisDir = rAxis;
        al::normalize(&axisDir);

        sead::Vector3f vertDiff;
        al::verticalizeVec(&vertDiff, axisDir, diff);

        if (nerd::sqrt(vertDiff.squaredLength()) <= radius) {
            return false;
        }

        sead::Vector3f vertMove;
        al::verticalizeVec(&vertMove, axisDir, rMove);
        sead::Vector3f vertMoveDir = vertMove;
        al::normalize(&vertMoveDir);
        f32 approach = vertDiff.dot(vertMoveDir);

        if (approach < 0.0f) {
            return false;
        }

        f32 dist = approach - nerd::sqrt(radius * radius - sideDist * sideDist);
        f32 rate = dist / nerd::sqrt(vertMove.squaredLength());
        *pRate = rate;

        if (rate < 0.0f || rate > 1.0f) {
            return false;
        }

        *pHitPos = rPos + rMove * rate;
        f32 axisPos = (*pHitPos - rStart).dot(axisDir);

        if (!(axisPos < 0.0f) && !(axisPos > nerd::sqrt(rAxis.squaredLength()))) {
            return true;
        }

        return false;
    }

    /**
     * Checks a line segment against a sphere.
     * @param pRate Receives the hit rate along the movement.
     * @param pHitPos Receives the line position at the hit.
     * @param rCenter Sphere center.
     * @param radius Sphere radius.
     * @param rPos Line start position.
     * @param rMove Line movement vector.
     * @return Whether the line hits the sphere within the movement.
     */
    bool Triangle::checkSphereAndLine(f32* pRate, sead::Vector3f* pHitPos,
                                      const sead::Vector3f& rCenter, f32 radius,
                                      const sead::Vector3f& rPos, const sead::Vector3f& rMove) {
        sead::Vector3f moveDir = rMove;

        if (al::normalizeOrZero(&moveDir)) {
            return false;
        }

        sead::Vector3f diff = rCenter - rPos;
        f32 approach = diff.dot(moveDir);

        if (approach < 0.0f) {
            return false;
        }

        f32 sqDist = diff.squaredLength() - approach * approach;
        f32 dist = approach - nerd::sqrt(radius * radius - sqDist);
        *pRate = dist / nerd::sqrt(rMove.squaredLength());
        *pHitPos = rPos + rMove * *pRate;
        return *pRate >= 0.0f && *pRate <= 1.0f;
    }

    /**
     * Checks a line segment against the triangle.
     * @param rPos Line start position.
     * @param rMove Line movement vector.
     * @return Whether the line crosses the triangle.
     */
    bool Triangle::check(const sead::Vector3f& rPos, const sead::Vector3f& rMove) {
        mIsHit = false;

        if (!mIsValid) {
            return false;
        }

        sead::Vector3f startDiff = rPos - mPos0;
        f32 startDist = startDiff.dot(mNormal);
        f32 endDist = (startDiff + rMove).dot(mNormal);

        if (startDist < 0.0f || endDist > 0.0f) {
            return false;
        }

        sead::Vector3f hitMove = rMove * (startDist / (startDist - endDist));
        sead::Vector3f hitPos = rPos + hitMove;

        if (!isInTriangle(hitPos)) {
            return false;
        }

        mHitDepth = nerd::sqrt(rMove.squaredLength()) - nerd::sqrt(hitMove.squaredLength());
        mHitPos = hitPos;
        mIsHit = true;
        return true;
    }

    /**
     * Checks whether a position is on the left side of an edge, seen along the face normal.
     * @param rStart Edge start position.
     * @param rEnd Edge end position.
     * @param rPos Position to test.
     * @return Whether the position is on the left side (or on the edge).
     */
    bool Triangle::isPosInLeftSide(const sead::Vector3f& rStart, const sead::Vector3f& rEnd,
                                   const sead::Vector3f& rPos) const {
        return mNormal.dot((rEnd - rStart).cross(rPos - rStart)) >= 0.0f;
    }

}  // namespace unittest

/**
 * Creates an empty triangle soup.
 */
TestTriangleSoup::TestTriangleSoup() : mNumTriangles(0), _1304(0), _1308(0), _130C(0) {}

/**
 * Appends a triangle to the soup.
 * @param rPos0 First vertex.
 * @param rPos1 Second vertex.
 * @param rPos2 Third vertex.
 */
void TestTriangleSoup::appendTriangle(const sead::Vector3f& rPos0, const sead::Vector3f& rPos1,
                                      const sead::Vector3f& rPos2) {
    mTriangles[mNumTriangles].set(rPos0, rPos1, rPos2);
    mNumTriangles++;
}

/**
 * Appends a quad to the soup as two triangles.
 * @param rPos0 First vertex.
 * @param rPos1 Second vertex.
 * @param rPos2 Third vertex.
 * @param rPos3 Fourth vertex.
 */
void TestTriangleSoup::appendQuad(const sead::Vector3f& rPos0, const sead::Vector3f& rPos1,
                                  const sead::Vector3f& rPos2, const sead::Vector3f& rPos3) {
    mTriangles[mNumTriangles++].set(rPos0, rPos1, rPos2);
    mTriangles[mNumTriangles++].set(rPos0, rPos2, rPos3);
}
