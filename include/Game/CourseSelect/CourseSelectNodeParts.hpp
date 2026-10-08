#pragma once

#include <basis/seadTypes.h>

#include "Library/LiveActor/LiveActor.hpp"

namespace al {
class ActorInitInfo;
}  // namespace al

/**
 * @brief Road part placed at a course-select node: start or end of a road, or a curve joining
 * two roads.
 * @note Minimal declaration: only what the reconstructed code uses is declared so far.
 */
class CourseSelectNodeParts : public al::LiveActor {
public:
    CourseSelectNodeParts(const char* pName, const char* pGrowAction, const char* pWaitAction);

    void initPartsWithArchiveName(const al::ActorInitInfo& rInfo, const char* pArchiveName);
    void startOpen();
    void startOpenImmediately();
    bool isEndGrow() const;

    /** @brief Gets whether the part is a curve. @return true for a curve. */
    bool isCurve() const { return mIsCurve; }

private:
    u8 _144[0x158 - 0x144];
    bool mIsCurve;  // 0x158
};

static_assert(sizeof(CourseSelectNodeParts) == 0x160);
