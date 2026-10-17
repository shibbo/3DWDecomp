#pragma once
#include "Library/Layout/LayoutActor.hpp"
namespace al { class LayoutInitInfo; }
class TestLayoutPartsActor;
/** @brief Test layout for directional parts and text animations. */
class TestAoyagiLayout : public al::LayoutActor {
public:
    explicit TestAoyagiLayout(const al::LayoutInitInfo& rInfo);
    void control() override;
    void exeWait();
    void exeTextAppear();
    void exeTextWait();
    void exeTextEnd();
private:
    TestLayoutPartsActor* mLeft = nullptr;
    TestLayoutPartsActor* mRight = nullptr;
    TestLayoutPartsActor* mUp = nullptr;
    TestLayoutPartsActor* mDown = nullptr;
    al::LayoutActor* mParts;
};
static_assert(sizeof(TestAoyagiLayout) == 0x150);
