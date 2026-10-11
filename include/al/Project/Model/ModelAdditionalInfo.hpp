#pragma once

#include <basis/seadTypes.h>

#include "Library/Draw/GraphicsSystemInfo.hpp"
#include "Library/Shader/DeferredRendering/CubeMapDirector.hpp"
#include "Library/Shader/ForwardRendering/CubeMapKeeper.hpp"
#include "Library/Shader/ForwardRendering/EnvTextureKeeper.hpp"

namespace nn::g3d {
class ResMaterial;
}

namespace al {
class SimpleModelG3D;

class ModelAdditionalInfo {
public:
    ModelAdditionalInfo(const GraphicsSystemInfo* pInfo, bool isSecondCategory);

    void activateEnvTextureIndividual(s32 shapeIndex, const SimpleModelG3D* pModel);
    void activateEnvTexture(s32 shapeIndex, const SimpleModelG3D* pModel);
    void activateModelLightTexture(s32 shapeIndex, const SimpleModelG3D* pModel);
    void activateModelLightTexture(const nn::g3d::ResMaterial* pMaterial);
    void activateModelLightTexture(s32 index);

    const GraphicsSystemInfo* getGraphicsSystemInfo() const { return mGraphicsSystemInfo; }
    void setGraphicsSystemInfo(const GraphicsSystemInfo* info) { mGraphicsSystemInfo = info; }
    const void* getLightInfo() const { return mLightInfo; }
    EnvTexId* getEnvTexId() { return &mEnvTexId; }
    s32 getCategory() const { return mCategory; }

protected:
    explicit ModelAdditionalInfo(const GraphicsSystemInfo* pInfo) : mGraphicsSystemInfo(pInfo) {
        mEnvTexId.initForCache();
    }

    void updateLightInfo() {
        ShaderCubeMapKeeper* keeper =
            mGraphicsSystemInfo->getCubeMapDirector()->getShaderCubeMapKeeper();
        if (keeper) {
            mLightInfo = keeper->getCurrentCategoryLightInfo(mCategory);
        }
    }

    EnvTexId mEnvTexId;
    s32 mCategory = 0;
    const GraphicsSystemInfo* mGraphicsSystemInfo;
    bool mIsRenderCubeMap = false;
    const void* mLightInfo = nullptr;
};

static_assert(sizeof(ModelAdditionalInfo) == 0x40);

}  // namespace al
