#pragma once

#include "Library/Layout/LayoutActor.hpp"

namespace al { class LayoutInitInfo; }

/** @brief Layout parts used to test a warning animation. */
class TestLayoutPartsActor : public al::LayoutActor {
public:
    TestLayoutPartsActor(const al::LayoutInitInfo& rInfo, const char* pName,
                         const char* pPartsName, al::LayoutActor* pParent);
    void start();
    void exeWait();
    void exeWarning();
};
