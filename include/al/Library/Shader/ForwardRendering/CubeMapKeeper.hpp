#pragma once

#include <basis/seadTypes.h>
#include <container/seadPtrArray.h>
#include <common/aglDisplayList.h>
#include <container/seadSafeArray.h>
#include <math/seadVector.h>
#include <prim/seadSafeString.h>

#include "Library/Shader/ForwardRendering/EnvTextureKeeper.hpp"

namespace agl {
class DisplayList;
class TextureData;
class TextureSampler;
}  // namespace agl

namespace nn::g3d {
class ResFile;
}  // namespace nn::g3d

namespace al {
class CategoryLightInfo;
class CategoryLightInfoHolder;
class GpuMemAllocator;
class GraphicsParamFilePath;
class GraphicsSystemInfo;
class LiveActor;
class LiveActorKit;
class PlayerHolder;
class Resource;

/**
 * @brief Prebuilt display lists binding the cube map textures of one roughness array.
 */
class RoughnessArrayDL {
public:
    RoughnessArrayDL();

    sead::SafeArray<agl::DisplayList*, 5> mRoughness;
    sead::SafeArray<agl::DisplayList*, 5> mRefract;
    agl::DisplayList* mIrradiance;
};

static_assert(sizeof(RoughnessArrayDL) == 0x58);

/**
 * @brief The cube map textures of one light category: one sampler per roughness type (the last
 * one is the irradiance map).
 */
class RoughnessArray {
public:
    RoughnessArray() : mName("No Name") {}

    ~RoughnessArray();

    bool activateTexture(s32 roughness, bool isRefract, bool isForceBlack) const;
    bool isFileLoaded() const;

    sead::FixedSafeString<256> mName;
    sead::FixedPtrArray<agl::TextureSampler, 6> mSamplers;
    RoughnessArrayDL mDisplayLists;
};

static_assert(sizeof(RoughnessArray) == 0x1b0);

/**
 * @brief A registered cube map, with one roughness array per light category.
 */
class CubeMapInfo {
public:
    /**
     * @brief Creates a cube map with empty roughness arrays.
     * @param categoryNum Number of light categories.
     * @param pTextureData Texture data (unused).
     */
    CubeMapInfo(s32 categoryNum, const agl::TextureData* pTextureData)
        : mTextureData(pTextureData), mName("No Name") {
        mRoughnessArrays.allocBuffer(categoryNum, nullptr);

        for (s32 i = 0; i < categoryNum; i++) {
            mRoughnessArrays.pushBack(new RoughnessArray());
        }
    }

    ~CubeMapInfo();

    void setCategoryName();
    bool isFileLoaded() const;

    sead::PtrArray<RoughnessArray> mRoughnessArrays;
    const agl::TextureData* mTextureData;
    sead::FixedSafeString<256> mName;
};

static_assert(sizeof(CubeMapInfo) == 0x130);

/**
 * @brief Loads the cube maps of the stage and of the default stage and binds the one selected by
 * the current graphics area.
 */
class ShaderCubeMapKeeper {
public:
    /**
     * @brief Cube map resources of a stage: the model file holding the texture list and the raw
     * texture file.
     */
    struct TexResInfo {
        void tryLoad(const char* pName);

        nn::g3d::ResFile* mResFile = nullptr;
        void* mTextureFile = nullptr;
    };

    ShaderCubeMapKeeper(GraphicsSystemInfo* pGraphicsSystemInfo, PlayerHolder* pPlayerHolder);
    ~ShaderCubeMapKeeper();

    void initStageResource(const Resource* pResource, const char* pName, const LiveActorKit* pKit);
    void makeTextureByInfo(const TexResInfo& rInfo, bool isUnused, const char* pName,
                           const char* pUnused);
    void endInit();
    CubeMapInfo* findCubeMapInfoByName(const char* pName) const;
    bool activateCubeMapTexture(s32 index, s32 roughness, s32 category, bool isRefract) const;
    CubeMapInfo* findCubeMapInfo(s32 index) const;
    const char* tryGetCurrentCubeMapLightPresetName() const;
    const char* tryGetCubeMapLightPresetName(const char* pName) const;
    const char* tryGetCubeMapLightPresetName(s32 index) const;
    bool isDrawCubeMap() const;
    void setCubeMap(const char* pName);
    void updateCubeMapKeeper();
    CubeMapInfo* tryFindCubeMapInfoByName(const char* pName) const;
    s32 findCubeMapIndexByName(const char* pName) const;
    const CategoryLightInfo* getCurrentCategoryLightInfo(s32 category) const;
    CategoryLightInfoHolder* getCategoryLightInfoHolder(s32 category) const;
    const agl::TextureSampler* getIrradiance(s32 category, const sead::Vector3f& rPos) const;
    const agl::TextureSampler* getRoughnessCubeMap(s32 roughness, s32 category) const;

    const CubeMapInfo* getForceCubeMapInfo() const { return mForceCubeMapInfo; }

    void setForceCubeMapInfo(CubeMapInfo* pInfo) { mForceCubeMapInfo = pInfo; }

    f32 getModelLightIntensity() const { return mModelLightIntensity; }
    void setModelLightIntensity(f32 intensity) { mModelLightIntensity = intensity; }

private:
    const char* mCubeMapName = nullptr;
    f32 mModelLightIntensity = 1.0f;
    GraphicsSystemInfo* mGraphicsSystemInfo;
    sead::FixedPtrArray<CubeMapInfo, 58> mCubeMapInfos;
    CubeMapInfo* mForceCubeMapInfo = nullptr;
    CubeMapInfo* mDefaultCubeMapInfo = nullptr;
    CubeMapInfo* mCurrentCubeMapInfo = nullptr;
    TexResInfo mStageTexResInfo;
    TexResInfo mDefaultTexResInfo;
    GraphicsParamFilePath* mParamFilePath;
    CategoryLightInfoHolder* mStandardLightInfoHolder = nullptr;
    CategoryLightInfoHolder* mCharacterLightInfoHolder = nullptr;
    GpuMemAllocator* mGpuMemAllocator;
    bool mIsInitialized = false;
};

static_assert(sizeof(ShaderCubeMapKeeper) == 0x258);

}  // namespace al

namespace CubeMapFunction {
al::ShaderCubeMapKeeper* getShaderCubeMapKeeper(const al::LiveActor* pActor);
}  // namespace CubeMapFunction
