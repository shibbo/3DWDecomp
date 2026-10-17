#pragma once

#include "Library/Layout/LayoutActor.hpp"

namespace al { class ActorInitInfo; }

/** @brief Animated layout used as part of a demo scene. */
class DemoLayoutObj : public al::LayoutActor {
public:
    DemoLayoutObj();
    void initDemoSceneLayout(const al::ActorInitInfo& rInfo, const char* pName);
    void exeWait();
    void exeEnd();
    void startDemo();
    void endDemo();
    bool isEnd(int remainingFrames) const;
    int getMaxFrame() const;
    int getCurrentFrame() const;

private:
    const char* mActionName;
    float mStepRate = 1.0f;
    int mMaxFrame;
};
