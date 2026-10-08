#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"

namespace al {
class ActorInitInfo;
}  // namespace al

class CourseSelectNode;

/**
 * @brief Road connecting the courses of the course select map.
 * @note Minimal declaration: only what the reconstructed code uses is declared so far.
 */
class CourseSelectRoad : public al::LiveActor {
public:
    CourseSelectRoad(const char* pName, const CourseSelectNode* pNode,
                     const sead::Vector3f& rTargetTrans);

    static void setRoadOpenSuperSpeed(bool isSuperSpeed);

    void initRoad(const al::ActorInitInfo& rInfo, const sead::Vector3f* pOffset);
    void subDistance(f32 distance);
    void startOpen(bool isImmediately);
    bool isEndOpen() const;
    bool isWait() const;

    /** @brief Gets the position the road grows to. @return The target position. */
    const sead::Vector3f& getTargetTrans() const { return mTargetTrans; }

private:
    u8 _144[0x150 - 0x144];
    sead::Vector3f mTargetTrans;  // 0x150
    u8 _15c[0x178 - 0x15c];
};

static_assert(sizeof(CourseSelectRoad) == 0x178);
