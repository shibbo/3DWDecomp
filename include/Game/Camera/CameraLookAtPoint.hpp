#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class CameraLookAtPoint : public al::LiveActor {
public:
    explicit CameraLookAtPoint(const char* name);
    void interpolate(u32, u32, float, sead::Vector3f*, sead::Vector3f*) const;
    void interpolateIn(u32, const sead::Vector3f&, const sead::Vector3f&, float, sead::Vector3f*, sead::Vector3f*) const;
    void interpolateInOut(u32, const sead::Vector3f&, const sead::Vector3f&, float, sead::Vector3f*, sead::Vector3f*) const;
    u32 getPointCount() const { return mPointCount; }
    int getWaitTime(u32 point) const { return mPoints[point].waitTime; }
    int getMoveTime(u32 point) const { return mPoints[point].moveTime; }
private:
    struct Point {
        sead::Vector3f at;
        sead::Vector3f pos;
        int waitTime;
        int moveTime;
        int _20;
    };
    Point* mPoints;
    u32 mPointCount;
    u8 mUnreconstructed[0x4c];
};
static_assert(sizeof(CameraLookAtPoint) == 0x1a0);
