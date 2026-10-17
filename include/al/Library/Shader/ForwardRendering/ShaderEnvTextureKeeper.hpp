#pragma once

#include <basis/seadTypes.h>
#include <common/aglShaderEnum.h>
#include <container/seadPtrArray.h>
#include <math/seadMatrix.h>
#include <math/seadVector.h>
#include <prim/seadSafeString.h>
#include <utility/aglParameter.h>
#include <utility/aglParameterObj.h>

namespace agl {
class TextureData;

namespace utl {
class IParameterIO;
}  // namespace utl
}  // namespace agl

namespace nn::g3d {
class ResShadingModel;
}  // namespace nn::g3d

namespace al {
class EnvTexInfo;
class FullScreenQuadModel;
class GraphicsAreaDirector;
class GraphicsSystemInfo;
class ModelAdditionalInfo;
class PlayerHolder;
class ShaderFresnelTextureKeeper;
class ShaderHolder;
class SimpleModelEnv;
class UniformBlock;

/**
 * @brief Alpha mask projection settings of one graphics area parameter set.
 */
class AlphaMaskProjectionParam : public agl::utl::IParameterObj {
public:
    AlphaMaskProjectionParam(bool isDefault);

    const char* getName() const { return mName->ref().cstr(); }

    const sead::Vector3f& getDir() const { return mDir->ref(); }

    void setDir(const sead::Vector3f& rDir) { mDir->ref() = rDir; }

    bool isCameraDir() const { return mIsCameraDir->ref(); }

private:
    agl::utl::Parameter<sead::FixedSafeString<64>>* mName = nullptr;
    agl::utl::Parameter<sead::Vector3f>* mDir = nullptr;
    agl::utl::Parameter<bool>* mIsCameraDir = nullptr;
};

static_assert(sizeof(AlphaMaskProjectionParam) == 0x48);

/**
 * @brief Selects the alpha mask projection parameters of the current graphics area.
 */
class AlphaMaskProjectionInfo {
public:
    AlphaMaskProjectionInfo(const GraphicsSystemInfo* pInfo);

    void init(const GraphicsAreaDirector* pAreaDirector, const char* pStageName);
    void update();
    bool isCameraDir() const;
    const sead::Vector3f& getCurrentDir() const;

private:
    const GraphicsAreaDirector* mAreaDirector = nullptr;
    sead::PtrArray<AlphaMaskProjectionParam> mParams;
    AlphaMaskProjectionParam* mCurrentParam = nullptr;
    AlphaMaskProjectionParam* mDefaultParam = nullptr;
    agl::utl::IParameterIO* mParamIO = nullptr;
    bool mIsLoaded = false;
    const GraphicsSystemInfo* mGraphicsSystemInfo;
};

static_assert(sizeof(AlphaMaskProjectionInfo) == 0x40);

/**
 * @brief Binds the environment textures (cube maps, fresnel and thickness curves, mirror and
 * indirect textures) of materials, and draws the cube map sky.
 */
class ShaderEnvTextureKeeper {
public:
    ShaderEnvTextureKeeper(GraphicsSystemInfo* pInfo, PlayerHolder* pPlayerHolder);
    ~ShaderEnvTextureKeeper();

    void initTexture(ShaderHolder* pShaderHolder);
    void initGraphicsAreaParam(GraphicsAreaDirector* pAreaDirector, const char* pStageName);
    void endInit();
    bool activateEnvTexture(const EnvTexInfo& rInfo, ModelAdditionalInfo* pAdditionalInfo,
                            bool isForce) const;
    void updateEnvTexture();
    void execute();
    agl::ShaderMode renderCubeMapSky(const sead::Matrix34f& rViewMtx,
                                     const sead::Matrix44f& rProjMtx, s32 viewIndex, bool isMRT,
                                     const SimpleModelEnv* pModelEnv, agl::ShaderMode shaderMode,
                                     f32 intensity) const;

    ShaderFresnelTextureKeeper* getFresnelTextureKeeper() const { return mFresnelTextureKeeper; }

    bool isUseViewMtx() const { return mIsUseViewMtx; }

    const sead::Vector3f& getFrontDir() const { return mFrontDir; }

    void setIndirectTexture(const agl::TextureData* pTexture) { mIndirectTexture = pTexture; }

    const agl::TextureData* getIndirectTexture() const { return mIndirectTexture; }

    const agl::TextureData* getCopiedColorTexture() const {
        return static_cast<const agl::TextureData*>(_38);
    }

    void setCopiedDepthTarget(void* pTarget) { _40 = pTarget; }

private:
    GraphicsSystemInfo* mGraphicsSystemInfo;
    bool mIsEnable = false;
    ShaderFresnelTextureKeeper* mFresnelTextureKeeper = nullptr;
    AlphaMaskProjectionInfo* mAlphaMaskProjectionInfo = nullptr;
    bool mIsUseViewMtx = false;
    sead::Vector3f mFrontDir = {0.0f, 0.0f, 1.0f};
    const agl::TextureData* mIndirectTexture = nullptr;
    void* _38 = nullptr;
    void* _40 = nullptr;
    void* _48;
    FullScreenQuadModel* mFullScreenQuadModel = nullptr;
    const nn::g3d::ResShadingModel* mShadingModel = nullptr;
    UniformBlock* mUniformBlock = nullptr;
};

static_assert(sizeof(ShaderEnvTextureKeeper) == 0x68);

}  // namespace al
