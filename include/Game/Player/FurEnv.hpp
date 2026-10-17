#pragma once

#include <basis/seadTypes.h>

#include "Library/Scene/ISceneObj.hpp"

namespace al {
class GraphicsSystemInfo;
class IUseSceneObjHolder;
class UniformBlock;
}  // namespace al

namespace nn::g3d {
class ResShaderProgram;
class ResShadingModel;
}  // namespace nn::g3d

/**
 * Scene object holding the shared shader environment of the shell fur (fur uniform blocks and
 * the "RenderShellFur" shading model).
 */
class FurEnv : public al::ISceneObj {
public:
    FurEnv(al::GraphicsSystemInfo* pGraphicsSystemInfo);
    virtual ~FurEnv();

    const char* getSceneObjName() const override { return "FurEnv"; }

    void update();
    void activate();
    const nn::g3d::ResShaderProgram* getShaderProgram(u32 skinWeightNum) const;

private:
    al::GraphicsSystemInfo* mGraphicsSystemInfo;
    al::UniformBlock* mFurUbo = nullptr;
    al::UniformBlock* mShadowUbo = nullptr;
    nn::g3d::ResShadingModel* mShadingModel = nullptr;
};

static_assert(sizeof(FurEnv) == 0x28);

namespace rc {
FurEnv* getFurEnv(const al::IUseSceneObjHolder* pHolder);
}  // namespace rc
