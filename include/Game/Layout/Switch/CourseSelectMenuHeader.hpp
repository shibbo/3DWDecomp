#pragma once

#include <basis/seadTypes.h>

#include "Library/Layout/LayoutActor.hpp"

namespace al {
class LayoutInitInfo;
}  // namespace al

/**
 * @brief Header of the course select map menus (title and L/R button guides).
 * @note Minimal declaration: only what the reconstructed code uses is declared so far.
 */
class CourseSelectMenuHeader : public al::LayoutActor {
public:
    explicit CourseSelectMenuHeader(const al::LayoutInitInfo& rInfo);

    void setButtonType(bool isJoySingle);
    void setHeader(const char* pName, bool isSkipAnim);
    void end();

private:
    u8 _121[0x138 - 0x121];
};

static_assert(sizeof(CourseSelectMenuHeader) == 0x138);
