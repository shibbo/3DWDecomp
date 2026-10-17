#pragma once

#include <math/seadVector.h>

#include "Library/Layout/LayoutActor.hpp"

namespace al {
class DemoDirector;
class LayoutInitInfo;
}  // namespace al

/**
 * @brief Counter shown over a locked Giga Bell with the Cat Shines still needed to unlock it.
 */
class CounterLockGigaBell : public al::LayoutActor {
public:
    CounterLockGigaBell(const char* pName, const al::LayoutInitInfo& rInfo,
                        const sead::Vector3f* pTrans, al::DemoDirector* pDemoDirector);

    void startShow(s32 count);
    void endShow();
    bool isDemoOn();
    void exeAppear();
    void exeWait();
    void exeEnd();
    al::SceneCameraInfo* getSceneCameraInfo() const override;

private:
    al::SceneCameraInfo* mSceneCameraInfo;
    al::DemoDirector* mDemoDirector;
    const sead::Vector3f* mTrans;
};

static_assert(sizeof(CounterLockGigaBell) == 0x140);
