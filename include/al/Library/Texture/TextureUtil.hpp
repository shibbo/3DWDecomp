#pragma once

#include <basis/seadTypes.h>
#include <common/aglGPUMemAddr.h>
#include <common/aglTextureEnum.h>
#include <gfx/seadColor.h>
#include <math/seadMatrix.h>
#include <math/seadVector.h>
#include <prim/seadSafeString.h>

namespace agl {
class DrawContext;
class ShaderProgram;
class TextureData;
class TextureSampler;
}  // namespace agl

namespace nn::gfx {
struct ResTextureData;
}  // namespace nn::gfx

namespace al {
class LiveActor;

/**
 * @brief Texture reference as bound to a model material (texture view and descriptor slot).
 */
struct TextureRefData {
    TextureRefData() = default;
    TextureRefData(const void* pTextureView, u64 descriptorSlot)
        : mTextureView(pTextureView), mDescriptorSlot(descriptorSlot) {}

    void set(const void* pTextureView, u64 descriptorSlot) {
        mTextureView = pTextureView;
        mDescriptorSlot = descriptorSlot;
    }

    const void* mTextureView = nullptr;
    u64 mDescriptorSlot = 0xffffffffffffffff;
};

/**
 * @brief Wraps an agl texture as a gfx resource texture so it can replace a model texture.
 */
class TextureReplacer {
public:
    TextureReplacer(const agl::TextureData* pTextureData);
    TextureReplacer();

    void setup(const agl::TextureData* pTextureData);
    void replace(LiveActor* pActor, const char* pMaterialName, const char* pTextureName);
    void update();
    const TextureRefData* getTextureRef() const;
    nn::gfx::ResTextureData* getResTexture() const { return mResTexture; }

private:

    nn::gfx::ResTextureData* mResTexture = nullptr;
    nn::gfx::ResTextureData* mResTextureData = nullptr;
    const agl::TextureData* mTextureData = nullptr;
    TextureRefData* mTextureRef = nullptr;
};

/**
 * @brief Component selection presets for TextureUnit samplers.
 */
struct CompSelType {
    CompSelType(s32 value) : mValue(value) {}

    operator s32() const { return mValue; }

    volatile s32 mValue;
};

/**
 * @brief Creation parameters of a TextureUnit.
 */
struct TextureInitArg {
    s32 mWidth = 0;
    s32 mHeight = 0;
    s32 mDepth = 0;
    s32 mMipLevelNum = 1;
    bool mIsCubemap = false;
    agl::TextureFormat mFormat = agl::TextureFormat(0x1d);
    s32 mWrapX = 7;
    s32 mWrapY = 7;
    s32 mWrapZ = 7;
    s32 mMagFilter = 1;
    s32 mMinFilter = 1;
    s32 mMipFilter = 0;
    s32 mCompSel = 0;
};

static_assert(sizeof(TextureInitArg) == 0x34);

/**
 * @brief A texture with its own image memory and sampler.
 */
class TextureUnit {
public:
    TextureUnit(const char* pName);

    void finalize();
    u32 getWidth() const;
    u32 getHeight() const;
    u32 getDepth() const;
    bool is1D() const;
    bool is2D() const;
    bool is3D() const;
    bool isCubemap() const;
    void invalidateGpuCacheRead(agl::DrawContext* pDrawContext);
    void invalidateGpuCacheWrite(agl::DrawContext* pDrawContext);
    bool tryCreateTexture(const TextureInitArg& rArg);
    void applyCompSel(const CompSelType& rType);

    agl::TextureData* getTextureData() const { return mTexture; }
    agl::TextureSampler* getSampler() const { return mSampler; }

private:
    agl::TextureData* mTexture = nullptr;
    agl::TextureSampler* mSampler = nullptr;
    agl::GPUMemVoidAddr mImage;
    sead::FixedSafeString<256> mName;
    TextureInitArg mInitArg;
};

static_assert(sizeof(TextureUnit) == 0x178);

agl::TextureData* createTexture(s32 width, s32 height, u8** ppImage);
void calcOrthoProjectedTexCoord(sead::Vector2f* pTexCoord, const sead::Matrix34f& rViewMtx,
                                const sead::Vector3f& rOrigin, const sead::Vector3f& rPos,
                                f32 width, f32 height);
bool isInsideTexture(const sead::Vector2i& rPos, const agl::TextureData* pTextureData);
agl::TextureData* createAglTextureData(agl::TextureFormat format, s32 width, s32 height,
                                       s32 mipLevelNum, agl::TextureAttribute attribute);
void initAglTextureData(agl::TextureData* pTextureData, agl::TextureFormat format, s32 width,
                        s32 height, s32 mipLevelNum, agl::TextureAttribute attribute);
agl::TextureData* createAglTextureDataLinear(agl::TextureFormat format, s32 width, s32 height,
                                             s32 mipLevelNum);
void initAglTextureDataLinear(agl::TextureData* pTextureData, agl::TextureFormat format,
                              s32 width, s32 height, s32 mipLevelNum);
void destroyAglTextureAndImage(agl::TextureData** ppTextureData);
void drawColorCircle(agl::DrawContext* pDrawContext, const sead::Color4f& rColor,
                     const sead::Matrix34f& rViewMtx, const sead::Matrix44f& rProjMtx);
void makeTextureDataFromArchive(agl::TextureData* pTextureData, const char* pArchiveName,
                                const char* pFileName, const char* pTextureName);
bool tryMakeTextureDataFromArchive(agl::TextureData* pTextureData, const char* pArchiveName,
                                   const char* pFileName, const char* pTextureName);
sead::Color4u8 getColor(const agl::TextureData* pTextureData, s32 x, s32 y);
sead::Color4u8 getColorFromLinearTexture(const agl::TextureData* pTextureData, s32 x, s32 y);
f32 getF32FromLinearTextureF16(const agl::TextureData* pTextureData, s32 x, s32 y, s32 channel,
                               s32 channelNum);
f32 getF32FromLinearTextureF32(const agl::TextureData* pTextureData, s32 x, s32 y, s32 channel,
                               s32 channelNum);
void calcNormalFromLinearTexture(sead::Vector3f* pNormal, const agl::TextureData* pTextureData,
                                 s32 x, s32 y);
void activateSampler(agl::DrawContext* pDrawContext, const agl::TextureSampler* pSampler,
                     const agl::ShaderProgram* pProgram, const char* pName);

}  // namespace al
