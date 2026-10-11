#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class FurKeeper;

class FurActor : public al::LiveActor {
public:
    FurActor(al::LiveActor* pParent, const al::ActorInitInfo& rInfo, const char* pFurName);

    void movement() override;
    void calcAnim() override;
    void draw() const override;

private:
    FurKeeper* mFurKeeper;
};

static_assert(sizeof(FurActor) == 0x150);
