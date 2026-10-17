#include "Project/AreaObj/AreaShapeCube.hpp"
#include "Project/AreaObj/AreaShapeRound.hpp"

#include <math/seadMathCalcCommon.h>

#include "Library/Math/MathUtil.hpp"

namespace al {
/**
 * Constructs a cube of 1000 units per side.
 * @param originType whether the origin is the center or the bottom of the cube
 */
AreaShapeCube::AreaShapeCube(OriginType originType) : mOriginType(originType) {}

/**
 * Gets the bounding box of the cube in unscaled shape space.
 * @param pBox output box
 * @return true
 */
bool AreaShapeCube::calcLocalBoundingBox(sead::BoundBox3f* pBox) const {
    pBox->setUndef();
    sead::Vector3f min(-500.0f, calcBottom(), -500.0f);
    sead::Vector3f max(500.0f, calcTop(), 500.0f);
    pBox->addPoint(min);
    pBox->addPoint(max);
    return true;
}

/**
 * Resets a bounding box; the world bounding box of a cube is not calculated.
 * @param pBox output box
 * @return true
 */
bool AreaShapeCube::calcWorldBoundingBox(sead::BoundBox3f* pBox) const {
    pBox->setUndef();
    return true;
}

/**
 * Checks whether a world position is inside the cube.
 * @param rPos world position
 * @return true if inside
 */
bool AreaShapeCube::isInVolume(const sead::Vector3f& rPos) const {
    sead::Vector3f localPos = sead::Vector3f::zero;
    calcLocalPos(&localPos, rPos);
    return isInLocalVolume(localPos);
}

/**
 * Does nothing; cubes do not calculate near points.
 * @param pOut output position
 * @param rPos world position
 */
void AreaShapeCube::calcNearPoint(sead::Vector3f* pOut, const sead::Vector3f& rPos) const {}

/**
 * Calculates the nearest point on the surface of the cube.
 * @param pOut output world position
 * @param rPos world position
 * @return false if the position is at the center of the cube
 */
bool AreaShapeCube::calcNearestEdgePoint(sead::Vector3f* pOut, const sead::Vector3f& rPos) const {
    f32 bottom = calcBottom();
    f32 top = calcTop();

    sead::Vector3f localPos(0.0f, 0.0f, 0.0f);
    calcLocalPos(&localPos, rPos);

    if (isInLocalVolume(localPos)) {
        if (isNearZero(localPos, 0.001f)) {
            return false;
        }

        f32 rateX = calcRate01(localPos.x, -500.0f, 500.0f) - 0.5f;
        f32 rateY = calcRate01(localPos.y, bottom, top) - 0.5f;
        f32 rateZ = calcRate01(localPos.z, -500.0f, 500.0f) - 0.5f;

        f32 absX = sead::Mathf::abs(rateX);
        f32 absY = sead::Mathf::abs(rateY);
        f32 absZ = sead::Mathf::abs(rateZ);

        s32 axis;

        if (absX > absY) {
            axis = absX > absZ ? 0 : 2;
        } else {
            axis = absY > absZ ? 1 : 2;
        }

        switch (axis) {
        case 0:
            localPos.x = rateX < 0.0f ? -500.0f : 500.0f;
            break;
        case 1:
            localPos.y = rateY < 0.0f ? bottom : top;
            break;
        case 2:
            localPos.z = rateZ < 0.0f ? -500.0f : 500.0f;
            break;
        }
    } else {
        if (sead::Mathf::abs(localPos.x) >= 500.0f) {
            localPos.x = localPos.x < 0.0f ? -500.0f : 500.0f;
        }

        if (localPos.y <= bottom) {
            localPos.y = bottom;
        } else if (localPos.y >= top) {
            localPos.y = top;
        }

        if (sead::Mathf::abs(localPos.z) >= 500.0f) {
            localPos.z = localPos.z < 0.0f ? -500.0f : 500.0f;
        }
    }

    calcWorldPos(pOut, localPos);
    return true;
}

/**
 * Checks whether an unscaled shape space position is inside the cube.
 * @param rPos local position
 * @return true if inside
 */
bool AreaShapeCube::isInLocalVolume(const sead::Vector3f& rPos) const {
    sead::Vector3f min = {-500.0f, calcBottom(), -500.0f};
    f32 y = rPos.y;
    sead::Vector3f max = {500.0f, calcTop(), 500.0f};

    if (y < min.y || max.y < y) {
        return false;
    }

    if (rPos.x < min.x || max.x < rPos.x) {
        return false;
    }

    if (rPos.z < min.z || max.z < rPos.z) {
        return false;
    }

    return true;
}

/**
 * Checks whether a segment hits a face of the cube.
 * @param pHitPos output hit position
 * @param pHitNormal output face normal
 * @param rStart segment start
 * @param rEnd segment end
 * @return true on hit
 */
bool AreaShapeCube::checkArrowCollision(sead::Vector3f* pHitPos, sead::Vector3f* pHitNormal, const sead::Vector3f& rStart,
                                        const sead::Vector3f& rEnd) const {
    f32 bottom = calcBottom();
    f32 top = calcTop();

    sead::Vector3f localStart = sead::Vector3f::zero;
    calcLocalPos(&localStart, rStart);
    sead::Vector3f localEnd = sead::Vector3f::zero;
    calcLocalPos(&localEnd, rEnd);

    sead::Vector3f dir = localEnd - localStart;
    sead::Vector3f hitPos;

    if (isInLocalVolume(localStart)) {
        if (dir.y > 0.0f) {
            f32 t = (top - localStart.y) / dir.y;

            if (0.0f <= t && t <= 1.0f) {
                sead::Vector3f scaledDir = dir * t;
                hitPos = scaledDir + localStart;

                if (-500.0f <= hitPos.x && hitPos.x <= 500.0f && -500.0f <= hitPos.z && hitPos.z <= 500.0f) {
                    calcWorldPos(pHitPos, hitPos);

                    if (pHitNormal != nullptr) {
                        calcWorldDir(pHitNormal, sead::Vector3f::ey);
                    }

                    return true;
                }
            }
        } else if (dir.y < 0.0f) {
            f32 t = (bottom - localStart.y) / dir.y;

            if (0.0f <= t && t <= 1.0f) {
                sead::Vector3f scaledDir = dir * t;
                hitPos = scaledDir + localStart;

                if (-500.0f <= hitPos.x && hitPos.x <= 500.0f && -500.0f <= hitPos.z && hitPos.z <= 500.0f) {
                    calcWorldPos(pHitPos, hitPos);

                    if (pHitNormal != nullptr) {
                        calcWorldDir(pHitNormal, -sead::Vector3f::ey);
                    }

                    return true;
                }
            }
        }

        if (dir.z > 0.0f) {
            f32 t = (500.0f - localStart.z) / dir.z;

            if (0.0f <= t && t <= 1.0f) {
                sead::Vector3f scaledDir = dir * t;
                hitPos = scaledDir + localStart;

                if (-500.0f <= hitPos.x && hitPos.x <= 500.0f && hitPos.y >= bottom && hitPos.y <= top) {
                    calcWorldPos(pHitPos, hitPos);

                    if (pHitNormal != nullptr) {
                        calcWorldDir(pHitNormal, sead::Vector3f::ez);
                    }

                    return true;
                }
            }
        } else if (dir.z < 0.0f) {
            f32 t = (-500.0f - localStart.z) / dir.z;

            if (0.0f <= t && t <= 1.0f) {
                sead::Vector3f scaledDir = dir * t;
                hitPos = scaledDir + localStart;

                if (-500.0f <= hitPos.x && hitPos.x <= 500.0f && hitPos.y >= bottom && hitPos.y <= top) {
                    calcWorldPos(pHitPos, hitPos);

                    if (pHitNormal != nullptr) {
                        calcWorldDir(pHitNormal, -sead::Vector3f::ez);
                    }

                    return true;
                }
            }
        }

        if (dir.x > 0.0f) {
            f32 t = (500.0f - localStart.x) / dir.x;

            if (0.0f <= t && t <= 1.0f) {
                sead::Vector3f scaledDir = dir * t;
                hitPos = scaledDir + localStart;

                if (-500.0f <= hitPos.z && hitPos.y >= bottom && hitPos.z <= 500.0f && hitPos.y <= top) {
                    calcWorldPos(pHitPos, hitPos);

                    if (pHitNormal != nullptr) {
                        calcWorldDir(pHitNormal, sead::Vector3f::ex);
                    }

                    return true;
                }
            }
        } else if (dir.x < 0.0f) {
            f32 t = (-500.0f - localStart.x) / dir.x;

            if (0.0f <= t && t <= 1.0f) {
                sead::Vector3f scaledDir = dir * t;
                hitPos = scaledDir + localStart;

                if (-500.0f <= hitPos.z && hitPos.y >= bottom && hitPos.z <= 500.0f && hitPos.y <= top) {
                    calcWorldPos(pHitPos, hitPos);

                    if (pHitNormal != nullptr) {
                        calcWorldDir(pHitNormal, sead::Vector3f::ey);
                    }

                    return true;
                }
            }
        }

        return false;
    }

    if (dir.y > 0.0f) {
        f32 t = (bottom - localStart.y) / dir.y;

        if (0.0f <= t && t <= 1.0f) {
            sead::Vector3f scaledDir = dir * t;
            hitPos = scaledDir + localStart;

            if (-500.0f <= hitPos.x && hitPos.x <= 500.0f && -500.0f <= hitPos.z && hitPos.z <= 500.0f) {
                calcWorldPos(pHitPos, hitPos);

                if (pHitNormal != nullptr) {
                    calcWorldDir(pHitNormal, -sead::Vector3f::ey);
                }

                return true;
            }
        }
    } else if (dir.y < 0.0f) {
        f32 t = (top - localStart.y) / dir.y;

        if (0.0f <= t && t <= 1.0f) {
            sead::Vector3f scaledDir = dir * t;
            hitPos = scaledDir + localStart;

            if (-500.0f <= hitPos.x && hitPos.x <= 500.0f && -500.0f <= hitPos.z && hitPos.z <= 500.0f) {
                calcWorldPos(pHitPos, hitPos);

                if (pHitNormal != nullptr) {
                    calcWorldDir(pHitNormal, sead::Vector3f::ey);
                }

                return true;
            }
        }
    }

    if (dir.z > 0.0f) {
        f32 t = (-500.0f - localStart.z) / dir.z;

        if (0.0f <= t && t <= 1.0f) {
            sead::Vector3f scaledDir = dir * t;
            hitPos = scaledDir + localStart;

            if (-500.0f <= hitPos.x && hitPos.x <= 500.0f && hitPos.y >= bottom && hitPos.y <= top) {
                calcWorldPos(pHitPos, hitPos);

                if (pHitNormal != nullptr) {
                    calcWorldDir(pHitNormal, -sead::Vector3f::ez);
                }

                return true;
            }
        }
    } else if (dir.z < 0.0f) {
        f32 t = (500.0f - localStart.z) / dir.z;

        if (0.0f <= t && t <= 1.0f) {
            sead::Vector3f scaledDir = dir * t;
            hitPos = scaledDir + localStart;

            if (-500.0f <= hitPos.x && hitPos.x <= 500.0f && hitPos.y >= bottom && hitPos.y <= top) {
                calcWorldPos(pHitPos, hitPos);

                if (pHitNormal != nullptr) {
                    calcWorldDir(pHitNormal, sead::Vector3f::ez);
                }

                return true;
            }
        }
    }

    if (dir.x > 0.0f) {
        f32 t = (-500.0f - localStart.x) / dir.x;

        if (0.0f <= t && t <= 1.0f) {
            sead::Vector3f scaledDir = dir * t;
            hitPos = scaledDir + localStart;

            if (-500.0f <= hitPos.z && hitPos.y >= bottom && hitPos.z <= 500.0f && hitPos.y <= top) {
                calcWorldPos(pHitPos, hitPos);

                if (pHitNormal != nullptr) {
                    calcWorldDir(pHitNormal, -sead::Vector3f::ex);
                }

                return true;
            }
        }
    } else if (dir.x < 0.0f) {
        f32 t = (500.0f - localStart.x) / dir.x;

        if (0.0f <= t && t <= 1.0f) {
            sead::Vector3f scaledDir = dir * t;
            hitPos = scaledDir + localStart;

            if (-500.0f <= hitPos.z && hitPos.y >= bottom && hitPos.z <= 500.0f && hitPos.y <= top) {
                calcWorldPos(pHitPos, hitPos);

                if (pHitNormal != nullptr) {
                    calcWorldDir(pHitNormal, -sead::Vector3f::ex);
                }

                return true;
            }
        }
    }

    return false;
}

/**
 * Constructs a sphere of radius 500 scaled by the x scale.
 */
AreaShapeSphere::AreaShapeSphere() = default;

/**
 * Checks whether a world position is inside the sphere.
 * @param rPos world position
 * @return true if inside
 */
bool AreaShapeSphere::isInVolume(const sead::Vector3f& rPos) const {
    sead::Vector3f trans;
    calcTrans(&trans);
    sead::Vector3f diff = rPos - trans;
    f32 radius = mScale.x * 500.0f;
    return diff.squaredLength() <= radius * radius;
}

/**
 * Does nothing; spheres do not calculate near points.
 * @param pOut output position
 * @param rPos world position
 */
void AreaShapeSphere::calcNearPoint(sead::Vector3f* pOut, const sead::Vector3f& rPos) const {}

/**
 * Calculates the nearest point on the surface of the sphere.
 * @param pOut output world position
 * @param rPos world position
 * @return true
 */
bool AreaShapeSphere::calcNearestEdgePoint(sead::Vector3f* pOut,
                                           const sead::Vector3f& rPos) const {
    sead::Vector3f localPos = sead::Vector3f::zero;
    calcLocalPos(&localPos, rPos);
    f32 length = localPos.length();

    if (length > 0.0f) {
        localPos *= 500.0f / length;
    }

    calcWorldPos(pOut, localPos);
    return true;
}

/**
 * Does not check segments against spheres.
 * @param pHitPos output hit position
 * @param pNormal output normal
 * @param rStart segment start
 * @param rEnd segment end
 * @return false
 */
bool AreaShapeSphere::checkArrowCollision(sead::Vector3f* pHitPos, sead::Vector3f* pNormal,
                                          const sead::Vector3f& rStart,
                                          const sead::Vector3f& rEnd) const {
    return false;
}

/**
 * Does not calculate the world bounding box of spheres.
 * @param pBox output box
 * @return false
 */
bool AreaShapeSphere::calcWorldBoundingBox(sead::BoundBox3f* pBox) const {
    return false;
}

/**
 * Constructs an ellipsoid of radius 500 in unscaled shape space.
 */
AreaShapeOval::AreaShapeOval() = default;

/**
 * Checks whether a world position is inside the ellipsoid.
 * @param rPos world position
 * @return true if inside
 */
bool AreaShapeOval::isInVolume(const sead::Vector3f& rPos) const {
    sead::Vector3f localPos = sead::Vector3f::zero;
    calcLocalPos(&localPos, rPos);
    return localPos.squaredLength() <= 500.0f * 500.0f;
}

/**
 * Does nothing; ellipsoids do not calculate near points.
 * @param pOut output position
 * @param rPos world position
 */
void AreaShapeOval::calcNearPoint(sead::Vector3f* pOut, const sead::Vector3f& rPos) const {}

/**
 * Calculates the point on the surface of the ellipsoid in the direction of a position.
 * @param pOut output world position
 * @param rPos world position
 * @return true
 */
bool AreaShapeOval::calcNearestEdgePoint(sead::Vector3f* pOut, const sead::Vector3f& rPos) const {
    sead::Vector3f localPos = sead::Vector3f::zero;
    calcLocalPos(&localPos, rPos);
    f32 length = localPos.length();

    if (length > 0.0f) {
        localPos *= 500.0f / length;
    }

    calcWorldPos(pOut, localPos);
    return true;
}

/**
 * Does not check segments against ellipsoids.
 * @param pHitPos output hit position
 * @param pNormal output normal
 * @param rStart segment start
 * @param rEnd segment end
 * @return false
 */
bool AreaShapeOval::checkArrowCollision(sead::Vector3f* pHitPos, sead::Vector3f* pNormal,
                                        const sead::Vector3f& rStart,
                                        const sead::Vector3f& rEnd) const {
    return false;
}

/**
 * Does not calculate the world bounding box of ellipsoids.
 * @param pBox output box
 * @return false
 */
bool AreaShapeOval::calcWorldBoundingBox(sead::BoundBox3f* pBox) const {
    return false;
}

/**
 * Constructs a cylinder of radius 500 and height 500 standing on its origin.
 */
AreaShapeCylinder::AreaShapeCylinder() = default;

/**
 * Checks whether a world position is inside the cylinder.
 * @param rPos world position
 * @return true if inside
 */
bool AreaShapeCylinder::isInVolume(const sead::Vector3f& rPos) const {
    sead::Vector3f localPos = sead::Vector3f::zero;
    calcLocalPos(&localPos, rPos);

    if (localPos.y < 0.0f || localPos.y > 500.0f) {
        return false;
    }

    return localPos.x * localPos.x + localPos.z * localPos.z <= 500.0f * 500.0f;
}

/**
 * Calculates the nearest point inside the cylinder.
 * @param pOut output world position
 * @param rPos world position
 */
void AreaShapeCylinder::calcNearPoint(sead::Vector3f* pOut, const sead::Vector3f& rPos) const {
    sead::Vector3f localPos = sead::Vector3f::zero;
    calcLocalPos(&localPos, rPos);

    sead::Vector3f nearPos = sead::Vector3f::zero;
    nearPos.y = sead::Mathf::clamp(localPos.y, 0.0f, 500.0f);
    f32 lengthSq = localPos.x * localPos.x + localPos.z * localPos.z;

    if (lengthSq <= 500.0f * 500.0f) {
        nearPos.x = localPos.x;
        nearPos.z = localPos.z;
    } else {
        f32 rate = 500.0f / sead::Mathf::sqrt(lengthSq);
        nearPos.x = localPos.x * rate;
        nearPos.z = rate * localPos.z;
    }

    calcWorldPos(pOut, nearPos);
}

/**
 * Calculates the nearest point inside the cylinder.
 * @param pOut output world position
 * @param rPos world position
 * @return true
 */
bool AreaShapeCylinder::calcNearestEdgePoint(sead::Vector3f* pOut,
                                             const sead::Vector3f& rPos) const {
    sead::Vector3f localPos = sead::Vector3f::zero;
    calcLocalPos(&localPos, rPos);

    sead::Vector3f nearPos = sead::Vector3f::zero;
    nearPos.y = sead::Mathf::clamp(localPos.y, 0.0f, 500.0f);
    f32 lengthSq = localPos.x * localPos.x + localPos.z * localPos.z;

    if (lengthSq <= 500.0f * 500.0f) {
        nearPos.x = localPos.x;
        nearPos.z = localPos.z;
    } else {
        f32 rate = 500.0f / sead::Mathf::sqrt(lengthSq);
        nearPos.x = localPos.x * rate;
        nearPos.z = rate * localPos.z;
    }

    calcWorldPos(pOut, nearPos);
    return true;
}

/**
 * Does not calculate the world bounding box of cylinders.
 * @param pBox output box
 * @return false
 */
bool AreaShapeCylinder::calcWorldBoundingBox(sead::BoundBox3f* pBox) const {
    return false;
}

/**
 * Does not check segments against cylinders.
 * @param pHitPos output hit position
 * @param pNormal output normal
 * @param rStart segment start
 * @param rEnd segment end
 * @return false
 */
bool AreaShapeCylinder::checkArrowCollision(sead::Vector3f* pHitPos, sead::Vector3f* pNormal,
                                            const sead::Vector3f& rStart,
                                            const sead::Vector3f& rEnd) const {
    return false;
}

/**
 * Does not calculate the local bounding box of spheres.
 * @param pBox output box
 * @return false
 */
bool AreaShapeSphere::calcLocalBoundingBox(sead::BoundBox3f* pBox) const {
    return false;
}

/**
 * Does not calculate the local bounding box of ellipsoids.
 * @param pBox output box
 * @return false
 */
bool AreaShapeOval::calcLocalBoundingBox(sead::BoundBox3f* pBox) const {
    return false;
}

/**
 * Does not calculate the local bounding box of cylinders.
 * @param pBox output box
 * @return false
 */
bool AreaShapeCylinder::calcLocalBoundingBox(sead::BoundBox3f* pBox) const {
    return false;
}
}  // namespace al
