#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class TentackHead;
class TentackStateFallRock;

class TentackRockFaller : public al::LiveActor {
public:
    TentackRockFaller(const char* pName, TentackHead* pHead);
    ~TentackRockFaller() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void appear() override;
    void exeFallRock();
    void setValidFollowForce();

private:
    TentackStateFallRock* mFallRockState;
};
