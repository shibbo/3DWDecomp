#include "LuigiBros/Emulator/Emu.hpp"
#include "LuigiBros/vessel.hpp"
#include "LuigiBros/Vessel/screenfilterCpu.hpp"
#include "common/aglRenderBuffer.h"
#include "common/aglShaderProgram.h"
#include "common/aglShaderLocation.h"
#include "common/aglTextureSampler.h"
#include <gfx/seadViewport.h>
#include <gfx/seadTextWriter.h>
#include <gfx/seadGraphicsContext.h>
#include "al/Library/Framework/GameFrameworkNx.hpp"
#include "al/Library/Memory/Util.hpp"
#include "al/Library/Shader/ForwardRendering/ShaderHolder.hpp"
#include "detail/aglPrivateResource.h"
#include "LuigiBros/Vessel/gamescreen.hpp"
#include "utility/aglDynamicTextureAllocator.h"
#include <nn/gfx/gfx_Shader.h>

/**
 * @brief Sets visibility for one or both game screens on selected monitors.
 * @param monitor Target monitor or both monitors.
 * @param game Target game screen or both screens.
 * @param enabled New visibility state.
 */
void gamescreen::screen(MONITER monitor, GAMESCREEN game, bool enabled) {
    if (monitor == MonitorBoth) {
        if (game == GameScreenBoth) {
            mMonitors[0].screens[0].enabled = enabled;
            mMonitors[0].screens[1].enabled = enabled;
            mMonitors[1].screens[0].enabled = enabled;
            mMonitors[1].screens[1].enabled = enabled;
        } else {
            unsigned long index = game;
            mMonitors[0].screens[index].enabled = enabled;
            mMonitors[1].screens[index].enabled = enabled;
        }
    } else if (game == GameScreenBoth) {
        mMonitors[monitor].screens[0].enabled = enabled;
        mMonitors[monitor].screens[1].enabled = enabled;
    } else {
        mMonitors[monitor].screens[game].enabled = enabled;
    }
    mMonitors[0].enabled = mMonitors[0].screens[0].enabled | mMonitors[0].screens[1].enabled;
    mMonitors[1].enabled = mMonitors[1].screens[0].enabled | mMonitors[1].screens[1].enabled;
}

/**
 * @brief Returns this port's fixed screen-query result.
 * @param monitor Unused monitor selection.
 * @param game Unused game screen selection.
 * @return False.
 */
bool gamescreen::IsScreen(MONITER monitor, GAMESCREEN game) { return false; }

/**
 * @brief Sets the selected monitor's GPU filter.
 * @param monitor Target monitor index.
 * @param filter Filter selection.
 * @param strength Filter intensity.
 */
void gamescreen::gpuFilter(MONITER monitor, FILTER filter, float strength) {
    mMonitors[monitor].filter = filter;
    mMonitors[monitor].strength = strength;
}

/**
 * @brief Sets the selected game's GPU filter.
 * @param game Target game index.
 * @param filter Filter selection.
 * @param strength Filter intensity.
 */
void gamescreen::gpuFilter(GAMESCREEN game, FILTER filter, float strength) {
    mGames[game].gpuFilter = filter;
    mGames[game].gpuStrength = strength;
}

/**
 * @brief Sets the selected game's CPU filter.
 * @param game Target game index.
 * @param filter Filter selection.
 * @param strength Filter intensity.
 */
void gamescreen::cpuFilter(GAMESCREEN game, FILTER filter, float strength) {
    mGames[game].cpuFilter = filter;
    mGames[game].cpuStrength = strength;
}

/**
 * @brief Provides the empty text callback hook in this port.
 * @param monitor Unused monitor selection.
 * @param rCallback Unused callback.
 */
void gamescreen::entryTextCallback(MONITER monitor, const systemdrawscreen::TextCallback& rCallback) {}

/**
 * @brief Provides the empty text callback hook in this port.
 * @param monitor Unused monitor selection.
 * @param rCallback Unused callback.
 */
void gamescreen::cancelTextCallback(MONITER monitor, const systemdrawscreen::TextCallback& rCallback) {}

/** @brief Provides the empty m_HomeButtonMenuToDRC hook in this port. */
void gamescreen::m_HomeButtonMenuToDRC() {}

/** @brief Provides the empty acquiredForeground hook in this port. */
void gamescreen::acquiredForeground() {}

/** @brief Provides the empty releaseForeground hook in this port. */
void gamescreen::releaseForeground() {}

/** @brief Provides the empty buildDevice hook in this port. */
void gamescreen::buildDevice() {}

/** @brief Provides the empty buildRenderTargets hook in this port. */
void gamescreen::buildRenderTargets() {}

/**
 * @brief Allocates the requested byte buffer.
 * @param type Unused allocation category.
 * @param size Buffer size in bytes.
 * @param alignment Unused alignment request.
 * @return Allocated byte buffer.
 */
void* gamescreen::malloc(ALLOC_TYPE type, unsigned int size, unsigned int alignment) {
    return new unsigned char[size];
}

/** @brief Releases rendering buffers, textures, and vertex resources. */
void gamescreen::finalize() {
    auto* pAllocator = agl::utl::DynamicTextureAllocator::instance();
    pAllocator->free(mGames[0].pTexture);
    pAllocator->free(mGames[1].pTexture);
    mVertexAttribute.destroy();
    mVertices.freeBuffer();
    if (mpVertexBuffer) {
        delete mpVertexBuffer;
        mpVertexBuffer = nullptr;
    }
    if (mGames[0].pOwnedTexture) {
        delete mGames[0].pOwnedTexture;
        mGames[0].pOwnedTexture = nullptr;
    }
    mGames[0].image.deleteGPUMemBlock();
    if (mpPixels) {
        delete[] mpPixels;
        mpPixels = nullptr;
    }
}

/** @brief Finalizes rendering resources before destroying owned graphics objects. */
gamescreen::~gamescreen() {
    finalize();
}

class _SimpleTextureShader {
public:
    /** @brief Destroys the embedded graphics shader. */
    ~_SimpleTextureShader();
    unsigned char _00[0x10];
    nn::gfx::detail::ShaderImpl<nn::gfx::ApiVariationNvn8> mShader;
};

/** @brief Destroys the embedded graphics shader. */
_SimpleTextureShader::~_SimpleTextureShader() = default;

gamescreen m_gamescreen;

namespace {
sead::Heap* sScreenHeap;
_SimpleTextureShader sSimpleTextureShader;
gamescreen::Vtx sScreenQuad[6] = {
    {-0.75f, 1.0f, 0.0f, 0.0f}, {-0.75f, -1.0f, 0.0f, 1.0f},
    {0.75f, 1.0f, 1.0f, 0.0f}, {0.75f, 1.0f, 1.0f, 0.0f},
    {-0.75f, -1.0f, 0.0f, 1.0f}, {0.75f, -1.0f, 1.0f, 1.0f}
};
}

/** @brief Allocates the screen quad, texture buffers, and emulator shader. */
void gamescreen::init() {
    mVertices.allocBuffer(6, al::getCurrentHeap(), 8, static_cast<agl::MemoryAttribute>(0));
    agl::GPUMemAddr<Vtx> vertexAddress(mVertices, 0);
    for (int i = 0; i < 6; ++i) {
        auto* pMapped = static_cast<unsigned char*>(nvnMemoryPoolMap(mVertices.getMemoryPool()->getDriverPool()));
        Vtx* pVertices = reinterpret_cast<Vtx*>(pMapped + mVertices.getByteOffset());
        pVertices[i] = sScreenQuad[i];
    }
    mpVertexBuffer = new agl::VertexBuffer;
    mpVertexBuffer->setUpBuffer(agl::ConstGPUMemVoidAddr(mVertices, 0), 16, 96);
    mpVertexBuffer->setUpStream(0, static_cast<agl::VertexStreamFormat>(22), 0, false);
    mpVertexBuffer->setUpStream(1, static_cast<agl::VertexStreamFormat>(22), 8, false);
    mVertexAttribute.create(2, nullptr);
    mVertexAttribute.setVertexStream(0, mpVertexBuffer, 0);
    mVertexAttribute.setVertexStream(1, mpVertexBuffer, 1);
    mVertexAttribute.setUp();
    mpPixels = new unsigned char[0x100000];
    auto* pAllocator = agl::utl::DynamicTextureAllocator::instance();
    mGames[0].pTexture = pAllocator->alloc(al::GameFrameworkNx::getAglDrawContext(),
        sead::SafeString("gbuf_emuscreen"), static_cast<agl::TextureFormat>(32), 512, 512, 1,
        nullptr, agl::utl::DynamicTextureAllocator::cAllocateType_1, true, true);
    mGames[1].pTexture = pAllocator->alloc(al::GameFrameworkNx::getAglDrawContext(),
        sead::SafeString("gbuf_emuscreen"), static_cast<agl::TextureFormat>(32), 512, 512, 1,
        nullptr, agl::utl::DynamicTextureAllocator::cAllocateType_1, true, true);
    mGames[0].pOwnedTexture = new agl::TextureData;
    mGames[0].pOwnedTexture->initialize_(static_cast<agl::TextureType>(1),
        static_cast<agl::TextureFormat>(29), 512, 512, 1, 1,
        static_cast<agl::TextureAttribute>(1), static_cast<agl::MultiSampleType>(0), true);
    sead::Heap* pHeap = agl::detail::PrivateResource::instance()->getWorkHeap();
    unsigned int alignment = mGames[0].pOwnedTexture->getAlignment();
    unsigned int size = mGames[0].pOwnedTexture->getImageByteSize();
    auto* pImage = new (pHeap, 8) agl::GPUMemBlock<unsigned char>;
    pImage->allocBuffer_(size, pHeap, alignment, static_cast<agl::MemoryAttribute>(8));
    mGames[0].image = agl::GPUMemVoidAddr(*pImage, 0);
    mGames[0].pOwnedTexture->setImagePtr(mGames[0].image, 0);
    mpShader = al::ShaderHolder::instance()->getShaderProgram("alEmu");
    sScreenHeap = al::getCurrentHeap();
}

/** @brief Uploads filtered emulator pixels and draws the visible screen quad. */
void gamescreen::draw() {
    auto* pRenderBuffer = al::GameFrameworkNx::sInstance->getCurrentRenderBuffer();
    Emu* pEmulator = m_vessel.mpEmulator;
    if (pEmulator) {
        if (m_vessel.mState == 2 || mRefresh != 0) {
            pEmulator->MakeScreenTextue(reinterpret_cast<unsigned int*>(mpPixels));
        }
        pEmulator->GetScreenSize(&mWidth, &mHeight);
    } else {
        mHeight = 0;
        mWidth = 0;
    }
    m_ScreenFilterCpu.Process(reinterpret_cast<unsigned int*>(mpPixels),
        static_cast<unsigned int*>(mGames[0].image.getPtr()));
    mGames[0].pOwnedTexture->setImagePtr(mGames[0].image, 0);
    pRenderBuffer->bind(al::GameFrameworkNx::getAglDrawContext());
    sead::Viewport viewport(*pRenderBuffer);
    viewport.apply(al::GameFrameworkNx::getAglDrawContext(), *pRenderBuffer);
    pRenderBuffer->fastClear(al::GameFrameworkNx::getAglDrawContext(), 0, 1,
        sead::Color4f(0, 0, 0, 1), 0.0f, 0, sead::Viewport(*pRenderBuffer), true);
    if (mMonitors[1].enabled) {
        sead::Viewport gameViewport(0.0f, 0.0f, static_cast<float>(mWidth), static_cast<float>(mHeight));
        sead::TextWriter::setupGraphics(al::GameFrameworkNx::getAglDrawContext());
        sead::TextWriter textWriter(al::GameFrameworkNx::getAglDrawContext(), &gameViewport);
        sead::GraphicsContext context;
        context.setDepthEnable(false, false);
        context.setBlendEnable(false);
        context.setCullingMode(0);
        context.apply(al::GameFrameworkNx::getAglDrawContext());
        const agl::ShaderProgram* pProgram = mpShader->getVariation(0);
        pProgram->activate(al::GameFrameworkNx::getAglDrawContext(), true);
        agl::SamplerLocation baseLocation(sead::SafeString("cBase"));
        baseLocation.search(*pProgram);
        agl::SamplerLocation paintLocation(sead::SafeString("cPaint"));
        paintLocation.search(*pProgram);
        agl::TextureSampler sampler;
        sampler.applyTextureData(*mGames[0].pOwnedTexture);
        sampler.getTextureData().invalidateCPUCache();
        sampler.setFilterDirect(0, 0, 1);
        sampler.activate(al::GameFrameworkNx::getAglDrawContext(), baseLocation, -1, false);
        mVertexAttribute.activate(al::GameFrameworkNx::getAglDrawContext());
        nvnCommandBufferDrawArrays(al::GameFrameworkNx::getAglDrawContext()->getNvnCommandBuffer(),
            NVN_DRAW_PRIMITIVE_TRIANGLES, 0, 6);
    }
}
