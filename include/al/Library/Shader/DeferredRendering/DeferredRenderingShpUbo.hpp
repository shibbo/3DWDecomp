#pragma once

#include <basis/seadTypes.h>
#include <common/aglShaderLocation.h>
#include <math/seadMatrix.h>

namespace nn::g3d {
class ResShadingModel;
class ResShaderProgram;
}  // namespace nn::g3d

namespace al {
class DeferredRenderingShader;
class UniformBlock;

/**
 * @brief Ring of "shape" uniform blocks (model matrix and skinning weight count) for the
 * deferred rendering shader.
 */
class DeferredRenderingShpUbo {
public:
    DeferredRenderingShpUbo(const DeferredRenderingShader* pShader, s32 uboNum);
    DeferredRenderingShpUbo(const nn::g3d::ResShadingModel* pShadingModel,
                            const nn::g3d::ResShaderProgram* pShaderProgram, s32 uboNum);

    void setMtx(const sead::Matrix34f* pMtx);
    void setWeightNum(s32 weightNum);
    virtual void swap();
    void activate();

private:
    u32 mUboNum;
    s32 mCurrentIndex = 0;
    agl::UniformBlockLocation mLocation;
    UniformBlock** mUbos;
    sead::Matrix34f mMtx;
    s32 mMtxUpdateCount;
    s32 mWeightNum = 0;
    s32 mWeightNumUpdateCount;
};

static_assert(sizeof(DeferredRenderingShpUbo) == 0x70);

}  // namespace al
