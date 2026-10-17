#pragma once
#include "Library/Layout/LayoutActor.hpp"
namespace al { class LayoutInitInfo; }
/** @brief Original layout test for text replacement and parts animations. */
class TestMusaLayout : public al::LayoutActor {
public:
    explicit TestMusaLayout(const al::LayoutInitInfo& rInfo);
    void exeWait();
    void exeScale();
    void setString(const char16_t* pString);
};
