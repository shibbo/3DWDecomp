#include "Library/PostProcessing/VignettingDrawer.hpp"

#include <cmath>
#include <gfx/seadGraphicsContext.h>
#include <gfx/seadViewport.h>
#include <math/seadMathCalcCommon.h>
#include <nerd/nerdMath.h>
#include <nvn/nvn_FuncPtrInline.h>

#include "common/aglDrawContext.h"
#include "common/aglGPUMemAddr.h"
#include "common/aglIndexStream.h"
#include "common/aglRenderBuffer.h"
#include "common/aglRenderTarget.h"
#include "common/aglShaderLocation.h"
#include "common/aglShaderProgram.h"
#include "common/aglTextureData.h"
#include "common/aglTextureSampler.h"
#include "common/aglVertexAttribute.h"
#include "common/aglVertexBuffer.h"
#include "detail/aglMemoryPoolHeap.h"
#include "utility/aglDynamicTextureAllocator.h"

#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/Memory/Util.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Screen/ScreenFunction.hpp"
#include "Library/Shader/DeferredRendering/FullScreenTriangle.hpp"
#include "Library/Shader/ForwardRendering/ShaderHolder.hpp"
#include "Library/Texture/TextureUtil.hpp"
#include "Project/PostProcessing/PostProcessingDrawerUtil.hpp"

namespace {

const s32 cMeshNum = 2;
const s32 cRingNum = 4;
const s32 cDivNum[cMeshNum] = {64, 4};
const f32 cRingAlpha[cRingNum] = {1.0f, 1.0f, 0.0f, 0.0f};

/**
 * Gets the CPU pointer of the buffer of a GPU memory block.
 * @param rBlock Memory block.
 * @return Pointer to the start of the buffer.
 */
template <typename T>
T* getBufferPtr(const agl::GPUMemBlock<T>& rBlock) {
    return reinterpret_cast<T*>(
        static_cast<u8*>(nvnMemoryPoolMap(rBlock.getMemoryPool()->getDriverPool())) +
        rBlock.getByteOffset());
}

/**
 * Gets the absolute value of a float with a compare instead of fabs.
 * @param value Value.
 * @return Absolute value.
 */
inline f32 absf(f32 value) {
    return value > 0.0f ? value : -value;
}

/**
 * Calculates the height of a mip level of a texture.
 * @param rTexture Texture.
 * @param mipLevel Mip level.
 * @return Height of the mip level.
 */
inline s32 calcMipHeight(const agl::TextureData& rTexture, s32 mipLevel) {
    return sead::Mathi::max(rTexture.getMinHeight_(), rTexture.getHeight() >> mipLevel);
}

/**
 * Draws the elements of an index stream with the currently active vertex attribute.
 * @param pContext Draw context.
 * @param rStream Index stream to draw.
 */
inline void drawIndexStream(agl::DrawContext* pContext, const agl::IndexStream& rStream) {
    u32 count = rStream.getCount();

    if (count != 0) {
        NVNdrawPrimitive primitive = rStream.getPrimitiveType();
        NVNcommandBuffer* pCommandBuffer = pContext->getNvnCommandBuffer();
        NVNbufferAddress address = nvnBufferGetAddress(rStream.getNvnBuffer());
        nvnCommandBufferDrawElements(pCommandBuffer, primitive,
                                     NVNindexType(rStream.getFormat()), count, address);
    }
}

/**
 * Searches a uniform by name and sets a color to it.
 * @param pContext Draw context.
 * @param pProgram Shader program.
 * @param pName Uniform name.
 * @param rColor Color to set.
 */
inline void setColorUniform(agl::DrawContext* pContext, const agl::ShaderProgram* pProgram,
                            const char* pName, const sead::Color4f& rColor) {
    agl::UniformLocation location(pName);
    location.search(*pProgram);
    location.setUniform(pContext, 4, &rColor);
}

/**
 * Gets the blur power clamped to [0, 1].
 * @param pParam Vignetting parameter.
 * @return Clamped blur power.
 */
inline f32 calcBlurPowerRate(const al::VignettingParam* pParam) {
    return sead::Mathf::clamp(pParam->getBlurPower(), 0.0f, 1.0f);
}

/**
 * Calculates the number of mip levels of the blur texture.
 * @param powerRate Clamped blur power.
 * @param pParam Vignetting parameter.
 * @return Number of mip levels (at least one).
 */
inline s32 calcBlurMipLevelNum(f32 powerRate, const al::VignettingParam* pParam) {
    return sead::Mathi::max(sead::Mathf::ceil(powerRate * pParam->getBlurPowerMax()), 1);
}

}  // namespace

namespace al {

/**
 * Calculates the layout position of the player normalized to [-1, 1].
 * @param pPos Output position.
 * @return Whether a player was found.
 */
inline bool VignettingDrawer::tryCalcPlayerScreenPos(sead::Vector2f* pPos) const {
    LiveActor* player = tryGetPlayerActor(mPlayerHolder, 0);

    if (player == nullptr) {
        return false;
    }

    sead::Vector3f trans = getTrans(player);
    trans.y += 100.0f;
    calcLayoutPosFromWorldPos(pPos, mCameraInfo, trans, 0);
    pPos->x /= static_cast<u32>(getDisplayWidth()) * 0.5f;
    pPos->y /= static_cast<u32>(getDisplayHeight()) * 0.5f;
    return true;
}

/**
 * Constructs the vignetting drawing parameters.
 */
VignettingParam::VignettingParam() {
    mParamObj = new ParameterObj();
    mIsBlurEnable = new ParameterBool(false, mParamObj, "IsBlurEnable", "IsBlurEnable", "", true);
    mIsBlurPlayerEnable = new ParameterBool(false, mParamObj, "IsBlurPlayerEnable", "IsBlurPlayerEnable", "", true);
    mBlurType = new ParameterS32(0, mParamObj, "BlurType", "BlurType", "Min=0, Max=1", true);
    mBlurRange = new ParameterF32(0.25f, mParamObj, "BlurRange", "BlurRange", "Min=0.f, Max=1.f", true);
    mBlurChangeRange = new ParameterF32(2.0f, mParamObj, "BlurChangeRange", "BlurChangeRange", "Min=0.f, Max=1.f", true);
    mBlurPower = new ParameterF32(1.0f, mParamObj, "BlurPower", "BlurPower", "Min=0.f, Max=1.f", true);
    mBlurPowerMax = new ParameterF32(2.0f, mParamObj, "BlurPowerMax", "BlurPowerMax", "Min=0.f, Max=6.f", true);
    mBlurScale = new ParameterV2f({1.0f, 1.0f}, mParamObj, "BlurScale", "BlurScale", "Min=0.f, Max=2.f", true);
    mBlurOffset = new ParameterV2f({0.0f, 0.0f}, mParamObj, "BlurOffset", "BlurOffset", "Min=-1.f, Max=1.f", true);
    mBlurQuality = new ParameterS32(1, mParamObj, "BlurQuality", "BlurQuality", "Min=0, Max=1", true);
    mIsColorEnable = new ParameterBool(false, mParamObj, "IsColorEnable", "IsColorEnable", "", true);
    mIsColorPlayerEnable = new ParameterBool(false, mParamObj, "IsColorPlayerEnable", "IsColorPlayerEnable", "", true);
    mColorType = new ParameterS32(0, mParamObj, "ColorType", "ColorType", "Min=0, Max=1", true);
    mColorRange = new ParameterF32(0.25f, mParamObj, "ColorRange", "ColorRange", "Min=0.f, Max=1.f", true);
    mColorChangeRange = new ParameterF32(2.0f, mParamObj, "ColorChangeRange", "ColorChangeRange", "Min=0.f, Max=1.f", true);
    mColorScale = new ParameterV2f({1.0f, 1.0f}, mParamObj, "ColorScale", "ColorScale", "Min=0.f, Max=2.f", true);
    mColorOffset = new ParameterV2f({0.0f, 0.0f}, mParamObj, "ColorOffset", "ColorOffset", "Min=-1.f, Max=1.f", true);
    mColor = new ParameterC4f(sead::Color4f(0.0f, 0.0f, 0.0f, 0.75f), mParamObj, "Color", "Color", "Min=0.f, Max=1.f", true);
    mColorBlendType = new ParameterS32(0, mParamObj, "ColorBlendType", "ColorBlendType", "Min=0, Max=3", true);
}

/**
 * Creates the parameter interpolation and the vignetting meshes.
 * @param pShaderHolder Shader holder to get the shader from.
 * @param pPlayerHolder Player holder used to center the effect on the player.
 * @param pCameraInfo Camera info used to project the player position.
 */
VignettingDrawer::VignettingDrawer(ShaderHolder* pShaderHolder, const PlayerHolder* pPlayerHolder,
                                   const SceneCameraInfo* pCameraInfo)
    : mRequestInterp(new ParamRequestInterp()), mPlayerHolder(pPlayerHolder),
      mCameraInfo(pCameraInfo) {
    ParamRequestInterp* interp = mRequestInterp;
    interp->mCurrentParam = new VignettingParam();
    interp->mStartParam = new VignettingParam();
    interp->mEndParam = new VignettingParam();
    interp->mRequestParam = new VignettingParam();
    mShaderProgram = pShaderHolder->getShaderProgram("alVignettingShader");

    for (s32 i = 0; i < cMeshNum; i++) {
        s32 divNum = cDivNum[i];
        Mesh& mesh = mMeshes[i];
        mesh.mVertexBlock.allocBuffer(divNum * cRingNum, getCurrentHeap(), 8,
                                      agl::MemoryAttribute::Default);
        agl::GPUMemAddr<Vertex> vertexAddr(mesh.mVertexBlock, 0);

        const agl::GPUMemBlock<Vertex>& vertexBlock = mesh.mVertexBlock;
        s32 vertexIndex = 0;
        for (s32 ring = 0; ring < cRingNum; ring++) {
            if (i == 1) {
                sead::Vector2f param(cRingAlpha[ring], ring);
                getBufferPtr(vertexBlock)[vertexIndex].mPos = sead::Vector2f(-1.0f, 1.0f);
                getBufferPtr(vertexBlock)[vertexIndex].mParam = param;
                getBufferPtr(vertexBlock)[vertexIndex + 1].mPos = sead::Vector2f(-1.0f, -1.0f);
                getBufferPtr(vertexBlock)[vertexIndex + 1].mParam = param;
                getBufferPtr(vertexBlock)[vertexIndex + 2].mPos = sead::Vector2f(1.0f, -1.0f);
                getBufferPtr(vertexBlock)[vertexIndex + 2].mParam = param;
                getBufferPtr(vertexBlock)[vertexIndex + 3].mPos = sead::Vector2f(1.0f, 1.0f);
                getBufferPtr(vertexBlock)[vertexIndex + 3].mParam = param;
                vertexIndex += 4;
            } else if (i == 0) {
                for (s32 j = 0; j < divNum; j++) {
                    f32 angle = j * sead::Mathf::pi2() / divNum;
                    f32 c = cosf(angle);
                    f32 s = sinf(angle);
                    getBufferPtr(vertexBlock)[vertexIndex].mPos = sead::Vector2f(c, s);
                    getBufferPtr(vertexBlock)[vertexIndex].mParam =
                        sead::Vector2f(cRingAlpha[ring], ring);
                    vertexIndex++;
                }
            }
        }

        mesh.mVertexBuffer = new agl::VertexBuffer();
        mesh.mVertexBuffer->setUpBuffer(agl::ConstGPUMemVoidAddr(mesh.mVertexBlock, 0),
                                        sizeof(Vertex),
                                        static_cast<u32>(mesh.mVertexBlock.getSize()));
        mesh.mVertexBuffer->setUpStream(0, agl::VertexStreamFormat(0x16), 0, false);
        mesh.mVertexBuffer->setUpStream(1, agl::VertexStreamFormat(0x16), 8, false);
        mesh.mVertexAttribute = new agl::VertexAttribute();
        mesh.mVertexAttribute->create(1, getCurrentHeap());
        mesh.mVertexAttribute->setVertexStream(0, mesh.mVertexBuffer, 0);
        mesh.mVertexAttribute->setVertexStream(1, mesh.mVertexBuffer, 1);
        mesh.mVertexAttribute->setUp();

        mesh.mIndexBlock.allocBuffer(divNum * (cRingNum - 1) * 6, getCurrentHeap(), 4,
                                     agl::MemoryAttribute::Default);
        agl::GPUMemAddr<u16> indexAddr(mesh.mIndexBlock, 0);

        const agl::GPUMemBlock<u16>& indexBlock = mesh.mIndexBlock;
        s32 index = 0;
        for (s32 ring = 0; ring < cRingNum - 1; ring++) {
            s32 inner = ring * divNum;
            s32 outer = inner + divNum;

            for (s32 j = 0; j < divNum; j++) {
                getBufferPtr(indexBlock)[index] = inner + j;
                s32 next = (j + 1) % divNum;
                getBufferPtr(indexBlock)[index + 1] = outer + next;
                getBufferPtr(indexBlock)[index + 2] = inner + next;
                getBufferPtr(indexBlock)[index + 3] = inner + j;
                getBufferPtr(indexBlock)[index + 4] = outer + j;
                getBufferPtr(indexBlock)[index + 5] = outer + next;
                index += 6;
            }
        }

        mesh.mIndexStream = new agl::IndexStream();
        mesh.mIndexStream->setUpStream(agl::GPUMemAddr<u16>(mesh.mIndexBlock, 0),
                                       mesh.mIndexBlock.getSize() / sizeof(u16));
        mesh.mIndexStream->setPrimitiveType(NVN_DRAW_PRIMITIVE_TRIANGLES);
    }
}

/**
 * Destroys the vertex and index buffers of the meshes.
 */
VignettingDrawer::~VignettingDrawer() {
    for (s32 i = 0; i < cMeshNum; i++) {
        Mesh& mesh = mMeshes[i];

        if (mesh.mIndexStream != nullptr) {
            delete mesh.mIndexStream;
            mesh.mIndexStream = nullptr;
        }

        if (mesh.mVertexBuffer != nullptr) {
            delete mesh.mVertexBuffer;
            mesh.mVertexBuffer = nullptr;
        }

        if (mesh.mVertexAttribute != nullptr) {
            delete mesh.mVertexAttribute;
            mesh.mVertexAttribute = nullptr;
        }
    }
}

/**
 * Finishes initialization of the parameter interpolation.
 */
void VignettingDrawer::endInit() {
    mRequestInterp->endInit();
}

/**
 * Clears the parameter request.
 */
void VignettingDrawer::clearRequest() {
    mRequestInterp->clearRequest();
}

/**
 * Updates the parameter interpolation.
 */
void VignettingDrawer::update() {
    mRequestInterp->updateInterp();
}

/**
 * Requests a parameter.
 * @param priority Request priority.
 * @param step Interpolation steps.
 * @param rParam Requested parameter.
 */
void VignettingDrawer::requestParam(s32 priority, s32 step, const VignettingParam& rParam) {
    mRequestInterp->requestParam(priority, step, rParam);
}

/**
 * Draws the vignetting blur and color onto a render buffer.
 * @param pContext Draw context.
 * @param rBuffer Render buffer to draw onto.
 */
void VignettingDrawer::draw(agl::DrawContext* pContext, const agl::RenderBuffer& rBuffer) const {
    const VignettingParam* param = getCurrentParam();

    if (isEnableBlur()) {
        const agl::TextureData* color = rBuffer.getRenderTargetColor();
        agl::TextureData* blur = allocBuffer(pContext, *color, param);
        drawBlur(pContext, blur, *color, param);
        drawCompose(pContext, blur, rBuffer, param);
        freeBuffer(blur);
    }

    if (isEnableColor()) {
        drawVignetting(pContext, rBuffer, param);
    }
}

/**
 * Gets the current interpolated parameter.
 * @return Current parameter.
 */
const VignettingParam* VignettingDrawer::getCurrentParam() const {
    return static_cast<const VignettingParam*>(mRequestInterp->getCurrentParam());
}

/**
 * Checks whether the blur is enabled.
 * @return Whether the blur is drawn.
 */
bool VignettingDrawer::isEnableBlur() const {
    return mIsEnableBlur && getCurrentParam()->isBlurEnable();
}

/**
 * Allocates the mipmapped half resolution blur texture.
 * @param pContext Draw context.
 * @param rSrc Source texture.
 * @param pParam Vignetting parameter.
 * @return Allocated texture.
 */
agl::TextureData* VignettingDrawer::allocBuffer(agl::DrawContext* pContext,
                                                const agl::TextureData& rSrc,
                                                const VignettingParam* pParam) const {
    agl::utl::DynamicTextureAllocator* allocator = agl::utl::DynamicTextureAllocator::instance();
    s32 mipLevelNum = calcBlurMipLevelNum(calcBlurPowerRate(pParam), pParam);
    u32 width = rSrc.getWidth(0) * 0.5f;
    u32 height = rSrc.getHeight(0) * 0.5f;
    agl::TextureFormat format = agl::TextureFormat(rSrc.getTextureFormat());
    return allocator->alloc(pContext, "vignetting_blur", format, width, height, mipLevelNum,
                            nullptr, agl::utl::DynamicTextureAllocator::AllocateType(0), true,
                            false);
}

/**
 * Draws the blurred mip chain of a texture.
 * @param pContext Draw context.
 * @param pBlur Blur texture to draw into.
 * @param rSrc Source texture.
 * @param pParam Vignetting parameter.
 */
void VignettingDrawer::drawBlur(agl::DrawContext* pContext, agl::TextureData* pBlur,
                                const agl::TextureData& rSrc,
                                const VignettingParam* pParam) const {
    agl::RenderBuffer renderBuffer;
    agl::RenderTargetColor target;
    renderBuffer.setRenderTargetColorNullAll();
    renderBuffer.setRenderTargetDepth(nullptr);
    renderBuffer.setRenderTargetColor(&target);
    target.applyTextureData(*pBlur, 0, 0);

    sead::GraphicsContext graphicsContext;
    graphicsContext.setBlendEnable(false);
    graphicsContext.setDepthEnable(false, false);
    graphicsContext.setColorMask(true, true, true, false);
    graphicsContext.apply(pContext);

    agl::TextureSampler blurSampler;
    blurSampler.setFilter(1, 1, 1);
    blurSampler.applyTextureData(*pBlur);
    agl::TextureSampler srcSampler;
    srcSampler.setFilter(1, 1, 0);
    srcSampler.applyTextureData(rSrc);

    const char* macros[] = {"PASS",             "VIGNETTING_BLEND", "VIGNETTING_BLACK",
                            "VIGNETTING_COLOR", "VIGNETTING_BLUR",  "BLUR_QUALITY"};
    const char* values[] = {"PASS",             "VIGNETTING_BLEND", "VIGNETTING_BLACK",
                            "VIGNETTING_COLOR", "VIGNETTING_BLUR",  "BLUR_QUALITY"};
    s32 index = ShaderSearchImpl::searchMacroIndex(macros, "PASS");

    if (index != -1) {
        values[index] = "0";
    }

    index = ShaderSearchImpl::searchMacroIndex(macros, "VIGNETTING_BLEND");

    if (index != -1) {
        values[index] = "0";
    }

    index = ShaderSearchImpl::searchMacroIndex(macros, "VIGNETTING_BLACK");

    if (index != -1) {
        values[index] = "0";
    }

    index = ShaderSearchImpl::searchMacroIndex(macros, "VIGNETTING_COLOR");

    if (index != -1) {
        values[index] = "0";
    }

    index = ShaderSearchImpl::searchMacroIndex(macros, "VIGNETTING_BLUR");

    if (index != -1) {
        values[index] = "0";
    }

    index = ShaderSearchImpl::searchMacroIndex(macros, "BLUR_QUALITY");

    if (index != -1) {
        values[index] = pParam->getBlurQuality() == 1 ? "1" : "0";
    }

    const agl::ShaderProgram* program = mShaderProgram->searchVariation(6, macros, values);
    program->activate(pContext, true);

    s32 mipLevelNum = pBlur->getMipLevelNum();
    for (s32 i = 0; i < mipLevelNum; i++) {
        u32 srcWidth;
        if (i != 0) {
            blurSampler.setLod(i - 1, i - 1, 0.0f);
            activateSampler(pContext, &blurSampler, program, "cTexColor");
            srcWidth = pBlur->getMipWidth(i);
        } else {
            activateSampler(pContext, &srcSampler, program, "cTexColor");
            srcWidth = pBlur->getWidth(0);
        }

        sead::Vector4f texParam(0.5f / srcWidth,
                                0.5f / static_cast<u32>(calcMipHeight(*pBlur, i)), 0.0f, 0.0f);
        setPostProcessingUniform(pContext, program, "uTexParam", texParam);

        renderBuffer.getRenderTargetColor()->setMipLevel(i);
        const agl::RenderTargetColor* color = renderBuffer.getRenderTargetColor();
        s32 width = color->getMipWidth(i);
        s32 height = calcMipHeight(*color, i);
        renderBuffer.setPhysicalArea(sead::BoundBox2f(0.0f, 0.0f, width, height));
        renderBuffer.setVirtualSize(sead::Vector2f(width, height));

        sead::Viewport viewport(renderBuffer);
        viewport.apply(pContext, renderBuffer);
        renderBuffer.bind(pContext);
        drawPostProcessingQuad(pContext);
        renderBuffer.getRenderTargetColor()->invalidateGPUCache(pContext);
    }
}

/**
 * Composes the blurred texture onto a render buffer.
 * @param pContext Draw context.
 * @param pBlur Blur texture.
 * @param rBuffer Render buffer to draw onto.
 * @param pParam Vignetting parameter.
 */
void VignettingDrawer::drawCompose(agl::DrawContext* pContext, agl::TextureData* pBlur,
                                   const agl::RenderBuffer& rBuffer,
                                   const VignettingParam* pParam) const {
    sead::GraphicsContext graphicsContext;
    graphicsContext.setDepthEnable(false, false);
    graphicsContext.setColorMask(true, true, true, false);
    graphicsContext.apply(pContext);

    sead::Viewport viewport(rBuffer);
    viewport.apply(pContext, rBuffer);
    rBuffer.bind(pContext);

    const char* macros[] = {"PASS", "VIGNETTING_BLEND", "VIGNETTING_BLACK", "VIGNETTING_COLOR",
                            "VIGNETTING_BLUR"};
    const char* values[] = {"PASS", "VIGNETTING_BLEND", "VIGNETTING_BLACK", "VIGNETTING_COLOR",
                            "VIGNETTING_BLUR"};
    s32 index = ShaderSearchImpl::searchMacroIndex(macros, "PASS");

    if (index != -1) {
        values[index] = "1";
    }

    index = ShaderSearchImpl::searchMacroIndex(macros, "VIGNETTING_BLEND");

    if (index != -1) {
        values[index] = "0";
    }

    index = ShaderSearchImpl::searchMacroIndex(macros, "VIGNETTING_BLACK");

    if (index != -1) {
        values[index] = "0";
    }

    index = ShaderSearchImpl::searchMacroIndex(macros, "VIGNETTING_COLOR");

    if (index != -1) {
        values[index] = "0";
    }

    index = ShaderSearchImpl::searchMacroIndex(macros, "VIGNETTING_BLUR");

    if (index != -1) {
        values[index] = "1";
    }

    const agl::ShaderProgram* program = mShaderProgram->searchVariation(5, macros, values);
    program->activate(pContext, true);

    agl::TextureSampler sampler;
    sampler.setFilter(1, 1, 2);
    sampler.setLod(0.0f, pBlur->getMipLevelNum() - 1.0f, 0.0f);
    sampler.applyTextureData(*pBlur);
    activateSampler(pContext, &sampler, program, "cTexMipMap");

    const agl::TextureData* color = rBuffer.getRenderTargetColor();
    s32 width = color->getWidth(0);
    s32 height = color->getHeight(0);

    sead::Vector2f scale = pParam->getBlurScale();
    if (mScreenScaleInfo != nullptr && mScreenScaleInfo->mIsValid &&
        mScreenScaleInfo->mIsEnable) {
        scale.x *= 0.78;
        scale.y *= 1.5f;
    }

    scale.x = sead::Mathf::clampMin(scale.x, 0.001f);
    scale.y = sead::Mathf::clampMin(scale.y, 0.001f);

    sead::Vector2f offset = pParam->getBlurOffset();
    if (pParam->isBlurPlayerEnable()) {
        tryCalcPlayerScreenPos(&offset);
    }

    f32 range = sead::Mathf::max(1.0f - pParam->getBlurRange(), 0.0f);
    f32 maxOffset = sead::Mathf::max(absf(offset.x), absf(offset.y));
    f32 minScale = sead::Mathf::min(scale.x, scale.y);
    sead::Vector4f radius;
    radius.x = range;
    radius.y = range;
    radius.z = range + (1.0f - range) * pParam->getBlurChangeRange();
    radius.w = 1.0f / minScale * 1.1f + maxOffset;
    setPostProcessingUniform(pContext, program, "uVignettingRadius", radius);

    f32 aspectX = 1.0f;
    f32 aspectY = 1.0f;
    if (pParam->getBlurType() == 0) {
        f32 diagonal = nerd::sqrt(width * width + height * height);
        aspectX = diagonal / width;
        aspectY = diagonal / height;
    }

    sead::Vector4f vignettingParam(0.0f, 0.0f, 0.0f, 0.0f);
    vignettingParam.x = scale.x * aspectX;
    vignettingParam.y = scale.y * aspectY;
    f32 powerRate = calcBlurPowerRate(pParam);
    s32 mipLevelNum = calcBlurMipLevelNum(powerRate, pParam);
    vignettingParam.z = powerRate;
    vignettingParam.w = mipLevelNum - 1;
    setPostProcessingUniform(pContext, program, "uVignettingParam", vignettingParam);

    setPostProcessingUniform(pContext, program, "uVignettingTrans",
                             sead::Vector4f(offset.x, offset.y, 0.0f, 0.0f));
    setPostProcessingUniform(pContext, program, "uExp2MipLevelMax",
                             exp2f(pParam->getBlurPowerMax()));

    mMeshes[pParam->getBlurType()].mVertexAttribute->activate(pContext);
    drawIndexStream(pContext, *mMeshes[pParam->getBlurType()].mIndexStream);
}

/**
 * Frees a texture allocated by allocBuffer.
 * @param pTexture Texture to free.
 */
void VignettingDrawer::freeBuffer(agl::TextureData* pTexture) const {
    agl::utl::DynamicTextureAllocator::instance()->free(pTexture);
}

/**
 * Checks whether the color vignetting is enabled.
 * @return Whether the color is drawn.
 */
bool VignettingDrawer::isEnableColor() const {
    return mIsEnableColor && getCurrentParam()->isColorEnable();
}

/**
 * Draws the colored vignetting onto a render buffer.
 * @param pContext Draw context.
 * @param rBuffer Render buffer to draw onto.
 * @param pParam Vignetting parameter.
 */
void VignettingDrawer::drawVignetting(agl::DrawContext* pContext, const agl::RenderBuffer& rBuffer,
                                      const VignettingParam* pParam) const {
    sead::GraphicsContext graphicsContext;
    graphicsContext.setDepthEnable(false, false);
    graphicsContext.setColorMask(true, true, true, false);

    switch (pParam->getColorBlendType()) {
    case 0:
        graphicsContext.setBlendFactor(0, 5, 6);
        graphicsContext.setBlendEquation(0, 1);
        break;
    case 1:
        graphicsContext.setBlendFactor(0, 1, 3);
        graphicsContext.setBlendEquation(0, 1);
        break;
    case 2:
        graphicsContext.setBlendFactor(0, 5, 2);
        graphicsContext.setBlendEquation(0, 1);
        break;
    case 3:
        graphicsContext.setBlendFactor(0, 10, 2);
        graphicsContext.setBlendEquation(0, 1);
        break;
    }

    graphicsContext.apply(pContext);

    sead::Viewport viewport(rBuffer);
    viewport.apply(pContext, rBuffer);
    rBuffer.bind(pContext);

    const char* macros[] = {"PASS", "VIGNETTING_BLEND", "VIGNETTING_BLACK", "VIGNETTING_COLOR",
                            "VIGNETTING_BLUR"};
    const char* values[] = {"PASS", "VIGNETTING_BLEND", "VIGNETTING_BLACK", "VIGNETTING_COLOR",
                            "VIGNETTING_BLUR"};
    s32 index = ShaderSearchImpl::searchMacroIndex(macros, "PASS");

    if (index != -1) {
        values[index] = "1";
    }

    s32 blendType = pParam->getColorBlendType();
    const char* blendValue = blendType == 1 ? "1" : blendType == 3 ? "2" : "0";
    index = ShaderSearchImpl::searchMacroIndex(macros, "VIGNETTING_BLEND");

    if (index != -1) {
        values[index] = blendValue;
    }

    const sead::Color4f& vignettingColor = pParam->getColor();
    bool isBlack =
        vignettingColor.r == 0.0f && vignettingColor.g == 0.0f && vignettingColor.b == 0.0f;
    index = ShaderSearchImpl::searchMacroIndex(macros, "VIGNETTING_BLACK");

    if (index != -1) {
        values[index] = isBlack && pParam->isColorEnable() ? "1" : "0";
    }

    index = ShaderSearchImpl::searchMacroIndex(macros, "VIGNETTING_COLOR");

    if (index != -1) {
        values[index] = !isBlack && pParam->isColorEnable() ? "1" : "0";
    }

    index = ShaderSearchImpl::searchMacroIndex(macros, "VIGNETTING_BLUR");

    if (index != -1) {
        values[index] = "0";
    }

    const agl::ShaderProgram* program = mShaderProgram->searchVariation(5, macros, values);
    program->activate(pContext, true);

    const agl::TextureData* color = rBuffer.getRenderTargetColor();
    s32 width = color->getWidth(0);
    s32 height = color->getHeight(0);

    sead::Vector2f scale = pParam->getColorScale();
    if (mScreenScaleInfo != nullptr && mScreenScaleInfo->mIsValid &&
        mScreenScaleInfo->mIsEnable) {
        scale.x *= 0.78;
        scale.y *= 1.5f;
    }

    scale.x = sead::Mathf::clampMin(scale.x, 0.001f);
    scale.y = sead::Mathf::clampMin(scale.y, 0.001f);

    sead::Vector2f offset = pParam->getColorOffset();
    if (pParam->isColorPlayerEnable() && mPlayerHolder != nullptr && mCameraInfo != nullptr) {
        tryCalcPlayerScreenPos(&offset);
    }

    f32 range = sead::Mathf::max(1.0f - pParam->getColorRange(), 0.0f);
    f32 maxOffset = sead::Mathf::max(absf(offset.x), absf(offset.y));
    f32 minScale = sead::Mathf::min(scale.x, scale.y);
    sead::Vector4f radius;
    radius.x = range;
    radius.y = range;
    radius.z = range + (1.0f - range) * pParam->getColorChangeRange();
    radius.w = 1.0f / minScale * 1.1f + maxOffset;
    setPostProcessingUniform(pContext, program, "uVignettingRadius", radius);

    f32 aspectY = 1.0f;
    f32 aspectX = 1.0f;
    if (pParam->getColorType() == 0) {
        f32 diagonal = nerd::sqrt(width * width + height * height);
        aspectX = diagonal / width;
        aspectY = diagonal / height;
    }

    sead::Vector4f vignettingParam(scale.x * aspectX, scale.y * aspectY, 0.0f, 0.0f);
    setPostProcessingUniform(pContext, program, "uVignettingParam", vignettingParam);

    setColorUniform(pContext, program, "uVignettingColor", pParam->getColor());
    setPostProcessingUniform(pContext, program, "uVignettingTrans",
                             sead::Vector4f(offset.x, offset.y, 0.0f, 0.0f));

    mMeshes[pParam->getColorType()].mVertexAttribute->activate(pContext);
    drawIndexStream(pContext, *mMeshes[pParam->getColorType()].mIndexStream);
}

/**
 * Allocates the blur map and draws the blurred mip chain of a texture into it.
 * @param pContext Draw context.
 * @param rSrc Source texture.
 */
void VignettingDrawer::drawBlurMap(agl::DrawContext* pContext, const agl::TextureData& rSrc) const {
    if (!isEnableBlur()) {
        return;
    }

    const VignettingParam* param = getCurrentParam();
    mBlurMap = allocBuffer(pContext, rSrc, param);
    drawBlur(pContext, mBlurMap, rSrc, param);
}

/**
 * Frees the blur map.
 */
void VignettingDrawer::freeBlurMap() const {
    if (!isEnableBlur()) {
        return;
    }

    freeBuffer(mBlurMap);
    mBlurMap = nullptr;
}

/**
 * Sets the blur map and the vignetting uniforms to a compose shader.
 * @param pContext Draw context.
 * @param pProgram Shader program to set up.
 */
void VignettingDrawer::setupCompose(agl::DrawContext* pContext,
                                    const agl::ShaderProgram* pProgram) {
    if (!isEnableBlur()) {
        return;
    }

    const VignettingParam* param = getCurrentParam();

    agl::TextureSampler sampler;
    sampler.setFilter(1, 1, 2);
    sampler.setLod(0.0f, mBlurMap->getMipLevelNum() - 1.0f, 0.0f);
    sampler.applyTextureData(*mBlurMap);
    activateSampler(pContext, &sampler, pProgram, "uCameraBlurFrame");

    sead::Vector4f vignettingParam(0.0f, 0.0f, 0.0f, 0.0f);
    vignettingParam.x = sead::Mathf::clamp(param->getBlurChangeRange() * 0.1f, 0.0f, 1.0f);
    vignettingParam.y = sead::Mathf::clamp(1.0f - param->getBlurRange(), 0.0f, 1.0f);
    vignettingParam.z = calcBlurPowerRate(param);
    vignettingParam.w = calcBlurMipLevelNum(vignettingParam.z, param) - 1;
    setPostProcessingUniform(pContext, pProgram, "uVignettingParam", vignettingParam);

    sead::Vector2f offset = param->getBlurOffset();
    if (param->isBlurPlayerEnable() &&
        tryCalcPlayerScreenPos(&offset)) {
        offset.y = -offset.y;
    }

    sead::Vector4f vignettingParam2;
    vignettingParam2.x = offset.x + 0.5f;
    vignettingParam2.y = offset.y + 0.5f;
    vignettingParam2.z = exp2f(param->getBlurPowerMax());
    vignettingParam2.w = 0.1f;
    setPostProcessingUniform(pContext, pProgram, "uVignettingParam2", vignettingParam2);
}

}  // namespace al
