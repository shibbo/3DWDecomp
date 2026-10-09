#pragma once

#include <nn/ui2d/ui2d_ArcExtractor.h>
#include <nn/ui2d/ui2d_FontContainer.h>
#include <nn/ui2d/ui2d_ResourceAccessor.h>
#include <nn/ui2d/ui2d_ShaderContainer.h>
#include <nn/ui2d/ui2d_TextureContainer.h>
#include <nn/util/util_IntrusiveList.h>

namespace nn::gfx {
class ResTextureFile;
}  // namespace nn::gfx

namespace nn::ui2d {

/** @brief One archive attached to a resource accessor, with the resources loaded from it. */
class ArchiveHandle {
public:
    ArchiveHandle();
    virtual ~ArchiveHandle();

    bool Initialize(void* pArchiveStart, const char* pResourceRootDirectory,
                    nn::gfx::MemoryPool* pMemoryPool, ptrdiff_t memoryPoolOffset,
                    size_t memoryPoolSize);
    void Finalize(nn::gfx::Device* pDevice);
    void InitializeBntxIfNeeded(nn::gfx::Device* pDevice);
    const char* GetResRootDir() const;
    void LoadTextureAll(nn::gfx::Device* pDevice);
    void LoadShaderAll(nn::gfx::Device* pDevice);
    ArcExtractor* GetArcExtractor();
    bool LoadTexture(ResourceTextureInfo* pResTextureInfo, nn::gfx::Device* pDevice,
                     const char* pName);
    nn::font::Font* LoadFont(nn::gfx::Device* pDevice, const char* pName);
    bool LoadShader(ShaderInfo* pShaderInfo, nn::gfx::Device* pDevice, const char* pName);
    bool LoadArchiveShader(ShaderInfo* pShaderInfo, nn::gfx::Device* pDevice, u32 signature,
                           size_t keyCount, const u32* pKeys);
    const ArcExtractor* GetArcExtractor() const;
    FontContainer* GetFontList();
    TextureContainer* GetTextureList();
    ShaderContainer* GetShaderList();
    const void* RegisterFont(const char* pName, nn::font::Font* pFont);
    ResourceTextureInfo* RegisterTexture(const char* pName);
    ShaderInfo* RegisterShader(const char* pName);
    void UnregisterAll();
    void RegisterTextureViewToDescriptorPool(RegisterTextureView pRegisterTextureViewSlot,
                                             void* pUserData);
    void UnregisterTextureViewFromDescriptorPool(UnregisterTextureView pUnregisterTextureViewSlot,
                                                 void* pUserData);
    const void* GetArchiveDataStart() const;

    ArcExtractor mExtractor;
    char mRootDirectory[64];
    FontContainer mFonts;
    TextureContainer mTextures;
    ShaderContainer mShaders;
    nn::gfx::ResTextureFile* mTextureFile;
    void* mArchiveShader;
    void* mArchiveShaderVariationTable;
    void* mArchiveStart;
    nn::gfx::MemoryPool* mMemoryPool;
    ptrdiff_t mMemoryPoolOffset;
    size_t mMemoryPoolSize;
};

static_assert(sizeof(ArchiveHandle) == 0xe8, "ArchiveHandle size");

/** @brief Resource accessor reading resources from a single archive. */
class ArcResourceAccessor : public ResourceAccessor {
public:
    NN_RUNTIME_TYPEINFO(ResourceAccessor);

    ArcResourceAccessor();
    ~ArcResourceAccessor() override;

    void Finalize(nn::gfx::Device* pDevice) override;
    bool Attach(void* pArchiveStart, const char* pResourceRootDirectory,
                nn::gfx::MemoryPool* pMemoryPool, ptrdiff_t memoryPoolOffset,
                size_t memoryPoolSize);
    void* Detach();
    void* FindResourceByName(size_t* pSize, u32 resType, const char* pName) override;

    /**
     * @param pSize Receives the resource size; may be nullptr.
     * @param resType Resource type signature.
     * @param pName Resource name.
     * @return The resource, or nullptr when it is not found.
     */
    const void* FindResourceByName(size_t* pSize, u32 resType, const char* pName) const override {
        return const_cast<ArcResourceAccessor*>(this)->FindResourceByName(pSize, resType, pName);
    }

    /**
     * @param resType Resource type signature.
     * @param pName Resource name.
     * @return The resource, or nullptr when it is not found.
     */
    void* FindResourceByName(u32 resType, const char* pName) override {
        return FindResourceByName(nullptr, resType, pName);
    }

    /**
     * @param resType Resource type signature.
     * @param pName Resource name.
     * @return The resource, or nullptr when it is not found.
     */
    const void* FindResourceByName(u32 resType, const char* pName) const override {
        return FindResourceByName(nullptr, resType, pName);
    }

    void FindResourceByType(u32 resType, ResourceCallback pCallback,
                            void* pParam) const override;
    bool LoadTexture(ResourceTextureInfo* pResTextureInfo, nn::gfx::Device* pDevice,
                     const char* pName) override;
    nn::font::Font* LoadFont(nn::gfx::Device* pDevice, const char* pName) override;
    bool LoadShader(ShaderInfo* pShaderInfo, nn::gfx::Device* pDevice, const char* pName) override;
    bool LoadArchiveShader(ShaderInfo* pShaderInfo, nn::gfx::Device* pDevice, u32 signature,
                           size_t keyCount, const u32* pKeys) override;
    nn::font::Font* AcquireFont(nn::gfx::Device* pDevice, const char* pName) override;
    const void* RegisterFont(const char* pName, nn::font::Font* pFont);
    void UnregisterFont(const void* pFontRef);
    TextureInfo* AcquireTexture(nn::gfx::Device* pDevice, const char* pName) override;
    PlacementTextureInfo* RegisterTexture(const char* pName);
    void UnregisterTexture(TextureInfo* pTexture);
    ShaderInfo* AcquireShader(nn::gfx::Device* pDevice, const char* pName) override;
    ShaderInfo* AcquireArchiveShader(nn::gfx::Device* pDevice, u32 signature, size_t keyCount,
                                     const u32* pKeys) override;
    ShaderInfo* RegisterShader(const char* pName, bool isInitialized);
    void UnregisterShader(ShaderInfo* pShader);
    void RegisterTextureViewToDescriptorPool(RegisterTextureView pRegisterTextureViewSlot,
                                             void* pUserData) override;
    void UnregisterTextureViewFromDescriptorPool(UnregisterTextureView pUnregisterTextureViewSlot,
                                                 void* pUserData) override;
    TextureInfo* RegisterRenderTargetTexture(const char* pName) override;
    void UnregisterRenderTargetTexture(TextureInfo* pTexture) override;

protected:
    /**
     * @brief Hook run after an archive was attached successfully.
     * @return Whether the attach succeeded.
     */
    virtual bool SetupOnPostAttachSuccess_() { return true; }

public:
    ArchiveHandle m_ArcHandle;
    void* m_pArcBuf;
    FontContainer m_FontList;
    TextureContainer m_TextureList;
    ShaderContainer m_ShaderList;
    char m_ResRootDir[64];
};

static_assert(sizeof(ArcResourceAccessor) == 0x168, "ArcResourceAccessor size");

/** @brief Resource accessor searching several attached archives in attach order. */
class MultiArcResourceAccessor : public ResourceAccessor {
public:
    NN_RUNTIME_TYPEINFO(ResourceAccessor);

    /** @brief List node referencing one attached archive. */
    class ArcResourceLink {
    public:
        ArcResourceLink() : m_pArchiveHandle(nullptr) {}

        /** @param pArchiveHandle Archive referenced by this link. */
        void SetArchiveHandle(ArchiveHandle* pArchiveHandle) { m_pArchiveHandle = pArchiveHandle; }

        /** @return Archive referenced by this link. */
        ArchiveHandle* GetArchiveHandle() const { return m_pArchiveHandle; }

        nn::util::IntrusiveListNode m_Link;
        ArchiveHandle* m_pArchiveHandle;
    };

    using ArcResourceList = nn::util::IntrusiveList<
        ArcResourceLink,
        nn::util::IntrusiveListMemberNodeTraits<ArcResourceLink, &ArcResourceLink::m_Link>>;

    MultiArcResourceAccessor();
    ~MultiArcResourceAccessor() override;

    void Finalize(nn::gfx::Device* pDevice) override;
    void Attach(ArchiveHandle* pArchiveHandle);
    void Detach(const ArchiveHandle* pArchiveHandle);
    void DetachAll();
    void* FindResourceByName(size_t* pSize, u32 resType, const char* pName) override;

    /**
     * @param pSize Receives the resource size; may be nullptr.
     * @param resType Resource type signature.
     * @param pName Resource name.
     * @return The resource, or nullptr when it is not found.
     */
    const void* FindResourceByName(size_t* pSize, u32 resType, const char* pName) const override {
        return const_cast<MultiArcResourceAccessor*>(this)->FindResourceByName(pSize, resType,
                                                                               pName);
    }

    /**
     * @param resType Resource type signature.
     * @param pName Resource name.
     * @return The resource, or nullptr when it is not found.
     */
    void* FindResourceByName(u32 resType, const char* pName) override {
        return FindResourceByName(nullptr, resType, pName);
    }

    /**
     * @param resType Resource type signature.
     * @param pName Resource name.
     * @return The resource, or nullptr when it is not found.
     */
    const void* FindResourceByName(u32 resType, const char* pName) const override {
        return FindResourceByName(nullptr, resType, pName);
    }

    void FindResourceByType(u32 resType, ResourceCallback pCallback,
                            void* pParam) const override;
    nn::font::Font* AcquireFont(nn::gfx::Device* pDevice, const char* pName) override;
    ArchiveHandle* FindFontArchive(const char* pName);
    const void* RegisterFont(const char* pName, nn::font::Font* pFont);
    void UnregisterFont(const void* pFontRef);
    TextureInfo* AcquireTexture(nn::gfx::Device* pDevice, const char* pName) override;
    ArchiveHandle* FindTextureArchive(const char* pName);
    bool LoadTexture(ResourceTextureInfo* pResTextureInfo, nn::gfx::Device* pDevice,
                     const char* pName) override;
    bool LoadShader(ShaderInfo* pShaderInfo, nn::gfx::Device* pDevice, const char* pName) override;
    bool LoadArchiveShader(ShaderInfo* pShaderInfo, nn::gfx::Device* pDevice, u32 signature,
                           size_t keyCount, const u32* pKeys) override;
    PlacementTextureInfo* RegisterTexture(const char* pName);
    void UnregisterTexture(TextureInfo* pTexture);
    ShaderInfo* AcquireShader(nn::gfx::Device* pDevice, const char* pName) override;
    ArchiveHandle* FindShaderArchive(const char* pName);
    ShaderInfo* AcquireArchiveShader(nn::gfx::Device* pDevice, u32 signature, size_t keyCount,
                                     const u32* pKeys) override;
    ArchiveHandle* FindArchiveShaderArchive(u32 signature, size_t keyCount, const u32* pKeys);
    ShaderInfo* RegisterShader(const char* pName);
    void UnregisterShader(ShaderInfo* pShader);
    void RegisterTextureViewToDescriptorPool(RegisterTextureView pRegisterTextureViewSlot,
                                             void* pUserData) override;
    void UnregisterTextureViewFromDescriptorPool(UnregisterTextureView pUnregisterTextureViewSlot,
                                                 void* pUserData) override;
    TextureInfo* RegisterRenderTargetTexture(const char* pName) override;
    void UnregisterRenderTargetTexture(TextureInfo* pTexture) override;

    ArcResourceList m_ArcList;
    FontContainer m_FontList;
    TextureContainer m_TextureList;
    ShaderContainer m_ShaderList;
};

static_assert(sizeof(MultiArcResourceAccessor) == 0x48, "MultiArcResourceAccessor size");

}  // namespace nn::ui2d
