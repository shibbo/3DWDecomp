#pragma once

#include "Library/Execute/IUseExecutor.hpp"
#include "Library/Scene/ISceneObj.hpp"

namespace al {
class ExecuteDirector;
class LiveActor;
}

class CoinRotater : public al::IUseExecutor, public al::ISceneObj {
public:
    explicit CoinRotater(al::ExecuteDirector* pDirector);
    void execute() override;
    const char* getSceneObjName() const override;
    float getRotateY() const;
    float getRotateYInWater() const;

private:
    float mRotateY = 0.0f;
    float mRotateYInWater = 0.0f;
};

namespace rc {
float getCoinRotateY(const al::LiveActor* pActor);
float getCoinRotateYByFrame(const al::LiveActor* pActor);
}
