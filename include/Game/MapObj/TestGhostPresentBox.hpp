#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class GhostPresentBox;
class TestGhostPresentBox : public al::LiveActor {
public:
    TestGhostPresentBox(const char*);
    ~TestGhostPresentBox() override;
    void init(const al::ActorInitInfo&) override;
private:
    GhostPresentBox* mBox = nullptr;
    int mItemPatternIndex = 0;
};
