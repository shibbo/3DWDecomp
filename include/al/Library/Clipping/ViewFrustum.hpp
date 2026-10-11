#pragma once

#include <math/seadGeometry.h>
#include <math/seadMatrix.h>
#include <math/seadVector.h>

namespace al {
class ViewFrustum {
public:
    struct Points {
        sead::Vector3f mNearPoints[4];
        sead::Vector3f mFarPoints[4];
    };

    struct PlaneIntersection {
        u32 mPointNum;
        sead::Vector3f mPoints[4];
        bool mIsIntersectEdge[4];
    };

    ViewFrustum()
        : mPlanes{{sead::Vector3f::ex, 0.0f}, {sead::Vector3f::ex, 0.0f},
                  {sead::Vector3f::ex, 0.0f}, {sead::Vector3f::ex, 0.0f},
                  {sead::Vector3f::ex, 0.0f}, {sead::Vector3f::ex, 0.0f}} {}

    void calcFrustumFromViewMtx(const sead::Matrix34f& rViewMtx, f32 fovy, f32 aspect, f32 near,
                                f32 far);
    void makePlanesFromMtx(const sead::Matrix34f& rPoseMtx, f32 fovy, f32 aspect, f32 near,
                           f32 far);
    void calcFrustumFromPoseMtx(const sead::Matrix34f& rPoseMtx, f32 fovy, f32 aspect, f32 near,
                                f32 far);
    bool isIntersectSphere(const sead::Vector3f& rPos, f32 radius) const;
    void calcFrustumPoints(Points* pPoints) const;
    bool calcPlaneIntersection(PlaneIntersection* pIntersection,
                               const sead::Plane3f* pPlane) const;

private:
    enum PlaneIndex {
        cPlane_SideMinusX,
        cPlane_SidePlusX,
        cPlane_SideMinusY,
        cPlane_SidePlusY,
        cPlane_Near,
        cPlane_Far,
        cPlane_Num,
    };

    sead::Plane3f mPlanes[cPlane_Num];
};
}  // namespace al
