#pragma once
#include "Library/LiveActor/LiveActor.hpp"

class KoopaChaseKoopa : public al::LiveActor {
public:
    bool isStateDamage() const;
    void setStateWarpProvocation();
    bool isStateWarpProvocation() const;

private:
    al::LiveActor* mHost;
};
