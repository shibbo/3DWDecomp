#pragma once
#include "Library/Scene/Scene.hpp"
class ProductStageStartParam;
/** @brief Course-selection scene, partially reconstructed. */
class CourseSelectScene : public al::Scene {
public:
    CourseSelectScene(bool isAfterEnding);
    bool isControllerChange() const;
    bool isGotoTitle() const;
    bool isLoadGame() const;
    void getSelectedCourseInfo(ProductStageStartParam* pStartParam) const;
private:
    unsigned char mUnreconstructedE8[0x4a0];
};
static_assert(sizeof(CourseSelectScene) == 0x588);
