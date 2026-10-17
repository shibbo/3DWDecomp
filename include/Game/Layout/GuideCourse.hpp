#pragma once

#include "Library/Layout/LayoutActor.hpp"

namespace al { class LayoutInitInfo; class LiveActor; }

/** @brief Course guide positioned above a world-map miniature. */
class GuideCourse : public al::LayoutActor {
public:
    GuideCourse(const al::LayoutInitInfo& rInfo, const char* pName);
    void control() override;
    void updateScreenPos();
    void startAppear(al::LiveActor* pMiniature);
    void startEnd();
    bool isShow() const;
    void exeAppear();
    void exeWait();
    void exeEnd();
    void exeHide();
    /** @brief Returns the camera used to project the miniature.
     * @return Scene camera information. */
    al::SceneCameraInfo* getSceneCameraInfo() const override { return mSceneCameraInfo; }

private:
    al::SceneCameraInfo* mSceneCameraInfo;
    al::LiveActor* mMiniature = nullptr;
};

static_assert(sizeof(GuideCourse) == 0x138);
