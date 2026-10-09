#pragma once

#include <nn/ui2d/ui2d_Pane.h>

namespace nn::ui2d {
struct ResPicture;
class TextureInfo;
class BuildResultInformation;

class Picture : public Pane {
public:
    explicit Picture(int);
    explicit Picture(const TextureInfo&);
    Picture(BuildResultInformation*, nn::gfx::Device*, const ResPicture*, const ResPicture*,
            const BuildArgSet&);
    Picture(const Picture& rOther, nn::gfx::Device* pDevice)
        : Pane(rOther), m_pMaterial(nullptr), _F0(0), _F8(nullptr) {
        CopyImpl(rOther, pDevice, nullptr, nullptr);
    }
    ~Picture() override;
    NN_RUNTIME_TYPEINFO(Pane);
    bool CompareCopiedInstanceTest(const Picture& rOther) const;
    void Finalize(nn::gfx::Device*) override;
    nn::util::Unorm8x4 GetVertexColor(int) const override;
    void SetVertexColor(int, const nn::util::Unorm8x4&) override;
    u8 GetVertexColorElement(int) const override;
    void SetVertexColorElement(int, u8) override;
    u32 GetMaterialCount() const override;
    Material* GetMaterial(int) const override;
    using Pane::GetMaterial;
    void Calculate(DrawInfo&, CalculateContext&, bool) override;
    void DrawSelf(DrawInfo&, nn::gfx::CommandBuffer&) override;
    void SetupPaneEffectSourceImageRenderState(nn::gfx::CommandBuffer&) const override;
    void LoadMtx(DrawInfo&) override;
    virtual void Append(const TextureInfo&);
    void CopyImpl(const Picture&, nn::gfx::Device*, const Layout*, detail::BuildPaneTreeContext*);

    Material* m_pMaterial;
    nn::util::Unorm8x4 m_VertexColors[4];
    u64 _F0;
    void* _F8;
};
}  // namespace nn::ui2d
