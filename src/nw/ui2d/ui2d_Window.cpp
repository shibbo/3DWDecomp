#include <nn/ui2d/ui2d_Window.h>

#include <atomic>
#include <cstring>

#include <nn/font/font_GpuBuffer.h>
#include <nn/nn_SdkAssert.h>
#include <nn/gfx/gfx_CommandBuffer.h>
#include <nn/ui2d/ui2d_BuildArgSet.h>
#include <nn/ui2d/ui2d_BuildPaneTreeContext.h>
#include <nn/ui2d/ui2d_DrawInfo.h>
#include <nn/ui2d/ui2d_GraphicsResource.h>
#include <nn/ui2d/ui2d_Layout.h>
#include <nn/ui2d/ui2d_Material.h>
#include <nn/ui2d/ui2d_Util.h>

namespace nn::ui2d {
namespace {
/** @brief Number of characters compared when matching material names. */
const int MaterialNameStrMax = 28;

/** @brief Size of the vertex shader constants of one left-top emulation draw. */
const size_t UseLeftTopEmulationConstantBufferSize = 0x230;

/** @brief Vertex shader constants of a material, as far as windows use them. */
struct VertexShaderConstantBuffer {
    nn::util::MatrixT4x4fType projection;
    nn::util::MatrixT4x3fType modelView;
    unsigned char _70[0x130];
    float frameSize[4];
    float paneSize[2];
    unsigned char _1b8[8];
    unsigned char texCoordData[16];
    float vertexColors[4][4];
    unsigned char _210[0x1c];
    u32 frameTransform;
};
static_assert(sizeof(VertexShaderConstantBuffer) == 0x230, "VertexShaderConstantBuffer size");

/**
 * @brief Stores the matrices that draw a pane into its effect capture texture.
 * @param pConstantBuffer Constants to fill.
 * @param rProjectionMtx Projection matrix of the capture.
 * @param rRootMtx Model-view matrix of the capture root.
 */
void SetupCaptureMatrices(VertexShaderConstantBuffer* pConstantBuffer,
                          const nn::util::MatrixT4x4fType& rProjectionMtx,
                          const nn::util::MatrixT4x3fType& rRootMtx) {
    const float32x4_t projection0 = rProjectionMtx._m.val[0];
    const float32x4_t projection1 = rProjectionMtx._m.val[1];
    const float32x4_t projection2 = rProjectionMtx._m.val[2];
    const float32x4_t projection3 = rProjectionMtx._m.val[3];
    pConstantBuffer->projection._m.val[0] = projection0;
    pConstantBuffer->projection._m.val[1] = projection1;
    pConstantBuffer->projection._m.val[2] = projection2;
    pConstantBuffer->projection._m.val[3] = projection3;

    const float32x4_t modelView0 = rRootMtx._m.val[0];
    const float32x4_t modelView1 = rRootMtx._m.val[1];
    const float32x4_t modelView2 = rRootMtx._m.val[2];
    pConstantBuffer->modelView._m.val[0] = modelView0;
    pConstantBuffer->modelView._m.val[1] = modelView1;
    pConstantBuffer->modelView._m.val[2] = modelView2;
}

/** @brief Bit set in a frame transform when the window vertex colors do not apply to frames. */
const u32 FrameTransform_IgnoreVertexColor = 0x40000000;

/**
 * @param pMaterial Material to query.
 * @param rDrawInfo Drawing state.
 * @return The vertex shader constants of the material.
 */
VertexShaderConstantBuffer* GetVertexShaderConstantBuffer(const Material* pMaterial,
                                                          const DrawInfo& rDrawInfo) {
    return static_cast<VertexShaderConstantBuffer*>(
        pMaterial->GetConstantBufferForVertexShader(rDrawInfo));
}

/**
 * @param pConstantBuffer Constants handed to detail::CalculateQuad.
 * @return The same constants as the type the quad helpers take.
 */
Material::ConstantBufferForVertexShader* ToMaterialConstantBuffer(
    VertexShaderConstantBuffer* pConstantBuffer) {
    return reinterpret_cast<Material::ConstantBufferForVertexShader*>(pConstantBuffer);
}

/**
 * @brief Stores the pane and frame sizes used by shaders that spread vertex colors.
 * @param pConstantBuffer Constants to fill.
 * @param rPaneSize Size of the window pane.
 * @param rFrameSize Frame thickness of each edge.
 */
void SetupFrameSize(VertexShaderConstantBuffer* pConstantBuffer, const Size& rPaneSize,
                    const WindowFrameSize& rFrameSize) {
    pConstantBuffer->paneSize[0] = rPaneSize.width;
    pConstantBuffer->paneSize[1] = rPaneSize.height;
    pConstantBuffer->frameSize[0] = rFrameSize.left;
    pConstantBuffer->frameSize[1] = rFrameSize.right;
    pConstantBuffer->frameSize[2] = rFrameSize.top;
    pConstantBuffer->frameSize[3] = rFrameSize.bottom;
}

/**
 * @brief Stores the four content vertex colors as floats.
 * @param pConstantBuffer Constants to fill.
 * @param pColors Colors of the four content corners.
 */
void SetupVertexColors(VertexShaderConstantBuffer* pConstantBuffer,
                       const nn::util::Unorm8x4* pColors) {
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            pConstantBuffer->vertexColors[i][j] = pColors[i].v[j];
        }
    }
}

/**
 * @param rWindow Window that owns the material.
 * @param pMaterial Material about to be set up.
 * @return Whether vertex colors cannot change what the material draws.
 */
bool CanSkipVertexColor(const Window& rWindow, const Material* pMaterial) {
    if (!pMaterial->IsCombinerUserShaderCapable() && rWindow.IsContentVertexColorWhite() &&
        rWindow.GetGlobalAlpha() == 0xff) {
        return true;
    }

    return false;
}

/** @brief Position and size of one frame quad. */
struct FrameRect {
    float x;
    float y;
    float width;
    float height;
};

/**
 * @brief Compares two material names over at most MaterialNameStrMax characters.
 * @param pName1 First name.
 * @param pName2 Second name.
 * @return Whether the names are equal.
 */
bool EqualsMaterialName(const char* pName1, const char* pName2) {
    for (int i = 0; i < MaterialNameStrMax; ++i) {
        if (pName1[i] != pName2[i]) {
            return false;
        }

        if (pName1[i] == '\0') {
            return true;
        }
    }

    return true;
}
}  // namespace

namespace detail {
/**
 * @brief Builds the shader transform flags of a frame of a window with four frames.
 * @param frame Frame to draw.
 * @param flip Texture flip of the frame.
 * @param isVertexColorAll Whether the window vertex colors apply to the frame.
 * @return The transform flags.
 */
inline u32 DecideFrame4Transform(WindowFrame frame, TextureFlip flip, bool isVertexColorAll) {
    static const u32 frameFlag[4] = {0x813, 0x923, 0xa2b, 0xb17};
    static const u32 flipFlag[TextureFlip_MaxTextureFlip] = {0x0,     0x20000, 0x40000,
                                                             0x30000, 0x60000, 0x50000};

    const u32 flag = flipFlag[flip] | frameFlag[frame];
    return isVertexColorAll ? flag : flag | FrameTransform_IgnoreVertexColor;
}

/**
 * @brief Builds the shader transform flags of a frame of a window with eight frames.
 * @param frame Frame to draw.
 * @param flip Texture flip of the frame.
 * @param isVertexColorAll Whether the window vertex colors apply to the frame.
 * @return The transform flags.
 */
inline u32 DecideFrameTransform(WindowFrame frame, TextureFlip flip, bool isVertexColorAll) {
    static const u32 frameFlag[WindowFrame_MaxWindowFrame] = {0x3,   0x103, 0x203, 0x303,
                                                              0x42b, 0x523, 0x613, 0x717};
    static const u32 flipFlag[TextureFlip_MaxTextureFlip] = {0x0,     0x20000, 0x40000,
                                                             0x30000, 0x60000, 0x50000};

    const u32 flag = flipFlag[flip] | frameFlag[frame];
    return isVertexColorAll ? flag : flag | FrameTransform_IgnoreVertexColor;
}

/**
 * @brief Builds the shader transform flags of a frame of a horizontal window.
 * @param frame Frame to draw.
 * @param flip Texture flip of the frame.
 * @param isVertexColorAll Whether the window vertex colors apply to the frame.
 * @return The transform flags.
 */
inline u32 DecideHorizontalFrameTransform(WindowFrame frame, TextureFlip flip,
                                          bool isVertexColorAll) {
    static const u32 frameFlag[2] = {0x403, 0x503};
    static const u32 flipFlag[TextureFlip_MaxTextureFlip] = {0x0,     0x20000, 0x40000,
                                                             0x30000, 0x60000, 0x50000};

    const u32 flag = flipFlag[flip] | frameFlag[frame];
    return isVertexColorAll ? flag : flag | FrameTransform_IgnoreVertexColor;
}

/**
 * @brief Builds the shader transform flags of a frame of a horizontal window without content.
 * @param frame Frame to draw.
 * @param flip Texture flip of the frame.
 * @param isVertexColorAll Whether the window vertex colors apply to the frame.
 * @return The transform flags.
 */
inline u32 DecideHorizontalNocontextFrameTransform(WindowFrame frame, TextureFlip flip,
                                                   bool isVertexColorAll) {
    static const u32 frameFlag[WindowFrame_MaxWindowFrame] = {0xc13, 0x503};
    static const u32 flipFlag[TextureFlip_MaxTextureFlip] = {0x0,     0x20000, 0x40000,
                                                             0x30000, 0x60000, 0x50000};

    const u32 flag = flipFlag[flip] | frameFlag[frame];
    return isVertexColorAll ? flag : flag | FrameTransform_IgnoreVertexColor;
}
}  // namespace detail

/** @brief Deletes the frame material unless its owner manages it. */
Window::Frame::~Frame() {
    if (pMaterial != nullptr && !pMaterial->IsUserAllocated()) {
        Layout::DeleteObj(pMaterial);
    }

    pMaterial = nullptr;
}

/**
 * @brief Creates a window with one frame.
 * @param contentTexCount Number of textures of the content material.
 * @param frameTexCount Number of textures of the frame material.
 */
Window::Window(int contentTexCount, int frameTexCount)
    : m_WindowSize(),
      m_WindowKind(WindowKind_Around),
      m_WindowFlags(0),
      m_pUseLeftTopEmulationConstantBufferOffsets(nullptr),
      m_UseLeftTopEmulationConstantBufferCount(0) {
    Initialize();

    int frameTexCounts[1] = {frameTexCount};
    InitializeTexCount(contentTexCount, frameTexCounts, 1);
}

/** @brief Resets every member to the state of an empty window. */
void Window::Initialize() {
    m_WindowSize.inflation = ResWindowInflation();
    m_WindowSize.frameSize = ResWindowFrameSize();

    for (int i = 0; i < 4; ++i) {
        m_Content.texCoordArray.Initialize();
        m_Content.vtxColors[i] = nn::util::Unorm8x4{{0xff, 0xff, 0xff, 0xff}};
    }

    m_WindowKind = WindowKind_Around;
    m_FrameCount = 0;
    m_WindowFlags = 0;
    m_UseLeftTopEmulationConstantBufferCount = 0;
    m_pFrames = nullptr;
    m_pMaterial = nullptr;
    m_pUseLeftTopEmulationConstantBufferOffsets = nullptr;
}

/**
 * @brief Creates the content and frame materials with default shaders.
 * @param contentTexCount Number of textures of the content material.
 * @param pFrameTexCounts Number of textures of each frame material.
 * @param frameCount Number of frames.
 */
void Window::InitializeTexCount(int contentTexCount, int* pFrameTexCounts, int frameCount) {
    InitializeContent(contentTexCount);
    m_WindowSize.inflation = ResWindowInflation();
    m_WindowSize.frameSize = ResWindowFrameSize();

    m_pMaterial = Layout::NewObj<Material>();
    if (m_pMaterial != nullptr) {
        SetDefaultShaderId(m_pMaterial, contentTexCount);
        m_pMaterial->ReserveMem(contentTexCount, contentTexCount, contentTexCount, 0, false, 0,
                                false, 0, false, false, false, 0, 0);
    }

    InitializeFrame(frameCount);

    for (int i = 0; i < m_FrameCount; ++i) {
        m_pFrames[i].pMaterial = Layout::NewObj<Material>();
        if (m_pFrames[i].pMaterial != nullptr) {
            SetDefaultShaderId(m_pFrames[i].pMaterial, pFrameTexCounts[i]);
            m_pFrames[i].pMaterial->ReserveMem(pFrameTexCounts[i], pFrameTexCounts[i],
                                               pFrameTexCounts[i], 0, false, 0, false, 0, false,
                                               false, false, 0, 0);
        }
    }
}

/**
 * @brief Creates a window with one frame per corner.
 * @param contentTexCount Number of textures of the content material.
 * @param frameLtTexCount Number of textures of the left-top frame material.
 * @param frameRtTexCount Number of textures of the right-top frame material.
 * @param frameRbTexCount Number of textures of the right-bottom frame material.
 * @param frameLbTexCount Number of textures of the left-bottom frame material.
 */
Window::Window(int contentTexCount, int frameLtTexCount, int frameRtTexCount, int frameRbTexCount,
               int frameLbTexCount)
    : m_WindowSize(),
      m_WindowKind(WindowKind_Around),
      m_WindowFlags(0),
      m_pUseLeftTopEmulationConstantBufferOffsets(nullptr),
      m_UseLeftTopEmulationConstantBufferCount(0) {
    Initialize();

    int frameTexCounts[4] = {frameLtTexCount, frameRtTexCount, frameLbTexCount, frameRbTexCount};
    InitializeTexCount(contentTexCount, frameTexCounts, 4);
}

/**
 * @brief Creates a window with one frame per corner and per edge.
 * @param contentTexCount Number of textures of the content material.
 * @param frameLtTexCount Number of textures of the left-top frame material.
 * @param frameRtTexCount Number of textures of the right-top frame material.
 * @param frameRbTexCount Number of textures of the right-bottom frame material.
 * @param frameLbTexCount Number of textures of the left-bottom frame material.
 * @param frameLTexCount Number of textures of the left frame material.
 * @param frameTTexCount Number of textures of the top frame material.
 * @param frameRTexCount Number of textures of the right frame material.
 * @param frameBTexCount Number of textures of the bottom frame material.
 */
Window::Window(int contentTexCount, int frameLtTexCount, int frameRtTexCount, int frameRbTexCount,
               int frameLbTexCount, int frameLTexCount, int frameTTexCount, int frameRTexCount,
               int frameBTexCount)
    : m_WindowSize(),
      m_WindowKind(WindowKind_Around),
      m_WindowFlags(0),
      m_pUseLeftTopEmulationConstantBufferOffsets(nullptr),
      m_UseLeftTopEmulationConstantBufferCount(0) {
    Initialize();

    int frameTexCounts[8] = {frameLtTexCount, frameRtTexCount, frameLbTexCount, frameRbTexCount,
                             frameLTexCount,  frameRTexCount,  frameTTexCount,  frameBTexCount};
    InitializeTexCount(contentTexCount, frameTexCounts, 8);
}

/**
 * @brief Builds a window from its layout resource.
 * @param pResult Receives the constant buffer size the window needs, or nullptr.
 * @param pDevice Device that owns the material resources.
 * @param pBaseBlock Window resource of the layout.
 * @param pOverrideBlock Window resource that overrides the base one, or nullptr.
 * @param rArgs Layout construction arguments.
 */
Window::Window(BuildResultInformation* pResult, nn::gfx::Device* pDevice,
               const ResWindow* pBaseBlock, const ResWindow* pOverrideBlock,
               const BuildArgSet& rArgs)
    : Pane(pResult, pDevice, pBaseBlock->GetPane(), rArgs),
      m_WindowSize(),
      m_WindowKind(WindowKind_Around),
      m_WindowFlags(0),
      m_pUseLeftTopEmulationConstantBufferOffsets(nullptr),
      m_UseLeftTopEmulationConstantBufferCount(0) {
    Initialize();

    const ResWindow* pBlock = pBaseBlock;
    if (pOverrideBlock != nullptr) {
        pBlock = rArgs.overrideUsageFlag != 0 ? pBaseBlock : pOverrideBlock;
    }

    m_WindowKind = (pBlock->windowFlags >> 2) & 3;

    if ((pBlock->windowFlags & 2) != 0) {
        m_WindowFlags |= Flag_UseVertexColorAll;
    }

    if ((pBlock->windowFlags & 1) != 0) {
        m_WindowFlags |= Flag_UseOneMaterialForAll;
    }

    if ((pBlock->windowFlags & 0x10) != 0) {
        m_WindowFlags |= Flag_NotDrawContent;
    }

    const ResWindowContent* pResContent = pBlock->GetContent();
    const int texCoordCount = pResContent->texCoordCount < 3 ? pResContent->texCoordCount : 3;
    InitializeContent(texCoordCount);
    InitializeSize(pBlock);

    for (int i = 0; i < 4; ++i) {
        m_Content.vtxColors[i] =
            *reinterpret_cast<const nn::util::Unorm8x4*>(&pResContent->vtxCols[i]);
    }

    if (texCoordCount > 0 && m_Content.texCoordArray.GetCapacity() > 0) {
        m_Content.texCoordArray.Copy(pResContent + 1, texCoordCount);
    }

    const ResMaterial* pResMaterial =
        detail::GetResMaterial(rArgs.pCurrentBuildResSet, pBaseBlock->GetContent()->materialIdx);
    const ResMaterial* pOverrideResMaterial = nullptr;
    if (pOverrideBlock != nullptr) {
        pOverrideResMaterial = detail::GetResMaterial(rArgs.pOverrideBuildResSet,
                                                      pOverrideBlock->GetContent()->materialIdx);
    }

    m_pMaterial =
        Layout::NewObj<Material>(pResult, pDevice, pResMaterial, pOverrideResMaterial, rArgs);
    m_pMaterial->InitializeDynamicRenderingTexture(pResult, pDevice, rArgs);

    m_FrameCount = 0;
    m_pFrames = nullptr;

    if (pBlock->frameCount > 0) {
        InitializeFrame(pBlock->frameCount);

        const u32* pFrameOffsetTable = pBlock->GetFrameOffsetTable();
        const u32* pBaseFrameOffsetTable = pBaseBlock->GetFrameOffsetTable();
        const u32* pOverrideFrameOffsetTable = nullptr;
        if (pOverrideBlock != nullptr) {
            pOverrideFrameOffsetTable = pOverrideBlock->GetFrameOffsetTable();
        }

        for (int i = 0; i < m_FrameCount; ++i) {
            const ResWindowFrame* pResFrame = reinterpret_cast<const ResWindowFrame*>(
                reinterpret_cast<const u8*>(pBlock) + pFrameOffsetTable[i]);
            m_pFrames[i].textureFlip = pResFrame->textureFlip;

            const ResMaterial* pFrameResMaterial = nullptr;
            if (i < pBaseBlock->frameCount) {
                const ResWindowFrame* pBaseFrame = reinterpret_cast<const ResWindowFrame*>(
                    reinterpret_cast<const u8*>(pBaseBlock) + pBaseFrameOffsetTable[i]);
                pFrameResMaterial =
                    detail::GetResMaterial(rArgs.pCurrentBuildResSet, pBaseFrame->materialIdx);
            }

            const ResMaterial* pOverrideFrameResMaterial = nullptr;
            if (pOverrideBlock != nullptr) {
                const ResWindowFrame* pOverrideFrame = reinterpret_cast<const ResWindowFrame*>(
                    reinterpret_cast<const u8*>(pOverrideBlock) + pOverrideFrameOffsetTable[i]);
                pOverrideFrameResMaterial =
                    detail::GetResMaterial(rArgs.pOverrideBuildResSet, pOverrideFrame->materialIdx);
            }

            m_pFrames[i].pMaterial = Layout::NewObj<Material>(pResult, pDevice, pFrameResMaterial,
                                                              pOverrideFrameResMaterial, rArgs);
            m_pFrames[i].pMaterial->InitializeDynamicRenderingTexture(pResult, pDevice, rArgs);
        }

        if (pBlock->frameCount == 1) {
            InitializeUseLeftTopMaterialEmulation(pResult, pDevice);
        }
    }
}

/**
 * @brief Reserves the texture coordinates of the content area.
 * @param texCount Number of texture coordinate sets.
 */
void Window::InitializeContent(int texCount) {
    if (texCount > 0) {
        m_Content.texCoordArray.Reserve(texCount);
    }
}

/**
 * @brief Reads the content inflation and frame sizes from the resource.
 * @param pBlock Window resource.
 */
void Window::InitializeSize(const ResWindow* pBlock) {
    m_WindowSize.inflation.left = pBlock->inflation.left;
    m_WindowSize.inflation.right = pBlock->inflation.right;
    m_WindowSize.inflation.top = pBlock->inflation.top;
    m_WindowSize.inflation.bottom = pBlock->inflation.bottom;

    if (m_WindowSize.inflation.left != 0 || m_WindowSize.inflation.right != 0 ||
        m_WindowSize.inflation.top != 0 || m_WindowSize.inflation.bottom != 0) {
        m_WindowFlags |= Flag_ContentInflationEnabled;
    }

    switch (m_WindowKind) {
        case WindowKind_Horizontal: {
            m_WindowSize.frameSize.left = pBlock->frameSize.left;
            m_WindowSize.frameSize.right = pBlock->frameSize.right;
            m_WindowSize.frameSize.top = pBlock->frameSize.top;
            m_WindowSize.frameSize.bottom = pBlock->frameSize.bottom;

            Size size = GetSize();
            size.height = pBlock->frameSize.top;
            SetSize(size);
            break;
        }
        case WindowKind_HorizontalNoContent: {
            m_WindowSize.frameSize.left =
                static_cast<int>(GetSize().width) - pBlock->frameSize.right;
            m_WindowSize.frameSize.right = pBlock->frameSize.right;
            m_WindowSize.frameSize.top = pBlock->frameSize.top;
            m_WindowSize.frameSize.bottom = pBlock->frameSize.bottom;

            Size size = GetSize();
            size.height = pBlock->frameSize.top;
            SetSize(size);
            break;
        }
        default:
            m_WindowSize.frameSize.left = pBlock->frameSize.left;
            m_WindowSize.frameSize.right = pBlock->frameSize.right;
            m_WindowSize.frameSize.top = pBlock->frameSize.top;
            m_WindowSize.frameSize.bottom = pBlock->frameSize.bottom;
            break;
    }
}

/**
 * @brief Allocates the frames.
 * @param frameCount Number of frames.
 */
void Window::InitializeFrame(int frameCount) {
    m_FrameCount = 0;
    m_pFrames = Layout::NewArray<Frame>(frameCount);
    if (m_pFrames != nullptr) {
        m_FrameCount = frameCount;
    }
}

/**
 * @brief Prepares the constant buffers used to draw the single frame material on every corner.
 * @param pResult Receives the constant buffer size the window needs, or nullptr.
 * @param pDevice Device used to query the buffer alignment.
 */
void Window::InitializeUseLeftTopMaterialEmulation(BuildResultInformation* pResult,
                                                   nn::gfx::Device* pDevice) {
    int count = GetUseLeftTopMaterialEmulationCount();
    m_pUseLeftTopEmulationConstantBufferOffsets =
        static_cast<u32*>(Layout::AllocateMemory(sizeof(u32) * count));
    m_UseLeftTopEmulationConstantBufferCount = count;

    if (pResult != nullptr) {
        pResult->requiredUi2dConstantBufferSize +=
            GetAlignedBufferSize(pDevice, nn::gfx::GpuAccess_ConstantBuffer,
                                 UseLeftTopEmulationConstantBufferSize) *
            m_UseLeftTopEmulationConstantBufferCount;
    }
}

/**
 * @brief Copies the window state and duplicates its materials.
 * @param rWindow Source window.
 * @param pDevice Device that owns the copied material resources.
 * @param pLayout Layout that owns the copy.
 * @param pContext Pane tree build state shared by the copy.
 */
void Window::CopyImpl(const Window& rWindow, nn::gfx::Device* pDevice, const Layout* pLayout,
                      detail::BuildPaneTreeContext* pContext) {
    m_WindowSize = rWindow.m_WindowSize;
    m_WindowKind = rWindow.m_WindowKind;
    m_FrameCount = rWindow.m_FrameCount;
    m_WindowFlags = rWindow.m_WindowFlags;
    m_UseLeftTopEmulationConstantBufferCount = 0;

    for (int i = 0; i < 4; ++i) {
        m_Content.texCoordArray.Initialize();
        m_Content.vtxColors[i] = rWindow.m_Content.vtxColors[i];
    }

    const int texCoordCount = rWindow.m_Content.texCoordArray.GetSize();
    if (texCoordCount > 0) {
        m_Content.texCoordArray.Reserve(texCoordCount);
        m_Content.texCoordArray.SetSize(texCoordCount);

        for (int i = 0; i < texCoordCount; ++i) {
            m_Content.texCoordArray.SetCoord(i, rWindow.m_Content.texCoordArray.GetArray()[i]);
        }
    }

    MaterialCopyContext context = {pDevice, nullptr, pLayout, pContext};

    Material* pSrcMaterial = rWindow.m_pMaterial;
    if (pSrcMaterial != nullptr && !pSrcMaterial->IsUserAllocated()) {
        m_pMaterial = Layout::NewObj<Material>(*pSrcMaterial, context);
    } else {
        m_pMaterial = pSrcMaterial;
    }

    InitializeFrame(m_FrameCount);

    for (int i = 0; i < m_FrameCount; ++i) {
        m_pFrames[i].textureFlip = rWindow.m_pFrames[i].textureFlip;
        m_pFrames[i].pMaterial = Layout::NewObj<Material>(*rWindow.m_pFrames[i].pMaterial, context);
    }

    if (rWindow.m_pUseLeftTopEmulationConstantBufferOffsets != nullptr) {
        InitializeUseLeftTopMaterialEmulation(nullptr, pDevice);
    } else {
        m_pUseLeftTopEmulationConstantBufferOffsets = nullptr;
    }
}

/**
 * @brief Creates a copy of a window.
 * @param rOther Source window.
 * @param pDevice Device that owns the copied material resources.
 * @param pLayout Layout that owns the copy.
 */
Window::Window(const Window& rOther, nn::gfx::Device* pDevice, Layout* pLayout)
    : Pane(rOther, pDevice, pLayout) {
    void* pContextMemory =
        __builtin_alloca(detail::BuildPaneTreeContext::CalculateContextRequireMemorySize());
    detail::BuildPaneTreeContext context(
        pContextMemory, detail::BuildPaneTreeContext::CalculateContextRequireMemorySize());
    context.Initialize();
    context.PushCache(pLayout, nullptr);

    CopyImpl(rOther, pDevice, pLayout, &context);

    pLayout->AggregateDynamicTextureList(context.GetCurrentTextureShareInfo());
    context.PopCache();
    context.InitializeCaptureTexturesAfterPaneTreeBuilt(pDevice);
    context.Finalize();
}

/** @brief Destroys the window; resources are released by Finalize. */
Window::~Window() {
    NN_SDK_ASSERT(m_pMaterial == nullptr);
}

/**
 * @brief Releases the materials and buffers of the window.
 * @param pDevice Device that owns the material resources.
 */
void Window::Finalize(nn::gfx::Device* pDevice) {
    Pane::Finalize(pDevice);

    if (m_pUseLeftTopEmulationConstantBufferOffsets != nullptr) {
        Layout::FreeMemory(m_pUseLeftTopEmulationConstantBufferOffsets);
    }

    for (int i = 0; i < m_FrameCount; ++i) {
        m_pFrames[i].pMaterial->Finalize(pDevice);
    }

    Layout::DeleteArray(m_pFrames, m_FrameCount);

    if (m_pMaterial != nullptr && !m_pMaterial->IsUserAllocated()) {
        m_pMaterial->Finalize(pDevice);
        Layout::DeleteObj(m_pMaterial);
    }

    m_pMaterial = nullptr;
    m_Content.texCoordArray.Free();
}

/**
 * @param index Content corner to read.
 * @return The vertex color of that corner.
 */
nn::util::Unorm8x4 Window::GetVertexColor(int index) const {
    return m_Content.vtxColors[index];
}

/**
 * @param index Content corner to change.
 * @param rColor New vertex color of that corner.
 */
void Window::SetVertexColor(int index, const nn::util::Unorm8x4& rColor) {
    m_Content.vtxColors[index] = rColor;
}

/**
 * @param index Channel index across the four content corner colors.
 * @return The value of that channel.
 */
u8 Window::GetVertexColorElement(int index) const {
    return reinterpret_cast<const u8*>(&m_Content.vtxColors[index / 4])[index % 4];
}

/**
 * @param index Channel index across the four content corner colors.
 * @param value New value of that channel.
 */
void Window::SetVertexColorElement(int index, u8 value) {
    reinterpret_cast<u8*>(&m_Content.vtxColors[index / 4])[index % 4] = value;
}

/** @return The number of materials: the content material and one per frame. */
u32 Window::GetMaterialCount() const {
    return static_cast<u8>(m_FrameCount) + 1;
}

/**
 * @param idx 0 for the content material, otherwise 1 + the frame index.
 * @return The selected material, or nullptr when @p idx is out of range.
 */
Material* Window::GetMaterial(int idx) const {
    static_cast<void>(GetMaterialCount());

    if (idx == 0) {
        return GetContentMaterial();
    }

    if (idx <= m_FrameCount) {
        return GetFrameMaterial(static_cast<WindowFrame>(idx - 1));
    }

    return nullptr;
}

/**
 * @param pName Material name to look for.
 * @param isRecursive Whether child panes are searched too.
 * @return The material with that name, or nullptr.
 */
Material* Window::FindMaterialByName(const char* pName, bool isRecursive) {
    if (m_pMaterial != nullptr && EqualsMaterialName(m_pMaterial->GetName(), pName)) {
        return m_pMaterial;
    }

    for (int i = 0; i < m_FrameCount; ++i) {
        if (EqualsMaterialName(m_pFrames[i].pMaterial->GetName(), pName)) {
            return m_pFrames[i].pMaterial;
        }
    }

    if (isRecursive) {
        for (nn::util::IntrusiveListNode* pNode = m_Children.GetNext(); pNode != &m_Children;
             pNode = pNode->GetNext()) {
            Material* pMaterial = FromLink(pNode)->FindMaterialByName(pName, true);
            if (pMaterial != nullptr) {
                return pMaterial;
            }
        }
    }

    return nullptr;
}

/**
 * @param pName Material name to look for.
 * @param isRecursive Whether child panes are searched too.
 * @return The material with that name, or nullptr.
 */
const Material* Window::FindMaterialByName(const char* pName, bool isRecursive) const {
    return const_cast<Window*>(this)->FindMaterialByName(pName, isRecursive);
}

/**
 * @brief Replaces a frame material, deleting the old one unless its owner manages it.
 * @param frameIdx Frame to change.
 * @param pMaterial New material.
 */
void Window::SetFrameMaterial(WindowFrame frameIdx, Material* pMaterial) {
    if (m_pFrames[frameIdx].pMaterial == pMaterial) {
        return;
    }

    if (m_pFrames[frameIdx].pMaterial != nullptr &&
        !m_pFrames[frameIdx].pMaterial->IsUserAllocated()) {
        Layout::DeleteObj(m_pFrames[frameIdx].pMaterial);
    }

    m_pFrames[frameIdx].pMaterial = pMaterial;
}

/**
 * @brief Replaces the content material, deleting the old one unless its owner manages it.
 * @param pMaterial New material.
 */
void Window::SetContentMaterial(Material* pMaterial) {
    if (m_pMaterial == pMaterial) {
        return;
    }

    if (m_pMaterial != nullptr && !m_pMaterial->IsUserAllocated()) {
        Layout::DeleteObj(m_pMaterial);
    }

    m_pMaterial = pMaterial;
}

/**
 * @brief Updates the matrices and the constant buffers of every window material.
 * @param rDrawInfo Drawing state.
 * @param rContext Calculation state of the pane tree.
 * @param isDirtyParentMtx Whether the parent matrix changed.
 */
void Window::Calculate(DrawInfo& rDrawInfo, CalculateContext& rContext, bool isDirtyParentMtx) {
    Pane::Calculate(rDrawInfo, rContext, isDirtyParentMtx);
    LoadMtx(rDrawInfo);

    if (CheckInvisibleAndUpdateConstantBufferReady()) {
        return;
    }

    m_pMaterial->AllocateConstantBuffer(rDrawInfo);
    m_pMaterial->SetupBlendState(&rDrawInfo);

    for (int i = 0; i < m_FrameCount; ++i) {
        m_pFrames[i].pMaterial->AllocateConstantBuffer(rDrawInfo);
        m_pFrames[i].pMaterial->SetupBlendState(&rDrawInfo);
    }

    if (m_pMaterial->GetConstantBufferForVertexShader(rDrawInfo) != nullptr &&
        m_pMaterial->GetConstantBufferForPixelShader(rDrawInfo) != nullptr) {
        switch (m_WindowKind) {
            case WindowKind_Around:
                CalculateAroundFrameWindow(rDrawInfo);
                break;
            case WindowKind_Horizontal:
                CalculateHorizontalFrameWindow(rDrawInfo);
                break;
            case WindowKind_HorizontalNoContent:
                CalculateHorizontalFrameNocontentWindow(rDrawInfo);
                break;
            default:
                break;
        }

        if (IsPaneEffectEnabled()) {
            UpdateMaterialConstantBufferForEffectCapture(rDrawInfo);

            nn::util::MatrixT4x4fType projectionMtx;
            CalculateCaptureProjectionMatrix(projectionMtx);
            nn::util::MatrixT4x3fType rootMtx;
            CalculateCaptureRootMatrix(rootMtx, rDrawInfo);

            for (int i = 0; i < m_FrameCount; ++i) {
                VertexShaderConstantBuffer* pConstantBuffer =
                    static_cast<VertexShaderConstantBuffer*>(
                        m_pFrames[i].pMaterial->GetConstantBufferForVertexShader(rDrawInfo));
                SetupCaptureMatrices(pConstantBuffer, projectionMtx, rootMtx);
            }

            for (u32 i = 0; i < m_UseLeftTopEmulationConstantBufferCount; ++i) {
                VertexShaderConstantBuffer* pConstantBuffer =
                    reinterpret_cast<VertexShaderConstantBuffer*>(
                        static_cast<u8*>(rDrawInfo.GetUi2dConstantBuffer()->GetMappedPointer()) +
                        m_pUseLeftTopEmulationConstantBufferOffsets[i]);
                SetupCaptureMatrices(pConstantBuffer, projectionMtx, rootMtx);
            }
        }

        DrawInfo::PostCalculateCallback pCallback = rDrawInfo.GetPostCalculateCallback();
        if (pCallback != nullptr) {
            pCallback(rDrawInfo, this, rDrawInfo.GetPostCalculateCallbackUserData());
        }
    } else if (rDrawInfo.GetUi2dConstantBuffer()->IsUnallocated() && m_FrameCount == 1) {
        int count;
        switch (m_WindowKind) {
            case WindowKind_Around:
                count = 3;
                break;
            case WindowKind_Horizontal:
            case WindowKind_HorizontalNoContent:
                count = 1;
                break;
            default:
                return;
        }

        const size_t alignment = rDrawInfo.GetGraphicsResource()->m_ConstantBufferAlignment;
        const size_t size = (UseLeftTopEmulationConstantBufferSize + alignment - 1) & -alignment;

        for (int i = 0; i < count; ++i) {
            rDrawInfo.GetUi2dConstantBuffer()->AllocateWithoutAlignment(size);
        }
    }
}

/**
 * @brief Calculates a window whose frames surround the content.
 * @param rDrawInfo Drawing state.
 */
void Window::CalculateAroundFrameWindow(DrawInfo& rDrawInfo) {
    nn::util::Float2 basePos;
    const WindowFrameSize frameSize = {static_cast<float>(m_WindowSize.frameSize.left),
                                       static_cast<float>(m_WindowSize.frameSize.right),
                                       static_cast<float>(m_WindowSize.frameSize.top),
                                       static_cast<float>(m_WindowSize.frameSize.bottom)};
    basePos = GetVertexPos();

    switch (m_FrameCount) {
        case 1:
            CalculateFrame(rDrawInfo, basePos, m_pFrames[0], frameSize, GetGlobalAlpha());
            break;
        case 4:
            CalculateFrame4(rDrawInfo, basePos, m_pFrames, frameSize, GetGlobalAlpha());
            break;
        case 8:
            CalculateFrame8(rDrawInfo, basePos, m_pFrames, frameSize, GetGlobalAlpha());
            break;
        default:
            break;
    }

    if ((m_WindowFlags & Flag_NotDrawContent) != 0) {
        return;
    }

    u32 vertexColorFlags = 0;
    if (CanSkipVertexColor(*this, m_pMaterial)) {
        m_pMaterial->SetupGraphics(rDrawInfo, 0xff, ShaderVariation_WithoutVertexColor, true,
                                   GetGlobalMatrix(), &GetSize(), GetExtUserDataArray(),
                                   GetExtUserDataCount());
    } else {
        m_pMaterial->SetupGraphics(rDrawInfo, GetGlobalAlpha(), ShaderVariation_Standard, true,
                                   GetGlobalMatrix(), &GetSize(), GetExtUserDataArray(),
                                   GetExtUserDataCount());

        if ((m_WindowFlags & Flag_UseVertexColorAll) != 0) {
            SetupFrameSize(GetVertexShaderConstantBuffer(m_pMaterial, rDrawInfo), GetSize(),
                           frameSize);
            SetupVertexColors(GetVertexShaderConstantBuffer(m_pMaterial, rDrawInfo),
                              m_Content.vtxColors);
            vertexColorFlags = 0xd00;
        } else {
            SetupVertexColors(GetVertexShaderConstantBuffer(m_pMaterial, rDrawInfo),
                              m_Content.vtxColors);
        }
    }

    GetVertexShaderConstantBuffer(m_pMaterial, rDrawInfo)->frameTransform = vertexColorFlags;
    CalculateContent(rDrawInfo, basePos, frameSize, GetGlobalAlpha());
}

/**
 * @brief Calculates a window with a frame on the left and on the right of the content.
 * @param rDrawInfo Drawing state.
 */
void Window::CalculateHorizontalFrameWindow(DrawInfo& rDrawInfo) {
    const WindowFrameSize frameSize = {static_cast<float>(m_WindowSize.frameSize.left),
                                       static_cast<float>(m_WindowSize.frameSize.right), 0.0f,
                                       0.0f};

    Size size = GetSize();
    size.height = m_WindowSize.frameSize.top;
    SetSize(size);

    const nn::util::Float2 basePos = GetVertexPos();

    switch (m_FrameCount) {
        case 1:
            CalculateHorizontalFrame(rDrawInfo, basePos, m_pFrames[0], frameSize, GetGlobalAlpha());
            break;
        case 2:
            CalculateHorizontalFrame2(rDrawInfo, basePos, m_pFrames, frameSize, GetGlobalAlpha());
            break;
        default:
            break;
    }

    if ((m_WindowFlags & Flag_NotDrawContent) != 0) {
        return;
    }

    const bool isVertexColorNeeded = m_pMaterial->IsCombinerUserShaderCapable() ||
                                     !IsContentVertexColorWhite() || GetGlobalAlpha() != 0xff;
    const ShaderVariation variation =
        isVertexColorNeeded ? ShaderVariation_Standard : ShaderVariation_WithoutVertexColor;
    m_pMaterial->SetupGraphics(rDrawInfo, GetGlobalAlpha(), variation, false, GetGlobalMatrix(),
                               &GetSize(), GetExtUserDataArray(), GetExtUserDataCount());

    if (variation == ShaderVariation_Standard) {
        SetupFrameSize(GetVertexShaderConstantBuffer(m_pMaterial, rDrawInfo), GetSize(), frameSize);
        SetupVertexColors(GetVertexShaderConstantBuffer(m_pMaterial, rDrawInfo),
                          m_Content.vtxColors);
    }

    const u32 vertexColorFlags = (m_WindowFlags & Flag_UseVertexColorAll) != 0 ? 0xd00 : 0;
    GetVertexShaderConstantBuffer(m_pMaterial, rDrawInfo)->frameTransform = vertexColorFlags;
    CalculateContent(rDrawInfo, basePos, frameSize, GetGlobalAlpha());
}

/**
 * @brief Calculates a horizontal window whose frames cover the whole pane.
 * @param rDrawInfo Drawing state.
 */
void Window::CalculateHorizontalFrameNocontentWindow(DrawInfo& rDrawInfo) {
    WindowFrameSize frameSize;
    frameSize.right = m_WindowSize.frameSize.right;
    frameSize.left = GetSize().width - frameSize.right;
    frameSize.top = 0.0f;
    frameSize.bottom = 0.0f;

    Size size = GetSize();
    size.height = m_WindowSize.frameSize.top;
    SetSize(size);

    const nn::util::Float2 basePos = GetVertexPos();

    switch (m_FrameCount) {
        case 1:
            CalculateHorizontalNocontentFrame(rDrawInfo, basePos, m_pFrames[0], frameSize,
                                              GetGlobalAlpha());
            break;
        case 2:
            CalculateHorizontalNocontentFrame2(rDrawInfo, basePos, m_pFrames, frameSize,
                                               GetGlobalAlpha());
            break;
        default:
            break;
    }
}

/**
 * @brief Sets the blend state used when the window is the source of a pane effect.
 * @param rCommands Command buffer to record into.
 */
void Window::SetupPaneEffectSourceImageRenderState(nn::gfx::CommandBuffer& rCommands) const {
    m_pMaterial->SetCommandBufferOnlyBlend(rCommands);
}

/**
 * @brief Draws every part of the window with the shader of the first frame material.
 * @param rDrawInfo Drawing state.
 * @param rCommands Command buffer to record into.
 */
void Window::DrawSharedMaterialImpl(DrawInfo& rDrawInfo, nn::gfx::CommandBuffer& rCommands) {
    Material* pMaterial = m_pFrames[0].pMaterial;

    if (pMaterial->GetTexMapCount() > 0) {
        if (rDrawInfo.RecordCurrentShader(pMaterial->GetShaderInfo(),
                                          pMaterial->GetShaderVariation())) {
            pMaterial->SetShader(rCommands);
            rDrawInfo.SetupProgram(&rCommands);
        }

        pMaterial->ApplyPixelShaderConstantBuffer(rCommands, rDrawInfo);
        pMaterial->SetCommandBufferOnlyBlend(rCommands);

        if (IsPaneEffectEnabled()) {
            UpdateRenderStateForPaneEffectCapture(rCommands, rDrawInfo);
        }

        if (m_UseLeftTopEmulationConstantBufferCount != 0) {
            pMaterial->SetupSubmaterialOf_Texture(rDrawInfo, rCommands);
            pMaterial->ApplyVertexShaderConstantBuffer(rCommands, rDrawInfo);
            pMaterial->ApplyGeometryShaderConstantBuffer(rCommands, rDrawInfo);
            detail::DrawQuad(rCommands, rDrawInfo);

            const GraphicsResource* pGraphicsResource = rDrawInfo.GetGraphicsResource();
            for (u32 i = 0; i < m_UseLeftTopEmulationConstantBufferCount; ++i) {
                nn::gfx::GpuAddress gpuAddress;
                gpuAddress = rDrawInfo.GetUi2dConstantBuffer()->GetGpuAddress();
                gpuAddress.Offset(m_pUseLeftTopEmulationConstantBufferOffsets[i]);

                const int slot = pMaterial->GetShaderInfo()
                                     ->m_pVertexShaderSlots[pMaterial->GetShaderVariation()];
                rCommands.SetConstantBuffer(slot, nn::gfx::ShaderStage_Vertex, gpuAddress,
                                            UseLeftTopEmulationConstantBufferSize);
                rCommands.DrawIndexed(nn::gfx::PrimitiveTopology_TriangleList,
                                      nn::gfx::IndexFormat_Uint16,
                                      pGraphicsResource->m_IndexBufferGpuAddress, 6, 0);
            }
        } else {
            for (int i = 0; i < m_FrameCount; ++i) {
                const Frame& rFrame = m_pFrames[i];
                if (rFrame.pMaterial->GetTexMapCount() > 0) {
                    rFrame.pMaterial->SetupSubmaterialOf_Texture(rDrawInfo, rCommands);
                    rFrame.pMaterial->ApplyVertexShaderConstantBuffer(rCommands, rDrawInfo);
                    rFrame.pMaterial->ApplyGeometryShaderConstantBuffer(rCommands, rDrawInfo);
                    detail::DrawQuad(rCommands, rDrawInfo);
                }
            }
        }
    }

    if ((m_WindowFlags & Flag_NotDrawContent) == 0) {
        if (m_WindowKind == WindowKind_Around) {
            m_pMaterial->SetupSubmaterialOf_Texture(rDrawInfo, rCommands);
            detail::SetupMaterialRenderState(rCommands, rDrawInfo, *m_pMaterial);

            if (IsPaneEffectEnabled()) {
                UpdateRenderStateForPaneEffectCapture(rCommands, rDrawInfo);
            }

            detail::DrawQuad(rCommands, rDrawInfo);
        } else if (m_WindowKind == WindowKind_Horizontal) {
            m_pMaterial->SetupSubmaterialOf_Texture(rDrawInfo, rCommands);
            m_pMaterial->ApplyVertexShaderConstantBuffer(rCommands, rDrawInfo);
            m_pMaterial->ApplyGeometryShaderConstantBuffer(rCommands, rDrawInfo);
            detail::DrawQuad(rCommands, rDrawInfo);
        }
    }
}

/**
 * @brief Draws every frame and the content with their own materials.
 * @param rDrawInfo Drawing state.
 * @param rCommands Command buffer to record into.
 */
void Window::DrawNormalImpl(DrawInfo& rDrawInfo, nn::gfx::CommandBuffer& rCommands) {
    for (int i = 0; i < m_FrameCount; ++i) {
        const Frame& rFrame = m_pFrames[i];
        Material* pMaterial = rFrame.pMaterial;

        if (pMaterial->GetTexMapCount() > 0) {
            pMaterial->SetupSubmaterialOf_Texture(rDrawInfo, rCommands);

            if (rDrawInfo.RecordCurrentShader(pMaterial->GetShaderInfo(),
                                              pMaterial->GetShaderVariation())) {
                pMaterial->SetShader(rCommands);
                rDrawInfo.SetupProgram(&rCommands);
            }

            rFrame.pMaterial->SetCommandBuffer(rCommands, rDrawInfo);

            if (IsPaneEffectEnabled()) {
                GraphicsResource* pGraphicsResource =
                    const_cast<GraphicsResource*>(rDrawInfo.GetGraphicsResource());
                rCommands.SetBlendState(
                    pGraphicsResource->GetPresetBlendState(PresetBlendStateId_OpaqueOrAlphaTest));
            }

            detail::DrawQuad(rCommands, rDrawInfo);
        }
    }

    if ((m_WindowFlags & Flag_NotDrawContent) == 0) {
        m_pMaterial->SetupSubmaterialOf_Texture(rDrawInfo, rCommands);
        detail::SetupMaterialRenderState(rCommands, rDrawInfo, *m_pMaterial);

        if (IsPaneEffectEnabled()) {
            UpdateRenderStateForPaneEffectCapture(rCommands, rDrawInfo);
        }

        detail::DrawQuad(rCommands, rDrawInfo);
    }
}

/**
 * @brief Draws the window.
 * @param rDrawInfo Drawing state.
 * @param rCommands Command buffer to record into.
 */
void Window::DrawSelf(DrawInfo& rDrawInfo, nn::gfx::CommandBuffer& rCommands) {
    if (rDrawInfo.GetUi2dConstantBuffer()->IsUnallocated()) {
        return;
    }

    if ((m_WindowFlags & Flag_UseOneMaterialForAll) != 0 ||
        m_UseLeftTopEmulationConstantBufferCount != 0) {
        DrawSharedMaterialImpl(rDrawInfo, rCommands);
    } else {
        DrawNormalImpl(rDrawInfo, rCommands);
    }
}

/**
 * @brief Checks that a copied window matches its source.
 * @param rTarget Source window.
 * @return Whether every copied member is equal.
 */
bool Window::CompareCopiedInstanceTest(const Window& rTarget) const {
    if (std::memcmp(&m_WindowSize, &rTarget.m_WindowSize, sizeof(m_WindowSize)) != 0) {
        return false;
    }

    if (std::memcmp(m_Content.vtxColors, rTarget.m_Content.vtxColors,
                    sizeof(m_Content.vtxColors)) != 0) {
        return false;
    }

    if (!m_Content.texCoordArray.CompareCopiedInstanceTest(rTarget.m_Content.texCoordArray)) {
        return false;
    }

    if (m_WindowKind != rTarget.m_WindowKind) {
        return false;
    }

    if (m_FrameCount != rTarget.m_FrameCount) {
        return false;
    }

    if (m_WindowFlags != rTarget.m_WindowFlags) {
        return false;
    }

    if (m_UseLeftTopEmulationConstantBufferCount !=
        rTarget.m_UseLeftTopEmulationConstantBufferCount) {
        return false;
    }

    for (int i = 0; i < m_FrameCount; ++i) {
        if (m_pFrames[i].textureFlip != rTarget.m_pFrames[i].textureFlip) {
            return false;
        }

        if (rTarget.m_pFrames[i].pMaterial != nullptr) {
            if (m_pFrames[i].pMaterial == nullptr) {
                return false;
            }

            if (!m_pFrames[i].pMaterial->CompareCopiedInstanceTest(
                    *rTarget.m_pFrames[i].pMaterial)) {
                return false;
            }
        }
    }

    if (rTarget.m_pMaterial != nullptr) {
        if (m_pMaterial == nullptr) {
            return false;
        }

        if (!m_pMaterial->CompareCopiedInstanceTest(*rTarget.m_pMaterial)) {
            return false;
        }
    }

    if (m_pUseLeftTopEmulationConstantBufferOffsets != nullptr) {
        if (rTarget.m_pUseLeftTopEmulationConstantBufferOffsets == nullptr) {
            return false;
        }
    } else if (rTarget.m_pUseLeftTopEmulationConstantBufferOffsets != nullptr) {
        return false;
    }

    return true;
}
/**
 * @brief Calculates the single frame of a window and emulates it on the other three corners.
 * @param rDrawInfo Drawing state.
 * @param rBasePos Left-top position of the pane.
 * @param rFrame Frame to calculate.
 * @param rFrameSize Frame thickness of each edge.
 * @param alpha Alpha of the pane.
 */
void Window::CalculateFrame(DrawInfo& rDrawInfo, const nn::util::Float2& rBasePos,
                            const Frame& rFrame, const WindowFrameSize& rFrameSize, u8 alpha) {
    Material* pMaterial = rFrame.pMaterial;
    if (pMaterial->GetTexMapCount() <= 0) {
        return;
    }

    if (pMaterial->IsCombinerUserShaderCapable() || !IsContentVertexColorWhite() ||
        GetGlobalAlpha() != 0xff) {
        pMaterial->SetupGraphics(rDrawInfo, alpha, ShaderVariation_Standard, false,
                                 GetGlobalMatrix(), &GetSize(), GetExtUserDataArray(),
                                 GetExtUserDataCount());

        if ((m_WindowFlags & Flag_UseVertexColorAll) != 0) {
            SetupFrameSize(GetVertexShaderConstantBuffer(rFrame.pMaterial, rDrawInfo), GetSize(),
                           rFrameSize);
            SetupVertexColors(GetVertexShaderConstantBuffer(rFrame.pMaterial, rDrawInfo),
                              m_Content.vtxColors);
        }
    } else {
        pMaterial->SetupGraphics(rDrawInfo, alpha, ShaderVariation_WithoutVertexColor, false,
                                 GetGlobalMatrix(), &GetSize(), GetExtUserDataArray(),
                                 GetExtUserDataCount());

        if ((m_WindowFlags & Flag_UseVertexColorAll) != 0) {
            SetupFrameSize(GetVertexShaderConstantBuffer(rFrame.pMaterial, rDrawInfo), GetSize(),
                           rFrameSize);
        }
    }

    {
        const nn::util::Float2 pos = rBasePos;
        const Size size = {GetSize().width - rFrameSize.right, rFrameSize.top};
        const u32 transform = detail::DecideFrame4Transform(
            WindowFrame_LeftTop, TextureFlip_None, (m_WindowFlags & Flag_UseVertexColorAll) != 0);
        GetVertexShaderConstantBuffer(rFrame.pMaterial, rDrawInfo)->frameTransform = transform;
        detail::CalculateQuad(
            rDrawInfo,
            ToMaterialConstantBuffer(GetVertexShaderConstantBuffer(rFrame.pMaterial, rDrawInfo)),
            pos, size);
    }

    const size_t alignment = rDrawInfo.GetGraphicsResource()->m_ConstantBufferAlignment;
    const size_t alignedSize = (UseLeftTopEmulationConstantBufferSize + alignment - 1) & -alignment;

    m_pUseLeftTopEmulationConstantBufferOffsets[0] =
        rDrawInfo.GetUi2dConstantBuffer()->AllocateWithoutAlignment(alignedSize);

    u8* pMapped = static_cast<u8*>(rDrawInfo.GetUi2dConstantBuffer()->GetMappedPointer());
    if (pMapped != nullptr) {
        VertexShaderConstantBuffer* pConstantBuffer = reinterpret_cast<VertexShaderConstantBuffer*>(
            pMapped + m_pUseLeftTopEmulationConstantBufferOffsets[0]);
        std::memcpy(pConstantBuffer, GetVertexShaderConstantBuffer(rFrame.pMaterial, rDrawInfo),
                    sizeof(VertexShaderConstantBuffer));
        pConstantBuffer->frameTransform = detail::DecideFrame4Transform(
            WindowFrame_RightTop, TextureFlip_FlipH, (m_WindowFlags & Flag_UseVertexColorAll) != 0);

        const nn::util::Float2 pos = {
            {rBasePos.x + GetSize().width - rFrameSize.right, rBasePos.y}};
        const Size size = {rFrameSize.right, GetSize().height - rFrameSize.bottom};
        detail::CalculateQuad(rDrawInfo, ToMaterialConstantBuffer(pConstantBuffer), pos, size);
    }

    m_pUseLeftTopEmulationConstantBufferOffsets[1] =
        rDrawInfo.GetUi2dConstantBuffer()->AllocateWithoutAlignment(alignedSize);

    pMapped = static_cast<u8*>(rDrawInfo.GetUi2dConstantBuffer()->GetMappedPointer());
    if (pMapped != nullptr) {
        VertexShaderConstantBuffer* pConstantBuffer = reinterpret_cast<VertexShaderConstantBuffer*>(
            pMapped + m_pUseLeftTopEmulationConstantBufferOffsets[1]);
        std::memcpy(pConstantBuffer, GetVertexShaderConstantBuffer(rFrame.pMaterial, rDrawInfo),
                    sizeof(VertexShaderConstantBuffer));
        pConstantBuffer->frameTransform =
            detail::DecideFrame4Transform(WindowFrame_RightBottom, TextureFlip_Rotate180,
                                          (m_WindowFlags & Flag_UseVertexColorAll) != 0);

        const nn::util::Float2 pos = {
            {rBasePos.x + rFrameSize.left, rBasePos.y - GetSize().height + rFrameSize.bottom}};
        const Size size = {GetSize().width - rFrameSize.left, rFrameSize.bottom};
        detail::CalculateQuad(rDrawInfo, ToMaterialConstantBuffer(pConstantBuffer), pos, size);
    }

    m_pUseLeftTopEmulationConstantBufferOffsets[2] =
        rDrawInfo.GetUi2dConstantBuffer()->AllocateWithoutAlignment(alignedSize);

    pMapped = static_cast<u8*>(rDrawInfo.GetUi2dConstantBuffer()->GetMappedPointer());
    if (pMapped != nullptr) {
        VertexShaderConstantBuffer* pConstantBuffer = reinterpret_cast<VertexShaderConstantBuffer*>(
            pMapped + m_pUseLeftTopEmulationConstantBufferOffsets[2]);
        std::memcpy(pConstantBuffer, GetVertexShaderConstantBuffer(rFrame.pMaterial, rDrawInfo),
                    sizeof(VertexShaderConstantBuffer));
        pConstantBuffer->frameTransform =
            detail::DecideFrame4Transform(WindowFrame_LeftBottom, TextureFlip_FlipV,
                                          (m_WindowFlags & Flag_UseVertexColorAll) != 0);

        const nn::util::Float2 pos = {{rBasePos.x, rBasePos.y - rFrameSize.top}};
        const Size size = {rFrameSize.left, GetSize().height - rFrameSize.top};
        detail::CalculateQuad(rDrawInfo, ToMaterialConstantBuffer(pConstantBuffer), pos, size);
    }
}

/**
 * @brief Calculates the four corner frames of a window.
 * @param rDrawInfo Drawing state.
 * @param rBasePos Left-top position of the pane.
 * @param pFrames Frames to calculate.
 * @param rFrameSize Frame thickness of each edge.
 * @param alpha Alpha of the pane.
 */
void Window::CalculateFrame4(DrawInfo& rDrawInfo, const nn::util::Float2& rBasePos,
                             const Frame* pFrames, const WindowFrameSize& rFrameSize, u8 alpha) {
    const float width = GetSize().width;
    const float height = GetSize().height;
    const nn::util::Float2 positions[4] = {
        rBasePos,
        {{rBasePos.x + width - rFrameSize.right, rBasePos.y}},
        {{rBasePos.x, rBasePos.y - rFrameSize.top}},
        {{rBasePos.x + rFrameSize.left, rBasePos.y - height + rFrameSize.bottom}},
    };
    const Size sizes[4] = {
        {width - rFrameSize.right, rFrameSize.top},
        {rFrameSize.right, height - rFrameSize.bottom},
        {rFrameSize.left, height - rFrameSize.top},
        {width - rFrameSize.left, rFrameSize.bottom},
    };

    const bool isVertexColorNeeded = !IsContentVertexColorWhite() || GetGlobalAlpha() != 0xff;
    const bool isVertexColorAll = (m_WindowFlags & Flag_UseVertexColorAll) != 0;
    const bool isOneMaterialForAll = (m_WindowFlags & Flag_UseOneMaterialForAll) != 0;

    for (int i = 0; i < 4; ++i) {
        Material* pMaterial = pFrames[i].pMaterial;
        if (pMaterial == nullptr || pMaterial->GetTexMapCount() <= 0) {
            continue;
        }

        if (isVertexColorNeeded || pMaterial->IsCombinerUserShaderCapable()) {
            pMaterial->SetupGraphics(rDrawInfo, alpha, ShaderVariation_Standard, false,
                                     GetGlobalMatrix(), &GetSize(), GetExtUserDataArray(),
                                     GetExtUserDataCount());

            if (isVertexColorAll) {
                SetupFrameSize(GetVertexShaderConstantBuffer(pFrames[i].pMaterial, rDrawInfo),
                               GetSize(), rFrameSize);

                SetupVertexColors(GetVertexShaderConstantBuffer(pFrames[i].pMaterial, rDrawInfo),
                                  m_Content.vtxColors);
            }
        } else {
            pMaterial->SetupGraphics(rDrawInfo, alpha, ShaderVariation_WithoutVertexColor, false,
                                     GetGlobalMatrix(), &GetSize(), GetExtUserDataArray(),
                                     GetExtUserDataCount());

            if (isVertexColorAll) {
                SetupFrameSize(GetVertexShaderConstantBuffer(pFrames[i].pMaterial, rDrawInfo),
                               GetSize(), rFrameSize);
            }
        }

        if (isOneMaterialForAll && i != 0) {
            VertexShaderConstantBuffer* pConstantBuffer =
                GetVertexShaderConstantBuffer(pFrames[i].pMaterial, rDrawInfo);
            u8 texCoordData[16];
            std::memcpy(texCoordData, pConstantBuffer->texCoordData, sizeof(texCoordData));
            std::memcpy(pConstantBuffer,
                        GetVertexShaderConstantBuffer(pFrames[0].pMaterial, rDrawInfo),
                        sizeof(VertexShaderConstantBuffer));
            std::memcpy(pConstantBuffer->texCoordData, texCoordData, sizeof(texCoordData));
        }

        const u32 transform = detail::DecideFrame4Transform(
            static_cast<WindowFrame>(i), pFrames[i].GetTextureFlip(), isVertexColorAll);
        GetVertexShaderConstantBuffer(pFrames[i].pMaterial, rDrawInfo)->frameTransform = transform;
        detail::CalculateQuad(rDrawInfo,
                              ToMaterialConstantBuffer(
                                  GetVertexShaderConstantBuffer(pFrames[i].pMaterial, rDrawInfo)),
                              positions[i], sizes[i]);
    }
}

/**
 * @brief Calculates the four corner and four edge frames of a window.
 * @param rDrawInfo Drawing state.
 * @param rBasePos Left-top position of the pane.
 * @param pFrames Frames to calculate.
 * @param rFrameSize Frame thickness of each edge.
 * @param alpha Alpha of the pane.
 */
void Window::CalculateFrame8(DrawInfo& rDrawInfo, const nn::util::Float2& rBasePos,
                             const Frame* pFrames, const WindowFrameSize& rFrameSize, u8 alpha) {
    const float left = rBasePos.x;
    const float top = rBasePos.y;
    const float right = rBasePos.x + GetSize().width - rFrameSize.right;
    const float bottom = rBasePos.y - GetSize().height + rFrameSize.bottom;
    const float innerLeft = rBasePos.x + rFrameSize.left;
    const float innerTop = rBasePos.y - rFrameSize.top;
    const float innerWidth = GetSize().width - rFrameSize.left - rFrameSize.right;
    const float innerHeight = GetSize().height - rFrameSize.top - rFrameSize.bottom;

    const FrameRect rects[WindowFrame_MaxWindowFrame] = {
        {left, top, rFrameSize.left, rFrameSize.top},
        {right, top, rFrameSize.right, rFrameSize.top},
        {left, bottom, rFrameSize.left, rFrameSize.bottom},
        {right, bottom, rFrameSize.right, rFrameSize.bottom},
        {left, innerTop, rFrameSize.left, innerHeight},
        {right, innerTop, rFrameSize.right, innerHeight},
        {innerLeft, top, innerWidth, rFrameSize.top},
        {innerLeft, bottom, innerWidth, rFrameSize.bottom},
    };

    const bool isVertexColorNeeded = !IsContentVertexColorWhite() || GetGlobalAlpha() != 0xff;
    const bool isVertexColorAll = (m_WindowFlags & Flag_UseVertexColorAll) != 0;
    const bool isOneMaterialForAll = (m_WindowFlags & Flag_UseOneMaterialForAll) != 0;

    for (int i = 0; i < WindowFrame_MaxWindowFrame; ++i) {
        Material* pMaterial = pFrames[i].pMaterial;
        if (pMaterial->GetTexMapCount() <= 0) {
            continue;
        }

        if (isVertexColorNeeded || pMaterial->IsCombinerUserShaderCapable()) {
            pMaterial->SetupGraphics(rDrawInfo, alpha, ShaderVariation_Standard, false,
                                     GetGlobalMatrix(), &GetSize(), GetExtUserDataArray(),
                                     GetExtUserDataCount());

            if (isVertexColorAll) {
                SetupFrameSize(GetVertexShaderConstantBuffer(pFrames[i].pMaterial, rDrawInfo),
                               GetSize(), rFrameSize);

                SetupVertexColors(GetVertexShaderConstantBuffer(pFrames[i].pMaterial, rDrawInfo),
                                  m_Content.vtxColors);
            }
        } else {
            pMaterial->SetupGraphics(rDrawInfo, alpha, ShaderVariation_WithoutVertexColor, false,
                                     GetGlobalMatrix(), &GetSize(), GetExtUserDataArray(),
                                     GetExtUserDataCount());

            if (isVertexColorAll) {
                SetupFrameSize(GetVertexShaderConstantBuffer(pFrames[i].pMaterial, rDrawInfo),
                               GetSize(), rFrameSize);
            }
        }

        if (isOneMaterialForAll && i != 0) {
            VertexShaderConstantBuffer* pConstantBuffer =
                GetVertexShaderConstantBuffer(pFrames[i].pMaterial, rDrawInfo);
            u8 texCoordData[16];
            std::memcpy(texCoordData, pConstantBuffer->texCoordData, sizeof(texCoordData));
            std::memcpy(pConstantBuffer,
                        GetVertexShaderConstantBuffer(pFrames[0].pMaterial, rDrawInfo),
                        sizeof(VertexShaderConstantBuffer));
            std::memcpy(pConstantBuffer->texCoordData, texCoordData, sizeof(texCoordData));
            pConstantBuffer->frameTransform = detail::DecideFrameTransform(
                static_cast<WindowFrame>(i), pFrames[i].GetTextureFlip(), isVertexColorAll);
        } else {
            const u32 transform = detail::DecideFrameTransform(
                static_cast<WindowFrame>(i), pFrames[i].GetTextureFlip(), isVertexColorAll);
            GetVertexShaderConstantBuffer(pFrames[i].pMaterial, rDrawInfo)->frameTransform =
                transform;
        }

        const nn::util::Float2 pos = {{rects[i].x, rects[i].y}};
        const Size size = {rects[i].width, rects[i].height};
        detail::CalculateQuad(rDrawInfo,
                              ToMaterialConstantBuffer(
                                  GetVertexShaderConstantBuffer(pFrames[i].pMaterial, rDrawInfo)),
                              pos, size);
    }
}

/**
 * @brief Calculates the content area of a window.
 * @param rDrawInfo Drawing state.
 * @param rBasePos Left-top position of the pane.
 * @param rFrameSize Frame thickness of each edge.
 * @param alpha Alpha of the pane.
 */
void Window::CalculateContent(DrawInfo& rDrawInfo, const nn::util::Float2& rBasePos,
                              const WindowFrameSize& rFrameSize, u8 alpha) {
    static_cast<void>(alpha);

    nn::util::Float2 pos = {{rBasePos.x + rFrameSize.left, rBasePos.y - rFrameSize.top}};
    Size size = GetSize();
    size.width = size.width - rFrameSize.left - rFrameSize.right;
    size.height = size.height - rFrameSize.top - rFrameSize.bottom;

    if ((m_WindowFlags & Flag_ContentInflationEnabled) != 0) {
        const float inflationScale = 1.0f / 16.0f;
        const float inflationLeft = m_WindowSize.inflation.left * inflationScale;
        const float inflationRight = m_WindowSize.inflation.right * inflationScale;
        const float inflationTop = m_WindowSize.inflation.top * inflationScale;
        const float inflationBottom = m_WindowSize.inflation.bottom * inflationScale;

        pos.x -= inflationLeft;
        pos.y += inflationTop;
        size.width += inflationLeft + inflationRight;
        size.height += inflationTop + inflationBottom;
    }

    detail::CalculateQuadWithTexCoords(
        rDrawInfo, ToMaterialConstantBuffer(GetVertexShaderConstantBuffer(m_pMaterial, rDrawInfo)),
        pos, size, m_Content.texCoordArray.GetSize(), m_Content.texCoordArray.GetArray());
}

/**
 * @brief Calculates the single frame of a horizontal window and emulates it on the right side.
 * @param rDrawInfo Drawing state.
 * @param rBasePos Left-top position of the pane.
 * @param rFrame Frame to calculate.
 * @param rFrameSize Frame thickness of each edge.
 * @param alpha Alpha of the pane.
 */
void Window::CalculateHorizontalFrame(DrawInfo& rDrawInfo, const nn::util::Float2& rBasePos,
                                      const Frame& rFrame, const WindowFrameSize& rFrameSize,
                                      u8 alpha) {
    Material* pMaterial = rFrame.pMaterial;
    if (pMaterial->GetTexMapCount() <= 0) {
        return;
    }

    if (pMaterial->IsCombinerUserShaderCapable() || !IsContentVertexColorWhite() ||
        GetGlobalAlpha() != 0xff) {
        pMaterial->SetupGraphics(rDrawInfo, alpha, ShaderVariation_Standard, false,
                                 GetGlobalMatrix(), &GetSize(), GetExtUserDataArray(),
                                 GetExtUserDataCount());
        SetupVertexColors(GetVertexShaderConstantBuffer(rFrame.pMaterial, rDrawInfo),
                          m_Content.vtxColors);

        if ((m_WindowFlags & Flag_UseVertexColorAll) != 0) {
            SetupFrameSize(GetVertexShaderConstantBuffer(rFrame.pMaterial, rDrawInfo), GetSize(),
                           rFrameSize);
        }
    } else {
        pMaterial->SetupGraphics(rDrawInfo, alpha, ShaderVariation_WithoutVertexColor, false,
                                 GetGlobalMatrix(), &GetSize(), GetExtUserDataArray(),
                                 GetExtUserDataCount());

        if ((m_WindowFlags & Flag_UseVertexColorAll) != 0) {
            SetupFrameSize(GetVertexShaderConstantBuffer(rFrame.pMaterial, rDrawInfo), GetSize(),
                           rFrameSize);
        }
    }

    {
        const u32 transform = detail::DecideHorizontalFrameTransform(
            WindowFrame_LeftTop, TextureFlip_None, (m_WindowFlags & Flag_UseVertexColorAll) != 0);
        GetVertexShaderConstantBuffer(rFrame.pMaterial, rDrawInfo)->frameTransform = transform;

        const nn::util::Float2 pos = rBasePos;
        const Size size = {rFrameSize.left, GetSize().height};
        detail::CalculateQuad(
            rDrawInfo,
            ToMaterialConstantBuffer(GetVertexShaderConstantBuffer(rFrame.pMaterial, rDrawInfo)),
            pos, size);
    }

    const size_t alignment = rDrawInfo.GetGraphicsResource()->m_ConstantBufferAlignment;
    const size_t alignedSize = (UseLeftTopEmulationConstantBufferSize + alignment - 1) & -alignment;
    m_pUseLeftTopEmulationConstantBufferOffsets[0] =
        rDrawInfo.GetUi2dConstantBuffer()->AllocateWithoutAlignment(alignedSize);

    VertexShaderConstantBuffer* pConstantBuffer = reinterpret_cast<VertexShaderConstantBuffer*>(
        static_cast<u8*>(rDrawInfo.GetUi2dConstantBuffer()->GetMappedPointer()) +
        m_pUseLeftTopEmulationConstantBufferOffsets[0]);
    if (pConstantBuffer != nullptr) {
        std::memcpy(pConstantBuffer, GetVertexShaderConstantBuffer(rFrame.pMaterial, rDrawInfo),
                    sizeof(VertexShaderConstantBuffer));
        pConstantBuffer->frameTransform = detail::DecideHorizontalFrameTransform(
            WindowFrame_RightTop, TextureFlip_FlipH, (m_WindowFlags & Flag_UseVertexColorAll) != 0);

        const nn::util::Float2 pos = {
            {rBasePos.x + GetSize().width - rFrameSize.right, rBasePos.y}};
        const Size size = {rFrameSize.right, GetSize().height};
        detail::CalculateQuad(rDrawInfo, ToMaterialConstantBuffer(pConstantBuffer), pos, size);
    }
}

/**
 * @brief Calculates the two frames of a horizontal window.
 * @param rDrawInfo Drawing state.
 * @param rBasePos Left-top position of the pane.
 * @param pFrames Frames to calculate.
 * @param rFrameSize Frame thickness of each edge.
 * @param alpha Alpha of the pane.
 */
void Window::CalculateHorizontalFrame2(DrawInfo& rDrawInfo, const nn::util::Float2& rBasePos,
                                       const Frame* pFrames, const WindowFrameSize& rFrameSize,
                                       u8 alpha) {
    const bool isVertexColorNeeded = !IsContentVertexColorWhite() || GetGlobalAlpha() != 0xff;
    const bool isVertexColorAll = (m_WindowFlags & Flag_UseVertexColorAll) != 0;

    for (int i = 0; i < 2; ++i) {
        Material* pMaterial = pFrames[i].pMaterial;

        if (isVertexColorNeeded || pMaterial->IsCombinerUserShaderCapable()) {
            pMaterial->SetupGraphics(rDrawInfo, alpha, ShaderVariation_Standard, false,
                                     GetGlobalMatrix(), &GetSize(), GetExtUserDataArray(),
                                     GetExtUserDataCount());

            if (isVertexColorAll) {
                SetupFrameSize(GetVertexShaderConstantBuffer(pFrames[i].pMaterial, rDrawInfo),
                               GetSize(), rFrameSize);

                SetupVertexColors(GetVertexShaderConstantBuffer(pFrames[i].pMaterial, rDrawInfo),
                                  m_Content.vtxColors);
            }
        } else {
            pMaterial->SetupGraphics(rDrawInfo, alpha, ShaderVariation_WithoutVertexColor, false,
                                     GetGlobalMatrix(), &GetSize(), GetExtUserDataArray(),
                                     GetExtUserDataCount());

            if (isVertexColorAll) {
                SetupFrameSize(GetVertexShaderConstantBuffer(pFrames[i].pMaterial, rDrawInfo),
                               GetSize(), rFrameSize);
            }
        }

        nn::util::Float2 pos;
        Size size;
        if (i == 0) {
            pos = rBasePos;
            size.width = rFrameSize.left;
            size.height = GetSize().height;
        } else {
            pos.x = rBasePos.x + GetSize().width - rFrameSize.right;
            pos.y = rBasePos.y;
            size.width = rFrameSize.right;
            size.height = GetSize().height;
        }

        if (i != 0 && (m_WindowFlags & Flag_UseOneMaterialForAll) != 0) {
            VertexShaderConstantBuffer* pConstantBuffer =
                GetVertexShaderConstantBuffer(pFrames[i].pMaterial, rDrawInfo);
            std::memcpy(pConstantBuffer,
                        GetVertexShaderConstantBuffer(pFrames[0].pMaterial, rDrawInfo),
                        sizeof(VertexShaderConstantBuffer));
            pConstantBuffer->frameTransform = detail::DecideHorizontalFrameTransform(
                static_cast<WindowFrame>(i), pFrames[i].GetTextureFlip(), isVertexColorAll);
            detail::CalculateQuad(rDrawInfo, ToMaterialConstantBuffer(pConstantBuffer), pos, size);
        } else {
            const u32 transform = detail::DecideHorizontalFrameTransform(
                static_cast<WindowFrame>(i), pFrames[i].GetTextureFlip(), isVertexColorAll);
            GetVertexShaderConstantBuffer(pFrames[i].pMaterial, rDrawInfo)->frameTransform =
                transform;
            detail::CalculateQuad(rDrawInfo,
                                  ToMaterialConstantBuffer(GetVertexShaderConstantBuffer(
                                      pFrames[i].pMaterial, rDrawInfo)),
                                  pos, size);
        }

        pFrames[i].pMaterial->m_DrawTextureNum = rDrawInfo.m_TexMapNum;
    }
}

/**
 * @brief Calculates the single frame of a horizontal window without content and emulates it on the
 * right side.
 * @param rDrawInfo Drawing state.
 * @param rBasePos Left-top position of the pane.
 * @param rFrame Frame to calculate.
 * @param rFrameSize Frame thickness of each edge.
 * @param alpha Alpha of the pane.
 */
void Window::CalculateHorizontalNocontentFrame(DrawInfo& rDrawInfo,
                                               const nn::util::Float2& rBasePos,
                                               const Frame& rFrame,
                                               const WindowFrameSize& rFrameSize, u8 alpha) {
    Material* pMaterial = rFrame.pMaterial;
    if (pMaterial->GetTexMapCount() <= 0) {
        return;
    }

    if (pMaterial->IsCombinerUserShaderCapable() || !IsContentVertexColorWhite() ||
        GetGlobalAlpha() != 0xff) {
        pMaterial->SetupGraphics(rDrawInfo, alpha, ShaderVariation_Standard, false,
                                 GetGlobalMatrix(), &GetSize(), GetExtUserDataArray(),
                                 GetExtUserDataCount());
        SetupVertexColors(GetVertexShaderConstantBuffer(rFrame.pMaterial, rDrawInfo),
                          m_Content.vtxColors);

        if ((m_WindowFlags & Flag_UseVertexColorAll) != 0) {
            SetupFrameSize(GetVertexShaderConstantBuffer(rFrame.pMaterial, rDrawInfo), GetSize(),
                           rFrameSize);
        }
    } else {
        pMaterial->SetupGraphics(rDrawInfo, alpha, ShaderVariation_WithoutVertexColor, false,
                                 GetGlobalMatrix(), &GetSize(), GetExtUserDataArray(),
                                 GetExtUserDataCount());

        if ((m_WindowFlags & Flag_UseVertexColorAll) != 0) {
            SetupFrameSize(GetVertexShaderConstantBuffer(rFrame.pMaterial, rDrawInfo), GetSize(),
                           rFrameSize);
        }
    }

    {
        const u32 transform = detail::DecideHorizontalNocontextFrameTransform(
            WindowFrame_LeftTop, TextureFlip_None, (m_WindowFlags & Flag_UseVertexColorAll) != 0);
        GetVertexShaderConstantBuffer(rFrame.pMaterial, rDrawInfo)->frameTransform = transform;

        const nn::util::Float2 pos = rBasePos;
        const Size size = {rFrameSize.left, GetSize().height};
        detail::CalculateQuad(
            rDrawInfo,
            ToMaterialConstantBuffer(GetVertexShaderConstantBuffer(rFrame.pMaterial, rDrawInfo)),
            pos, size);
    }

    const size_t alignment = rDrawInfo.GetGraphicsResource()->m_ConstantBufferAlignment;
    const size_t alignedSize = (UseLeftTopEmulationConstantBufferSize + alignment - 1) & -alignment;
    m_pUseLeftTopEmulationConstantBufferOffsets[0] =
        rDrawInfo.GetUi2dConstantBuffer()->AllocateWithoutAlignment(alignedSize);

    VertexShaderConstantBuffer* pConstantBuffer = reinterpret_cast<VertexShaderConstantBuffer*>(
        static_cast<u8*>(rDrawInfo.GetUi2dConstantBuffer()->GetMappedPointer()) +
        m_pUseLeftTopEmulationConstantBufferOffsets[0]);
    if (pConstantBuffer != nullptr) {
        std::memcpy(pConstantBuffer, GetVertexShaderConstantBuffer(rFrame.pMaterial, rDrawInfo),
                    sizeof(VertexShaderConstantBuffer));
        pConstantBuffer->frameTransform = detail::DecideHorizontalNocontextFrameTransform(
            WindowFrame_RightTop, TextureFlip_FlipH, (m_WindowFlags & Flag_UseVertexColorAll) != 0);

        const nn::util::Float2 pos = {
            {rBasePos.x + GetSize().width - rFrameSize.right, rBasePos.y}};
        const Size size = {rFrameSize.right, GetSize().height};
        detail::CalculateQuad(rDrawInfo, ToMaterialConstantBuffer(pConstantBuffer), pos, size);
    }
}

/**
 * @brief Calculates the two frames of a horizontal window without content.
 * @param rDrawInfo Drawing state.
 * @param rBasePos Left-top position of the pane.
 * @param pFrames Frames to calculate.
 * @param rFrameSize Frame thickness of each edge.
 * @param alpha Alpha of the pane.
 */
void Window::CalculateHorizontalNocontentFrame2(DrawInfo& rDrawInfo,
                                                const nn::util::Float2& rBasePos,
                                                const Frame* pFrames,
                                                const WindowFrameSize& rFrameSize, u8 alpha) {
    const bool isVertexColorNeeded = !IsContentVertexColorWhite() || GetGlobalAlpha() != 0xff;
    const bool isVertexColorAll = (m_WindowFlags & Flag_UseVertexColorAll) != 0;

    for (int i = 0; i < 2; ++i) {
        Material* pMaterial = pFrames[i].pMaterial;

        if (isVertexColorNeeded || pMaterial->IsCombinerUserShaderCapable()) {
            pMaterial->SetupGraphics(rDrawInfo, alpha, ShaderVariation_Standard, false,
                                     GetGlobalMatrix(), &GetSize(), GetExtUserDataArray(),
                                     GetExtUserDataCount());

            if (isVertexColorAll) {
                SetupFrameSize(GetVertexShaderConstantBuffer(pFrames[i].pMaterial, rDrawInfo),
                               GetSize(), rFrameSize);

                SetupVertexColors(GetVertexShaderConstantBuffer(pFrames[i].pMaterial, rDrawInfo),
                                  m_Content.vtxColors);
            }
        } else {
            pMaterial->SetupGraphics(rDrawInfo, alpha, ShaderVariation_WithoutVertexColor, false,
                                     GetGlobalMatrix(), &GetSize(), GetExtUserDataArray(),
                                     GetExtUserDataCount());

            if (isVertexColorAll) {
                SetupFrameSize(GetVertexShaderConstantBuffer(pFrames[i].pMaterial, rDrawInfo),
                               GetSize(), rFrameSize);
            }
        }

        nn::util::Float2 pos;
        Size size;
        if (i == 0) {
            pos = rBasePos;
            size.width = rFrameSize.left;
            size.height = GetSize().height;
        } else {
            pos.x = rBasePos.x + GetSize().width - rFrameSize.right;
            pos.y = rBasePos.y;
            size.width = rFrameSize.right;
            size.height = GetSize().height;
        }

        if (i != 0 && (m_WindowFlags & Flag_UseOneMaterialForAll) != 0) {
            VertexShaderConstantBuffer* pConstantBuffer =
                GetVertexShaderConstantBuffer(pFrames[i].pMaterial, rDrawInfo);
            std::memcpy(pConstantBuffer,
                        GetVertexShaderConstantBuffer(pFrames[0].pMaterial, rDrawInfo),
                        sizeof(VertexShaderConstantBuffer));
            pConstantBuffer->frameTransform = detail::DecideHorizontalNocontextFrameTransform(
                static_cast<WindowFrame>(i), pFrames[i].GetTextureFlip(), isVertexColorAll);
            detail::CalculateQuad(rDrawInfo, ToMaterialConstantBuffer(pConstantBuffer), pos, size);
        } else {
            const u32 transform = detail::DecideHorizontalNocontextFrameTransform(
                static_cast<WindowFrame>(i), pFrames[i].GetTextureFlip(), isVertexColorAll);
            GetVertexShaderConstantBuffer(pFrames[i].pMaterial, rDrawInfo)->frameTransform =
                transform;
            detail::CalculateQuad(rDrawInfo,
                                  ToMaterialConstantBuffer(GetVertexShaderConstantBuffer(
                                      pFrames[i].pMaterial, rDrawInfo)),
                                  pos, size);
        }

        pFrames[i].pMaterial->m_DrawTextureNum = rDrawInfo.m_TexMapNum;
    }
}
}  // namespace nn::ui2d
