#pragma once

#include "Project/AreaObj/AreaObj.hpp"

namespace al {
class CollisionParts;
}

class FireBallSafeArea : public al::AreaObj {
public:
    FireBallSafeArea(const char* pName);
    void init(const al::AreaInitInfo& rInfo) override;
    void initAfterPlacement();
    void tryFollowCollisionConnection();
    bool isIgnoreGiantFireballs();

    bool mIgnoreGiantFireballs = true;
    bool mIsConnectToCollision = false;
    f32 mCollisionCheckDist = 1000.0f;
    al::CollisionParts* mCollisionParts = nullptr;
    sead::Vector3f mCollisionOffset = sead::Vector3f::zero;
};
