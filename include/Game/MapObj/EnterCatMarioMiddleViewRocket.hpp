#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class EnterCatMarioMiddleViewRocket : public al::LiveActor {
public:
    explicit EnterCatMarioMiddleViewRocket(const char* pName);
    ~EnterCatMarioMiddleViewRocket() override;
    void init(const al::ActorInitInfo& rInfo) override;

private:
    al::LiveActor* mRock;
    al::LiveActor* mRocket;
};
