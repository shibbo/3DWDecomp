#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class TouchTrackDrawer;
class TestTanakaMapObj : public al::LiveActor {
public:
    TestTanakaMapObj(const char*);
    ~TestTanakaMapObj() override;
    void init(const al::ActorInitInfo&) override;
    void exeWait();
private:
    TouchTrackDrawer* mTouchTrackDrawer = nullptr;
};
