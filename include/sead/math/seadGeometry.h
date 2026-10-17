#pragma once

#include <math/seadBoundBox.h>
#include <math/seadVector.h>

namespace sead
{
template <typename T>
class Line
{
public:
    Line() = default;
    Line(const T& rPos, const T& rDir) : mPos(rPos), mDir(rDir) {}

    const T& getPos() const { return mPos; }
    const T& getDir() const { return mDir; }

    void setPos(const T& rPos) { mPos = rPos; }
    void setDir(const T& rDir) { mDir = rDir; }

private:
    T mPos;
    T mDir;
};

template <typename T>
class Ray
{
public:
    Ray() = default;
    Ray(const T& rPos, const T& rDir) : mPos(rPos), mDir(rDir) {}

    const T& getPos() const { return mPos; }
    const T& getDir() const { return mDir; }

    void setPos(const T& rPos) { mPos = rPos; }
    void setDir(const T& rDir) { mDir = rDir; }
    void setPosDir(const T& rPos, const T& rDir)
    {
        mPos = rPos;
        mDir = rDir;
    }

private:
    T mPos;
    T mDir;
};

template <typename T>
class Segment
{
public:
    Segment() = default;
    Segment(const T& rPos0, const T& rPos1) : mP0(rPos0), mP1(rPos1) {}

    const T& getPos0() const { return mP0; }
    const T& getPos1() const { return mP1; }

    void setPos0(const T& rPos) { mP0 = rPos; }
    void setPos1(const T& rPos) { mP1 = rPos; }

private:
    T mP0;
    T mP1;
};

template <typename T>
class Sphere
{
public:
    using Scalar = decltype(T::x);

    Sphere() = default;
    Sphere(const T& rCenter, Scalar radius) : mCenter(rCenter), mRadius(radius) {}

    const T& getCenter() const { return mCenter; }
    Scalar getRadius() const { return mRadius; }

    void setCenter(const T& rCenter) { mCenter = rCenter; }
    void setRadius(Scalar radius) { mRadius = radius; }

private:
    T mCenter;
    Scalar mRadius;
};

template <typename T>
class Capsule
{
public:
    using Scalar = decltype(T::x);

    Capsule() = default;
    Capsule(const T& rPos0, const T& rPos1, Scalar radius) : mSegment(rPos0, rPos1), mRadius(radius)
    {
    }

    const Segment<T>& getSegment() const { return mSegment; }
    Scalar getRadius() const { return mRadius; }

private:
    Segment<T> mSegment;
    Scalar mRadius;
};

template <typename T>
class Plane2
{
public:
    Plane2() = default;
    Plane2(const Vector2<T>& rNormal, T d) : mNormal(rNormal), mD(d) {}

    const Vector2<T>& getNormal() const { return mNormal; }
    T getD() const { return mD; }

private:
    Vector2<T> mNormal;
    T mD;
};

template <typename T>
class Plane3
{
public:
    Plane3() = default;
    Plane3(const Vector3<T>& rNormal, T d) : mNormal(rNormal), mD(d) {}
    Plane3(const Vector3<T>& rNormal, const Vector3<T>& rPoint) { mNormal = rNormal; mD = rPoint.dot(Vector3<T>(rNormal)); }

    const Vector3<T>& getNormal() const { return mNormal; }
    T getD() const { return mD; }
    void setD(T d) { mD = d; }

private:
    Vector3<T> mNormal;
    T mD;
};

using Plane3f = Plane3<f32>;

class Geometry
{
public:
    static f32 calcSquaredDistancePointToLine(const Vector2f& rPoint, const Line<Vector2f>& rLine,
                                              f32* pT);
    static f32 calcSquaredDistancePointToLine(const Vector3f& rPoint, const Line<Vector3f>& rLine,
                                              f32* pT);
    static f32 calcSquaredDistancePointToRay(const Vector2f& rPoint, const Ray<Vector2f>& rRay,
                                             f32* pT);
    static f32 calcSquaredDistancePointToRay(const Vector3f& rPoint, const Ray<Vector3f>& rRay,
                                             f32* pT);
    static f32 calcSquaredDistancePointToSegment(const Vector2f& rPoint,
                                                 const Segment<Vector2f>& rSegment, f32* pT);
    static f32 calcSquaredDistancePointToSegment(const Vector3f& rPoint,
                                                 const Segment<Vector3f>& rSegment, f32* pT);
    static f32 calcSquaredDistancePointToPlane(const Vector2f& rPoint, const Plane2<f32>& rPlane);
    static f32 calcSquaredDistancePointToPlane(const Vector3f& rPoint, const Plane3<f32>& rPlane);
    static f32 calcSquaredDistanceSphereToPlane(const Sphere<Vector2f>& rSphere,
                                                const Plane2<f32>& rPlane);
    static f32 calcSquaredDistanceSphereToPlane(const Sphere<Vector3f>& rSphere,
                                                const Plane3<f32>& rPlane);
    static f32 calcSquaredDistancePointToAABB(const Vector2f& rPoint, const BoundBox2<f32>& rBox,
                                              Vector2f* pClosest);
    static f32 calcSquaredDistancePointToAABB(const Vector3f& rPoint, const BoundBox3<f32>& rBox,
                                              Vector3f* pClosest);
    static f32 calcSquaredDistanceLineToLine(const Line<Vector2f>& rLine0,
                                             const Line<Vector2f>& rLine1, f32* pT0, f32* pT1);
    static f32 calcSquaredDistanceLineToLine(const Line<Vector3f>& rLine0,
                                             const Line<Vector3f>& rLine1, f32* pT0, f32* pT1);
    static f32 calcSquaredDistanceRayToRay(const Ray<Vector2f>& rRay0, const Ray<Vector2f>& rRay1,
                                           f32* pT0, f32* pT1);
    static f32 calcSquaredDistanceRayToRay(const Ray<Vector3f>& rRay0, const Ray<Vector3f>& rRay1,
                                           f32* pT0, f32* pT1);
    static f32 calcSquaredDistanceSegmentToSegment(const Segment<Vector2f>& rSegment0,
                                                   const Segment<Vector2f>& rSegment1, f32* pT0,
                                                   f32* pT1);
    static f32 calcSquaredDistanceSegmentToSegment(const Segment<Vector3f>& rSegment0,
                                                   const Segment<Vector3f>& rSegment1, f32* pT0,
                                                   f32* pT1);
    static f32 calcSquaredDistanceLineToSegment(const Line<Vector2f>& rLine,
                                                const Segment<Vector2f>& rSegment, f32* pT0,
                                                f32* pT1);
    static f32 calcSquaredDistanceLineToSegment(const Line<Vector3f>& rLine,
                                                const Segment<Vector3f>& rSegment, f32* pT0,
                                                f32* pT1);
    static f32 calcSquaredDistanceRayToSegment(const Ray<Vector2f>& rRay,
                                               const Segment<Vector2f>& rSegment, f32* pT0,
                                               f32* pT1);
    static f32 calcSquaredDistanceRayToSegment(const Ray<Vector3f>& rRay,
                                               const Segment<Vector3f>& rSegment, f32* pT0,
                                               f32* pT1);

    static s32 calcIntersectionLineToPlane(const Line<Vector2f>& rLine, const Plane2<f32>& rPlane,
                                           f32* pT);
    static s32 calcIntersectionLineToPlane(const Line<Vector3f>& rLine, const Plane3<f32>& rPlane,
                                           f32* pT);
    static s32 calcIntersectionRayToPlane(const Ray<Vector2f>& rRay, const Plane2<f32>& rPlane,
                                          f32* pT);
    static s32 calcIntersectionRayToPlane(const Ray<Vector3f>& rRay, const Plane3<f32>& rPlane,
                                          f32* pT);
    static s32 calcIntersectionSegmentToPlane(const Segment<Vector2f>& rSegment,
                                              const Plane2<f32>& rPlane, f32* pT);
    static s32 calcIntersectionSegmentToPlane(const Segment<Vector3f>& rSegment,
                                              const Plane3<f32>& rPlane, f32* pT);
    static s32 calcIntersectionLineToSphere(const Line<Vector2f>& rLine,
                                            const Sphere<Vector2f>& rSphere, f32* pT0, f32* pT1);
    static s32 calcIntersectionLineToSphere(const Line<Vector3f>& rLine,
                                            const Sphere<Vector3f>& rSphere, f32* pT0, f32* pT1);
    static s32 calcIntersectionRayToSphere(const Ray<Vector2f>& rRay,
                                           const Sphere<Vector2f>& rSphere, f32* pT0, f32* pT1);
    static s32 calcIntersectionRayToSphere(const Ray<Vector3f>& rRay,
                                           const Sphere<Vector3f>& rSphere, f32* pT0, f32* pT1);
    static s32 calcIntersectionSegmentToSphere(const Segment<Vector2f>& rSegment,
                                               const Sphere<Vector2f>& rSphere, f32* pT0, f32* pT1);
    static s32 calcIntersectionSegmentToSphere(const Segment<Vector3f>& rSegment,
                                               const Sphere<Vector3f>& rSphere, f32* pT0, f32* pT1);
    static bool calcIntersectionLineToAABB(const Line<Vector2f>& rLine, const BoundBox2<f32>& rBox,
                                           f32* pT0, f32* pT1);
    static bool calcIntersectionLineToAABB(const Line<Vector3f>& rLine, const BoundBox3<f32>& rBox,
                                           f32* pT0, f32* pT1);
    static bool calcIntersectionRayToAABB(const Ray<Vector2f>& rRay, const BoundBox2<f32>& rBox,
                                          f32* pT0, f32* pT1);
    static bool calcIntersectionRayToAABB(const Ray<Vector3f>& rRay, const BoundBox3<f32>& rBox,
                                          f32* pT0, f32* pT1);
    static bool calcIntersectionSegmentToAABB(const Segment<Vector2f>& rSegment,
                                              const BoundBox2<f32>& rBox, f32* pT0, f32* pT1);
    static bool calcIntersectionSegmentToAABB(const Segment<Vector3f>& rSegment,
                                              const BoundBox3<f32>& rBox, f32* pT0, f32* pT1);
    static bool calcIntersectionSphereToAABB(const Sphere<Vector2f>& rSphere,
                                             const BoundBox2<f32>& rBox);
    static bool calcIntersectionSphereToAABB(const Sphere<Vector3f>& rSphere,
                                             const BoundBox3<f32>& rBox);
    static bool calcIntersectionSphereToSphere(const Sphere<Vector2f>& rSphere0,
                                               const Sphere<Vector2f>& rSphere1);
    static bool calcIntersectionSphereToSphere(const Sphere<Vector3f>& rSphere0,
                                               const Sphere<Vector3f>& rSphere1);
    static bool calcIntersectionPlaneToAABB(const Plane2<f32>& rPlane, const BoundBox2<f32>& rBox);
    static bool calcIntersectionPlaneToAABB(const Plane3<f32>& rPlane, const BoundBox3<f32>& rBox);
    static bool calcIntersectionCapsuleToCapsule(const Capsule<Vector2f>& rCapsule0,
                                                 const Capsule<Vector2f>& rCapsule1);
    static bool calcIntersectionCapsuleToCapsule(const Capsule<Vector3f>& rCapsule0,
                                                 const Capsule<Vector3f>& rCapsule1);
    static bool calcIntersectionLineToCapsule(const Line<Vector2f>& rLine,
                                              const Capsule<Vector2f>& rCapsule);
    static bool calcIntersectionLineToCapsule(const Line<Vector3f>& rLine,
                                              const Capsule<Vector3f>& rCapsule);
    static bool calcIntersectionRayToCapsule(const Ray<Vector2f>& rRay,
                                             const Capsule<Vector2f>& rCapsule);
    static bool calcIntersectionRayToCapsule(const Ray<Vector3f>& rRay,
                                             const Capsule<Vector3f>& rCapsule);
    static bool calcIntersectionSegmentToCapsule(const Segment<Vector2f>& rSegment,
                                                 const Capsule<Vector2f>& rCapsule);
    static bool calcIntersectionSegmentToCapsule(const Segment<Vector3f>& rSegment,
                                                 const Capsule<Vector3f>& rCapsule);
    static bool calcIntersectionPlaneToCapsule(const Plane2<f32>& rPlane,
                                               const Capsule<Vector2f>& rCapsule);
    static bool calcIntersectionPlaneToCapsule(const Plane3<f32>& rPlane,
                                               const Capsule<Vector3f>& rCapsule);
};

}  // namespace sead
