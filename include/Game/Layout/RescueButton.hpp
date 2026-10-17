#pragma once
#include "Library/Layout/LayoutActor.hpp"
#include <prim/seadSafeString.h>
namespace al { class LayoutInitInfo; class SceneCameraInfo; }
/** @brief Rescue prompt with a gradually draining hold meter. */
class RescueButton : public al::LayoutActor {
public:
    explicit RescueButton(const al::LayoutInitInfo& rInfo);
    void startAppear();
    void end();
    void startFinish();
    void setHoldTime(float time);
    void updateHoldMeter();
    void kill() override;
    void exeAppear();
    void exeWait();
    void exeEnd();
    bool isEnd() const;
    al::SceneCameraInfo* getSceneCameraInfo() const override;
private:
    al::SceneCameraInfo* mSceneCameraInfo = nullptr;
    sead::WFixedSafeString<32> mMeterText;
    float mDisplayedHold = 0.0f;
    float mHoldTime = 0.0f;
};
static_assert(sizeof(RescueButton) == 0x190);
