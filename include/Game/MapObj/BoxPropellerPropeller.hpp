#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/Execute/ExecuteUtil.hpp"
class BoxPropellerPropeller : public al::LiveActor {
public:
    BoxPropellerPropeller(const al::LiveActor*, const char*, const al::ActorInitInfo&, const char*);
    ~BoxPropellerPropeller() override;
    void control() override;
    void addToFrontDraw();
    void removeFromFrontDraw();
    void pause() override;
    void resume() override;
private:
    const al::LiveActor* mParent;
    const sead::Matrix34f* mJointMtx = nullptr;
    bool mAlignUp = false;
    bool mPaused = false;
    al::ModelDrawerArray mDrawers;
};
