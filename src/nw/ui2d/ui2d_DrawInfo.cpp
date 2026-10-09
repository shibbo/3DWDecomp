#include <nn/ui2d/ui2d_DrawInfo.h>
#include <nn/ui2d/ui2d_GraphicsResource.h>
#include <nn/font/font_GpuBuffer.h>
#include <nn/gfx/gfx_CommandBuffer.h>
#include <nn/gfx/gfx_Texture.h>

namespace nn::ui2d {

DrawInfo::DrawInfo()
    : m_pGraphicsResource(nullptr), m_pLayoutInformation(nullptr),
      m_pConstantBuffer(nullptr), m_pFontConstantBuffer(nullptr), m_pFramebufferTexture(nullptr),
      mFramebufferWidth(0), mFramebufferHeight(0), m_pFramebufferTextureSlot(nullptr),
      m_pFramebufferSamplerSlot(nullptr), m_pColorTarget(nullptr), m_pDepthTarget(nullptr),
      m_pDepthStencilState(nullptr), m_pRasterizerState(nullptr), _168{},
      mModelViewLoaded(false), mVertexBufferDirty(false), m_TexMapNum(0),
      m_pCurrentShader(nullptr), mCurrentShaderVariation(0) {
    m_LocationAdjustScale = {1, 1};
    mViewport.SetDefault();
    mScissor.SetDefault();
    m_ProjMtx._m.val[0] = {1, 0, 0, 0};
    m_ProjMtx._m.val[1] = {0, 1, 0, 0};
    m_ProjMtx._m.val[2] = {0, 0, 1, 0};
    m_ProjMtx._m.val[3] = {0, 0, 0, 1};
    m_ViewMtx._m.val[0] = {1, 0, 0, 0};
    m_ViewMtx._m.val[1] = {0, 1, 0, 0};
    m_ViewMtx._m.val[2] = {0, 0, 1, 0};
    m_ModelViewMtx._m.val[0] = {1, 0, 0, 0};
    m_ModelViewMtx._m.val[1] = {0, 1, 0, 0};
    m_ModelViewMtx._m.val[2] = {0, 0, 1, 0};
    mFlags = 0;
}

DrawInfo::~DrawInfo() = default;

// pLayout supplies layout information during this drawing pass.
void DrawInfo::ConfigureBeforeDrawing(Layout* pLayout) {
    ResetDrawState();
    m_pLayoutInformation = reinterpret_cast<const Pane::CalculateContext::LayoutInformation*>(pLayout);
}

void DrawInfo::ResetDrawState() {
    mVertexBufferDirty = true;
    ResetCurrentShader();
}

void DrawInfo::ConfigureAfterDrawing() { m_pLayoutInformation = nullptr; }

void DrawInfo::ResetCurrentShader() {
    m_pCurrentShader = nullptr;
    mCurrentShaderVariation = 0;
}

// bufferIndex selects the CPU-writable buffer in each optional constant-buffer collection.
void DrawInfo::Map(int bufferIndex) {
    if (m_pConstantBuffer != nullptr) m_pConstantBuffer->Map(bufferIndex);

    if (m_pFontConstantBuffer != nullptr) m_pFontConstantBuffer->Map(bufferIndex);
}

void DrawInfo::Unmap() {
    if (m_pConstantBuffer != nullptr) m_pConstantBuffer->Unmap();

    if (m_pFontConstantBuffer != nullptr) m_pFontConstantBuffer->Unmap();
}

// bufferIndex selects which constant buffer each subsequent GPU command reads.
void DrawInfo::SetGpuAccessBufferIndex(int bufferIndex) {
    if (m_pConstantBuffer != nullptr) m_pConstantBuffer->m_GpuAccessBufferIndex = bufferIndex;

    if (m_pFontConstantBuffer != nullptr) m_pFontConstantBuffer->m_GpuAccessBufferIndex = bufferIndex;
}

// pTexture supplies the framebuffer image; width and height give its dimensions in pixels.
void DrawInfo::SetFramebufferTexture(const nn::gfx::Texture* pTexture, int width, int height) {
    m_pFramebufferTexture = pTexture;
    mFramebufferWidth = width;
    mFramebufferHeight = height;
}

// rProjection supplies the column-major projection used by subsequent drawing.
void DrawInfo::SetProjectionMtx(const nn::util::MatrixT4x4fType& rProjection) {
    m_ProjMtx = rProjection;
}

// pCommands receives vertex-buffer binding when the cached drawing state is dirty.
void DrawInfo::SetupProgram(nn::gfx::CommandBuffer* pCommands) {
    if (mVertexBufferDirty) {
        mVertexBufferDirty = false;
        m_pGraphicsResource->ActivateVertexBuffer(pCommands);
    }
}

// pShader and variation identify the next program; return whether either differs from the cache.
bool DrawInfo::RecordCurrentShader(const ShaderInfo* pShader, u16 variation) {
    if (m_pCurrentShader == pShader && mCurrentShaderVariation == variation) return false;
    m_pCurrentShader = pShader;
    mCurrentShaderVariation = variation;
    return true;
}

// rCommands receives the saved render targets, scissor, viewport, and optional pipeline states.
void DrawInfo::ResetRenderTarget(nn::gfx::CommandBuffer& rCommands) const {
    using Impl = nn::gfx::detail::CommandBufferImpl<nn::gfx::ApiVariationNvn8>;
    using ColorView = nn::gfx::detail::ColorTargetViewImpl<nn::gfx::ApiVariationNvn8>;
    auto& commands = static_cast<Impl&>(rCommands);
    commands.SetRenderTargets(1, reinterpret_cast<const ColorView* const*>(&m_pColorTarget), m_pDepthTarget);
    commands.SetScissors(0, 1, &mScissor);
    commands.SetViewports(0, 1, &mViewport);

    if (m_pDepthStencilState) commands.SetDepthStencilState(m_pDepthStencilState);

    if (m_pRasterizerState) commands.SetRasterizerState(m_pRasterizerState);
}

// pMatrix receives four vectors containing the current column-major projection.
void DrawInfo::LoadProjectionMtx(float (*pMatrix)[4]) {
    const auto matrix = m_ProjMtx._m;
    vst1q_f32(pMatrix[0], matrix.val[0]);
    vst1q_f32(pMatrix[1], matrix.val[1]);
    vst1q_f32(pMatrix[2], matrix.val[2]);
    vst1q_f32(pMatrix[3], matrix.val[3]);
}

// pMatrix receives three model-view vectors once per model-view update.
void DrawInfo::LoadMtxModelView(float (*pMatrix)[4]) {
    if (mModelViewLoaded) return;
    const auto matrix = m_ModelViewMtx._m;
    mModelViewLoaded = true;
    vst1q_f32(pMatrix[0], matrix.val[0]);
    vst1q_f32(pMatrix[1], matrix.val[1]);
    vst1q_f32(pMatrix[2], matrix.val[2]);
}

// pSlot supplies the descriptor used to sample the framebuffer texture.
void DrawInfo::SetFramebufferTextureDescriptorSlot(const nn::gfx::DescriptorSlot* pSlot) {
    m_pFramebufferTextureSlot = pSlot;
}

// pSlot supplies the sampler descriptor paired with the framebuffer texture.
void DrawInfo::SetFramebufferSamplerDescriptorSlot(const nn::gfx::DescriptorSlot* pSlot) {
    m_pFramebufferSamplerSlot = pSlot;
}

const nn::gfx::DescriptorSlot* DrawInfo::GetFramebufferTextureDescriptorSlot() const {
    return m_pFramebufferTextureSlot;
}

const nn::gfx::DescriptorSlot* DrawInfo::GetFramebufferSamplerDescriptorSlot() const {
    return m_pFramebufferSamplerSlot;
}

}  // namespace nn::ui2d
