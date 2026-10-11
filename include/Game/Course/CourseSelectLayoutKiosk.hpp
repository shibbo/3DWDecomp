#pragma once

#include <basis/seadTypes.h>
#include "Library/Layout/LayoutActor.hpp"

namespace al {
class LayoutInitInfo;
}  // namespace al
class RCSControlGuideBar;

/**
 * @brief Course select layout of the kiosk (demo) version of the title screen.
 * @note Only the members used by already-decompiled callers are declared.
 */
class CourseSelectLayoutKiosk : public al::LayoutActor {
public:
    explicit CourseSelectLayoutKiosk(const al::LayoutInitInfo& rInfo);

    bool isDecideBack() const;
    bool isCourseDecided() const;

    /**
     * @brief Sets the control guide bar shown below the course select.
     * @param pGuideBar The control guide bar.
     */
    void setGuideBar(RCSControlGuideBar* pGuideBar) { mGuideBar = pGuideBar; }

    /**
     * @brief Gets the world of the decided course.
     * @return The world id.
     */
    s32 getDecidedWorldId() const { return mDecidedWorldId; }

    /**
     * @brief Gets the stage of the decided course.
     * @return The stage id.
     */
    s32 getDecidedStageId() const { return mDecidedStageId; }

private:
    u8 _121[0x128 - 0x121];
    RCSControlGuideBar* mGuideBar;  // 0x128
    u8 _130[0x138 - 0x130];
    s32 mDecidedWorldId;  // 0x138
    s32 mDecidedStageId;  // 0x13c
    u8 _140[0x148 - 0x140];
};

static_assert(sizeof(CourseSelectLayoutKiosk) == 0x148);
