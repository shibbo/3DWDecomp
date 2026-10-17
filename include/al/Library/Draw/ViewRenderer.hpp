#pragma once

#include <basis/seadTypes.h>
#include <common/aglShaderEnum.h>
#include <common/aglTextureSampler.h>
#include <postfx/aglBloom.h>
#include <postfx/aglHDRCompose.h>

namespace agl {
class RenderBuffer;
class TextureData;
}  // namespace agl

namespace sead {
class DirectResource;
class Viewport;
}  // namespace sead

namespace al {
class DeferredRendering;
class FullScreenQuadModel;
class GraphicsSystemInfo;
class LiveActorKit;
class ReducedBufferRenderer;
class RenderVariables;
class SceneCameraInfo;
class ScreenFader;
class SimpleModelEnv;
class SSR;

/**
 * Extra pass drawn into the albedo G-buffer after the light buffer has been created.
 * The concrete type is unknown.
 */
class ViewRendererGBufferDrawer {
public:
    virtual void draw(const agl::RenderBuffer& rRenderBuffer,
                      const sead::Viewport& rViewport) const = 0;
};

/**
 * Interface of the extra pass stored in ViewRenderer::mGBufferDrawer, as implemented by
 * DemoSingleModeEndingScene (the override's mangled name proves this signature).
 * @note ViewRendererGBufferDrawer is presumably the same interface with a guessed signature.
 */
class IUseFrameBufferDrawer {
public:
    virtual void drawToFrameBuffer(const agl::RenderBuffer* pRenderBuffer,
                                   const sead::Viewport* pViewport) = 0;
};

/**
 * Renders the views of a scene: deferred/forward 3D passes, mirrors, HDR post effects and the
 * system passes (depth shadow, cube maps).
 */
class ViewRenderer {
public:
    ViewRenderer(GraphicsSystemInfo* pInfo);
    virtual ~ViewRenderer();

    virtual void preDrawGraphics(const SceneCameraInfo* pCameraInfo);
    virtual agl::ShaderMode drawView(s32 viewIndex, s32 index, LiveActorKit* pKit,
                                     const SceneCameraInfo* pCameraInfo,
                                     const agl::RenderBuffer* pBuffer,
                                     const sead::Viewport& rViewport, bool isDrawHdrEffect,
                                     bool isFadeScreen, agl::ShaderMode shaderMode) const;
    virtual agl::ShaderMode drawSystem(LiveActorKit* pKit, agl::ShaderMode shaderMode) const;
    virtual const agl::TextureData* drawHdr(s32 viewIndex, const RenderVariables& rVariables,
                                            bool, bool, bool isPlayerEffect,
                                            agl::ShaderMode shaderMode) const;
    virtual agl::ShaderMode drawMirror(s32 viewIndex, RenderVariables* pVariables,
                                       bool isPlayerEffect, agl::ShaderMode shaderMode) const;

    void updatePreDraw();
    void setReducedEffectRender(bool isEnable, bool isHdr);
    void setSingleModeRendering();
    void dangerIndicatorEnable(bool isEnable);
    void enableSSR();

    SimpleModelEnv* getSimpleModelEnv() const { return mSimpleModelEnv; }
    void setForceFilterAA(bool isForce) { mIsForceFilterAA = isForce; }

    void setFrameBufferDrawer(IUseFrameBufferDrawer* pDrawer) {
        mGBufferDrawer = reinterpret_cast<ViewRendererGBufferDrawer*>(pDrawer);
    }

protected:
    GraphicsSystemInfo* mGraphicsSystemInfo;
    SimpleModelEnv* mSimpleModelEnv = nullptr;
    DeferredRendering* mDeferredRendering = nullptr;
    ScreenFader* mScreenFader = nullptr;
    agl::pfx::Bloom mBloom;
    agl::pfx::HDRCompose mHDRCompose;
    ReducedBufferRenderer* mReducedBufferRenderer = nullptr;
    s32 mReducedEffectRenderCount[2];
    sead::DirectResource* mDitherImageResource = nullptr;
    agl::TextureData* mDitherTexture = nullptr;
    agl::TextureSampler mDitherSampler;
    SSR* mSSR = nullptr;
    bool mIsFastRendering = false;
    bool mIsForceFilterAA = false;
    bool mIsDrawEffect = false;
    bool _e73 = false;
    bool _e74 = false;
    s32 _e78 = 0;
    bool _e7c = false;
    FullScreenQuadModel* mFullScreenQuadModel;
    ViewRendererGBufferDrawer* mGBufferDrawer = nullptr;
};

static_assert(sizeof(ViewRenderer) == 0xe90);

}  // namespace al
