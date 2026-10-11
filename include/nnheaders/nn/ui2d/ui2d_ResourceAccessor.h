#pragma once
#include <nn/font/font_Util.h>
#include <nn/gfx/gfx_DescriptorSlot.h>
namespace nn::font { class Font; }
namespace nn::ui2d {
class TextureInfo;
class ResourceTextureInfo;
class ShaderInfo;
struct BuildArgSet;
using RegisterTextureView = bool (*)(nn::gfx::DescriptorSlot*, const nn::gfx::TextureView&, void*);
using UnregisterTextureView = void (*)(nn::gfx::DescriptorSlot*, const nn::gfx::TextureView&, void*);
using ResourceCallback = void (*)(const void*, size_t, const char*, void*);
class ResourceAccessor {
public:
    NN_RUNTIME_TYPEINFO_BASE();
    ResourceAccessor();
    virtual ~ResourceAccessor();
    virtual void RegisterTextureViewToDescriptorPool(RegisterTextureView, void*) = 0;
    virtual void UnregisterTextureViewFromDescriptorPool(UnregisterTextureView, void*) = 0;
    virtual TextureInfo* RegisterRenderTargetTexture(const char*);
    virtual void UnregisterRenderTargetTexture(TextureInfo*);
    virtual void Finalize(nn::gfx::Device*);
    virtual void* FindResourceByName(size_t*, u32, const char*) = 0;
    virtual const void* FindResourceByName(size_t*, u32, const char*) const;
    virtual void* FindResourceByName(u32, const char*);
    virtual const void* FindResourceByName(u32, const char*) const;
    virtual void FindResourceByType(u32, ResourceCallback, void*) const = 0;
    virtual nn::font::Font* AcquireFont(nn::gfx::Device*, const char*) = 0;
    virtual TextureInfo* AcquireTexture(nn::gfx::Device*, const char*) = 0;
    virtual TextureInfo* AcquireDynamicGenerateTextureWithResolvePrefix(char*, int, const BuildArgSet&, bool, nn::gfx::Device*, const char*);
    virtual TextureInfo* AcquireDynamicGenerateTexture(char*, int, nn::gfx::Device*, const char*, const char*);
    virtual ShaderInfo* AcquireShader(nn::gfx::Device*, const char*) = 0;
    virtual ShaderInfo* AcquireArchiveShader(nn::gfx::Device*, u32, size_t, const u32*) = 0;
    virtual bool LoadTexture(ResourceTextureInfo*, nn::gfx::Device*, const char*) = 0;
    virtual nn::font::Font* LoadFont(nn::gfx::Device*, const char*);
    virtual bool LoadShader(ShaderInfo*, nn::gfx::Device*, const char*) = 0;
    virtual bool LoadArchiveShader(ShaderInfo*, nn::gfx::Device*, u32, size_t, const u32*) = 0;

    static const char* ArchiveShaderPrefix;
    static const char* ArchiveShaderSuffix;
};
}
