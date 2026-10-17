#pragma once

#include "Project/AreaObj/AreaObj.hpp"

namespace al {
class IUseCollision;
}
class ResultCheckArrow;

class PlayerCollisionYPlaneArea : public al::AreaObj {
public:
    PlayerCollisionYPlaneArea(const char* pName);
    void init(const al::AreaInitInfo& rInfo) override;
    ResultCheckArrow* getResultCheckArrow(const al::IUseCollision* pCollision);

    bool mIsCollisionChecked = false;
    s32 _88 = 0;
    ResultCheckArrow* mResultCheckArrow = nullptr;
};
