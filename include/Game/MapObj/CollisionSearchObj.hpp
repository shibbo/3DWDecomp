#pragma once

#include <container/seadPtrArray.h>
#include "Library/LiveActor/LiveActor.hpp"

namespace al { class CollisionParts; }

class CollisionSearchObj : public al::LiveActor {
public:
    CollisionSearchObj(const char* pName);
    ~CollisionSearchObj() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void initAfterPlacement() override;
    void gatherCollisionParts(al::CollisionParts* pParts);
    void appear() override;
    void kill() override;
    void control() override;

private:
    sead::PtrArray<al::CollisionParts> mParts;
    float mSearchRadius = 1000.0f;
};
