#pragma once

#include <basis/seadTypes.h>

#include "Library/Layout/LayoutActor.hpp"

namespace al {
class LayoutInitInfo;
}  // namespace al

/**
 * @brief Header of the course select map menus (title and L/R button guides).
 */
class CourseSelectMenuHeader : public al::LayoutActor {
public:
    explicit CourseSelectMenuHeader(const al::LayoutInitInfo& rInfo);

    void appear() override;
    void exeAppear();
    void exeWait();
    void exeTextChange();
    void exeEnd();

    void setButtonType(bool isJoySingle);
    void setHeader(const char* pName, bool isAnimate);
    void end();

private:
    al::LayoutActor* mHeader = nullptr;
    const char* mMessageName = nullptr;
};

static_assert(sizeof(CourseSelectMenuHeader) == 0x138);
