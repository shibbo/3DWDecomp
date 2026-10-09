#pragma once
#include <nn/ui2d/ui2d_ResourceAccessor.h>
#include <nn/ui2d/ui2d_ShaderContainer.h>
#include <nn/ui2d/ui2d_ArcExtractor.h>
#include <nn/ui2d/ui2d_TextureContainer.h>
namespace nn::gfx {
class ResTextureFile;
}  // namespace nn::gfx
namespace nn::ui2d {
class ArcResourceMgr;
class FontMgr;
class MultiArcResourceAccessorEx : public ResourceAccessor {
public:
    MultiArcResourceAccessorEx(const ArcResourceMgr* archives, const FontMgr* fonts);
    ~MultiArcResourceAccessorEx() override;
    NN_RUNTIME_TYPEINFO(ResourceAccessor);
    void RegisterTextureViewToDescriptorPool(RegisterTextureView callback, void* argument) override;
    void UnregisterTextureViewFromDescriptorPool(UnregisterTextureView callback, void* argument) override;
    void Finalize(nn::gfx::Device* device) override;
    void* FindResourceByName(size_t*, u32, const char*) override;
    void FindResourceByType(u32 type, ResourceCallback callback, void* argument) const override;
    nn::font::Font* AcquireFont(nn::gfx::Device* device, const char* name) override;
    TextureInfo* AcquireTexture(nn::gfx::Device*, const char*) override;
    ShaderInfo* AcquireShader(nn::gfx::Device*, const char*) override;
    ShaderInfo* AcquireArchiveShader(nn::gfx::Device*, u32, size_t, const u32*) override;
    bool LoadTexture(ResourceTextureInfo*, nn::gfx::Device*, const char*) override;
    bool LoadShader(ShaderInfo*, nn::gfx::Device*, const char*) override;
    bool LoadArchiveShader(ShaderInfo*, nn::gfx::Device*, u32, size_t, const u32*) override;
    bool IsArchiveAttached(void* archive);
    void AttachArchive(void* pArchive, nn::gfx::ResTextureFile* pTextureFile);
    struct ArchiveLink { nn::util::IntrusiveListNode link; ArcExtractor extractor; };
    struct TextureLink { nn::util::IntrusiveListNode link; ResourceTextureInfo texture; };
    using ArchiveList = nn::util::IntrusiveList<ArchiveLink, nn::util::IntrusiveListMemberNodeTraits<ArchiveLink, &ArchiveLink::link>>;
    using TextureList = nn::util::IntrusiveList<TextureLink, nn::util::IntrusiveListMemberNodeTraits<TextureLink, &TextureLink::link>>;
    const ArcResourceMgr* mArchiveManager;
    const FontMgr* mFontManager;
    ShaderContainer mShaders;
    ArchiveList mArchives;
    TextureList mTextures;
};
static_assert(sizeof(MultiArcResourceAccessorEx) == 0x48, "MultiArcResourceAccessorEx size");
}
