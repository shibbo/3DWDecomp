#include "MapObj/TouchTrackDrawer.hpp"
#include "MapObj/DrcAssistDirectorUtil.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Math/MatrixUtil.hpp"
#include "Project/Collision/CollisionUtil.hpp"
#include "Project/Obj/FootPrint.hpp"

TouchTrackDrawer::TouchTrackDrawer(al::LiveActor* pHost) : mHost(pHost) {}

void TouchTrackDrawer::init(const al::ActorInitInfo& rInfo, const char* pArchiveName, int count) {
    mActiveTracks.allocBuffer(count, nullptr);
    mAvailableTracks.allocBuffer(count, nullptr);
    for (int i = 0; i < count; ++i)
        mAvailableTracks.pushBack(new al::FootPrint(rInfo, pArchiveName));
}

void TouchTrackDrawer::update() {
    if (!rc::isEnableTouchPointer(mHost)) {
        mHasPreviousPoint = false;
        return;
    }
    const sead::Vector3f& point = rc::getTouchPointerPosition(mHost);
    sead::Vector3f direction = point - mPreviousPoint;
    float distance = direction.length();
    if (!mHasPreviousPoint) {
        if (distance < 25.0f)
            return;
        mPreviousPoint = point;
        direction = point - mPreviousPoint;
        distance = direction.length();
    }
    int hasPreviousPoint = mHasPreviousPoint ? 1 : 0;
    if (distance < 25.0f && hasPreviousPoint)
        return;
    if (al::normalizeOrZero(&direction)) {
        direction = sead::Vector3f::ez;
        mPreviousPoint = point;
    }
    int count = sead::Mathi::clamp(int(distance / 25.0f), 1, 3);
    for (int i = 0; i < count; ++i) {
        al::Triangle triangle;
        sead::Vector3f hitPoint;
        sead::Vector3f offset = sead::Vector3f::ey * 3.0f;
        sead::Vector3f start(point);
        start += offset;
        if (!alCollisionUtil::getFirstPolyOnArrow(mHost, &hitPoint, &triangle, start, sead::Vector3f::ey * -10.0f,
                                                nullptr, nullptr)) {
            mHasPreviousPoint = false;
            return;
        }
        if (al::getHitSensor(mHost, "Collision") != triangle.getSensor())
            return;
        if (mAvailableTracks.size() == 0) {
            al::FootPrint* oldest = mActiveTracks.popFront();
            oldest->kill();
            mAvailableTracks.pushBack(oldest);
        }
        sead::Vector3f position = direction * (distance * (float(i) / float(count))) + mPreviousPoint;
        al::FootPrint* footprint = mAvailableTracks.popFront();
        sead::Matrix34f pose = sead::Matrix34f::ident;
        al::makeMtxUpFront(&pose, *triangle.getNormal(0), direction);
        al::updatePoseMtx(footprint, &pose);
        al::setTrans(footprint, position + *triangle.getNormal(0) * 5.0f);
        footprint->appear();
        mActiveTracks.pushBack(footprint);
    }
    mHasPreviousPoint = true;
    mPreviousPoint = point;
}
