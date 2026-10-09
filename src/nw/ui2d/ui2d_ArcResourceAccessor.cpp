#include <nn/ui2d/ui2d_ArcResourceAccessor.h>

#include <cstring>
#include <nn/font/font_ResFont.h>
#include <nn/gfx/detail/gfx_MemoryPool-api.nvn.8.h>
#include <nn/gfx/gfx_MemoryPoolInfo.h>
#include <nn/gfx/gfx_ResTexture.h>
#include <nn/gfx/gfx_ResTextureData.h>
#include <nn/ui2d/ui2d_Layout.h>
#include <nn/ui2d/ui2d_RenderTargetTextureInfo.h>
#include <nn/ui2d/ui2d_Util.h>
#include <nn/ui2d/ui2d_VectorGraphics.h>
#include <nn/util/util_BytePtr.h>
#include <nn/util/util_ResDic.h>
#include <nn/util/util_StringUtil.h>
#include <nn/util/util_StringView.h>

namespace nn::ui2d {

namespace {

using MemoryPoolImpl = nn::gfx::detail::MemoryPoolImpl<nn::gfx::ApiVariationNvn8>;
using DeviceImpl = nn::gfx::detail::DeviceImpl<nn::gfx::ApiVariationNvn8>;

const u32 ResourceTypeTexture = 0x74696d67;  // 'timg'
const u32 ResourceTypeFont = 0x666f6e74;     // 'font'
const u32 ResourceTypeShader = 0x62677368;   // 'bgsh'

const int ArchivePathMax = 256;
const int ShaderNameMax = 8;

constexpr const char* CombinedTextureFileName = "__Combined.bntx";
constexpr const char* ArchiveShaderFileName = "__ArchiveShader.bnsh";
constexpr const char* ArchiveShaderVariationTableFileName = "__ArchiveShader.bushvt";
constexpr const char* ArchiveShaderRegisterName = "__arcsh";

/**
 * @param pFileName Shader file name relative to the shader directory.
 * @return Whether pFileName is the archive shader binary.
 */
inline bool IsArchiveShaderFile(const char* pFileName) {
    return std::strcmp(pFileName, ArchiveShaderFileName) == 0;
}

/**
 * @brief Writes a resource type signature as a four character string.
 * @param pOut Receives the five byte, null terminated string.
 * @param resType Resource type signature.
 */
inline void MakeResourceTypeString(char* pOut, u32 resType) {
    pOut[0] = static_cast<char>(resType >> 24);
    pOut[1] = static_cast<char>(resType >> 16);
    pOut[2] = static_cast<char>(resType >> 8);
    pOut[3] = static_cast<char>(resType);
    pOut[4] = '\0';
}

/**
 * @param pRootDirectory Resource root directory of an archive.
 * @return Whether pRootDirectory is the archive root itself (".").
 */
inline bool IsArchiveRoot(const char* pRootDirectory) {
    return pRootDirectory[0] == '.' && pRootDirectory[1] == '\0';
}

/**
 * @brief Looks up the archive entry of a resource.
 * @param pArchiveHandle Archive to search.
 * @param pRootDirectory Resource root directory inside the archive.
 * @param resType Resource type signature, used as the type directory name.
 * @param pName Resource file name.
 * @return Entry id of the resource, or -1 when it is not found.
 */
inline int FindResourceEntryId(ArchiveHandle* pArchiveHandle, const char* pRootDirectory,
                               u32 resType, const char* pName) {
    char resTypeString[5];
    MakeResourceTypeString(resTypeString, resType);
    char path[ArchivePathMax];

    if (IsArchiveRoot(pRootDirectory)) {
        nn::util::SNPrintf(path, sizeof(path) - 1, "%s/%s", resTypeString, pName);
    } else {
        nn::util::SNPrintf(path, sizeof(path) - 1, "%s/%s/%s", pRootDirectory, resTypeString,
                           pName);
    }

    return pArchiveHandle->GetArcExtractor()->ConvertPathToEntryId(path);
}

/**
 * @brief Makes the path of the directory holding every resource of one type.
 * @param pDirectory Receives the path; ArchivePathMax bytes.
 * @param pRootDirectory Resource root directory inside the archive.
 * @param resType Resource type signature, used as the type directory name.
 */
inline void MakeResourceTypeDirectory(char* pDirectory, const char* pRootDirectory, u32 resType) {
    char resTypeString[5];
    MakeResourceTypeString(resTypeString, resType);

    if (IsArchiveRoot(pRootDirectory)) {
        nn::util::SNPrintf(pDirectory, ArchivePathMax - 1, "%s/", resTypeString);
    } else {
        nn::util::SNPrintf(pDirectory, ArchivePathMax - 1, "%s/%s/", pRootDirectory,
                           resTypeString);
    }
}

/**
 * @brief Looks up a resource in an archive.
 * @param pArchiveHandle Archive to search.
 * @param pRootDirectory Resource root directory inside the archive.
 * @param resType Resource type signature, used as the type directory name.
 * @param pName Resource file name.
 * @param pSize Receives the resource size; may be nullptr.
 * @return The resource, or nullptr when it is not found.
 */
void* GetResourceSub(ArchiveHandle* pArchiveHandle, const char* pRootDirectory, u32 resType,
                     const char* pName, size_t* pSize) {
    const int entryId = FindResourceEntryId(pArchiveHandle, pRootDirectory, resType, pName);

    if (entryId == -1) {
        return nullptr;
    }

    ArcFileInfo fileInfo;
    void* pResource = pArchiveHandle->GetArcExtractor()->GetFileFast(&fileInfo, entryId);

    if (pSize != nullptr) {
        *pSize = fileInfo.GetLength();
    }

    return pResource;
}

/**
 * @brief Calls a callback for every resource of one type in an archive.
 * @param pArchiveHandle Archive to search.
 * @param pRootDirectory Resource root directory inside the archive.
 * @param resType Resource type signature, used as the type directory name.
 * @param pCallback Called with each resource, its size, its path and pParam.
 * @param pParam User data passed to pCallback.
 */
void FindResourceByTypeSub(ArchiveHandle* pArchiveHandle, const char* pRootDirectory, u32 resType,
                           ResourceCallback pCallback, void* pParam) {
    char directory[ArchivePathMax];
    MakeResourceTypeDirectory(directory, pRootDirectory, resType);
    ArcExtractor* pExtractor = pArchiveHandle->GetArcExtractor();
    int handle = 0;
    int entryId = 0;

    while (true) {
        ArcEntry entry;

        if (pExtractor->ReadEntry(&handle, &entry, 1) == 0) {
            break;
        }

        if (std::strncmp(entry.name, directory, std::strlen(directory)) == 0) {
            ArcFileInfo fileInfo;
            void* pResource = pExtractor->GetFileFast(&fileInfo, entryId);
            pCallback(pResource, fileInfo.GetLength(), entry.name, pParam);
        }

        entryId = handle;
    }
}

/**
 * @param pExtractor Archive to search.
 * @param pPath Path of the file inside the archive.
 * @return The file, or nullptr when it is not found.
 */
inline void* GetFile(ArcExtractor* pExtractor, const char* pPath) {
    const int entryId = pExtractor->ConvertPathToEntryId(pPath);
    return entryId >= 0 ? pExtractor->GetFileFast(nullptr, entryId) : nullptr;
}

/**
 * @param pTextureFile Texture file.
 * @return Whether the internal memory pool of pTextureFile is initialized.
 */
inline bool IsTextureMemoryPoolInitialized(nn::gfx::ResTextureFile* pTextureFile) {
    auto* pPool = static_cast<MemoryPoolImpl*>(
        pTextureFile->ToData().textureContainerData.pTextureMemoryPool.Get());
    return pPool->ToData()->state == MemoryPoolImpl::DataType::State_Initialized;
}

/**
 * @brief Binds the texture data of a texture file to a memory pool holding the file.
 * @param pTextureFile Texture file.
 * @param pMemoryPool Memory pool holding the file.
 * @param memoryPoolOffset Offset of the file in pMemoryPool.
 */
inline void InitializeTextureFile(nn::gfx::ResTextureFile* pTextureFile,
                                  nn::gfx::MemoryPool* pMemoryPool, ptrdiff_t memoryPoolOffset) {
    nn::gfx::ResTextureContainerData& rContainer = pTextureFile->ToData().textureContainerData;
    rContainer.pCurrentMemoryPool.Set(pMemoryPool);
    rContainer.memoryPoolOffsetBase = static_cast<uint32_t>(
        memoryPoolOffset + nn::util::BytePtr(pTextureFile).Distance(rContainer.pTextureData.Get()) +
        sizeof(nn::util::BinaryBlockHeader));
}

/**
 * @brief Binds the texture data of a texture file to its internal memory pool.
 * @param pTextureFile Texture file.
 * @param pDevice Device the internal memory pool is created on.
 */
inline void InitializeTextureFile(nn::gfx::ResTextureFile* pTextureFile,
                                  nn::gfx::Device* pDevice) {
    nn::gfx::ResTextureContainerData& rContainer = pTextureFile->ToData().textureContainerData;
    nn::gfx::MemoryPoolInfo info;
    std::memset(&info, 0, sizeof(info));
    info.SetMemoryPoolProperty(nn::gfx::MemoryPoolProperty_CpuInvisible |
                               nn::gfx::MemoryPoolProperty_GpuCached);
    auto* pBlock = static_cast<nn::util::BinaryBlockHeader*>(rContainer.pTextureData.Get());
    info.SetPoolMemory(nn::util::BytePtr(pBlock, sizeof(nn::util::BinaryBlockHeader)).Get(),
                       pBlock->GetBlockSize() - sizeof(nn::util::BinaryBlockHeader));
    static_cast<MemoryPoolImpl*>(rContainer.pTextureMemoryPool.Get())
        ->Initialize(reinterpret_cast<DeviceImpl*>(pDevice), info);
    rContainer.pCurrentMemoryPool.Set(rContainer.pTextureMemoryPool.Get());
    rContainer.memoryPoolOffsetBase = 0;
}

}  // namespace

/** @brief Constructs an archive handle with no archive attached. */
ArchiveHandle::ArchiveHandle()
    : mTextureFile(nullptr), mArchiveShader(nullptr), mArchiveShaderVariationTable(nullptr),
      mArchiveStart(nullptr), mMemoryPool(nullptr), mMemoryPoolOffset(0), mMemoryPoolSize(0) {}

/** @brief Destroys the archive handle. */
ArchiveHandle::~ArchiveHandle() = default;

/**
 * @brief Attaches an archive.
 * @param pArchiveStart Start of the archive data.
 * @param pResourceRootDirectory Resource root directory inside the archive.
 * @param pMemoryPool Memory pool holding the archive, or nullptr.
 * @param memoryPoolOffset Offset of the archive in pMemoryPool.
 * @param memoryPoolSize Size of the archive's region in pMemoryPool.
 * @return Whether the archive could be prepared.
 */
bool ArchiveHandle::Initialize(void* pArchiveStart, const char* pResourceRootDirectory,
                               nn::gfx::MemoryPool* pMemoryPool, ptrdiff_t memoryPoolOffset,
                               size_t memoryPoolSize) {
    if (!mExtractor.PrepareArchive(pArchiveStart)) {
        return false;
    }

    nn::util::Strlcpy(mRootDirectory, pResourceRootDirectory, sizeof(mRootDirectory));
    mArchiveStart = pArchiveStart;
    mMemoryPool = pMemoryPool;
    mMemoryPoolOffset = memoryPoolOffset;
    mMemoryPoolSize = memoryPoolSize;
    return true;
}

/**
 * @brief Releases the resources loaded from the archive.
 * @param pDevice Device the resources were created on.
 */
void ArchiveHandle::Finalize(nn::gfx::Device* pDevice) {
    mArchiveShader = nullptr;
    mArchiveShaderVariationTable = nullptr;
    mFonts.Finalize(pDevice);
    mTextures.Finalize(pDevice);
    mShaders.Finalize(pDevice);

    if (mTextureFile != nullptr) {
        nn::gfx::ResTextureContainerData& rContainer = mTextureFile->ToData().textureContainerData;

        if (rContainer.pCurrentMemoryPool.Get() == rContainer.pTextureMemoryPool.Get()) {
            static_cast<MemoryPoolImpl*>(rContainer.pCurrentMemoryPool.Get())
                ->Finalize(reinterpret_cast<DeviceImpl*>(pDevice));
        }

        rContainer.pCurrentMemoryPool.Set(nullptr);
        mTextureFile = nullptr;
    }
}

/**
 * @brief Sets up the combined texture file of the archive the first time it is needed.
 * @param pDevice Device the texture memory pool is created on.
 */
void ArchiveHandle::InitializeBntxIfNeeded(nn::gfx::Device* pDevice) {
    if (mTextureFile != nullptr) {
        return;
    }

    void* pBntx =
        GetResourceSub(this, GetResRootDir(), ResourceTypeTexture, CombinedTextureFileName, nullptr);

    if (pBntx == nullptr) {
        return;
    }

    mTextureFile = nn::gfx::ResTextureFile::ResCast(pBntx);

    if (mTextureFile != nullptr && IsTextureMemoryPoolInitialized(mTextureFile)) {
        return;
    }

    if (mMemoryPool != nullptr) {
        InitializeTextureFile(mTextureFile, mMemoryPool,
                              nn::util::BytePtr(mArchiveStart).Distance(mTextureFile) +
                                  mMemoryPoolOffset);
    } else {
        InitializeTextureFile(mTextureFile, pDevice);
    }
}

/** @return Resource root directory inside the archive. */
const char* ArchiveHandle::GetResRootDir() const {
    return mRootDirectory;
}

/**
 * @brief Registers and loads every texture of the combined texture file.
 * @param pDevice Device the textures are created on.
 */
void ArchiveHandle::LoadTextureAll(nn::gfx::Device* pDevice) {
    InitializeBntxIfNeeded(pDevice);

    if (mTextureFile == nullptr) {
        return;
    }

    const int textureCount =
        mTextureFile->ToData().textureContainerData.pTextureDic.Get()->GetCount();

    for (int i = 0; i < textureCount; i++) {
        nn::gfx::ResTexture* pResTexture =
            mTextureFile->ToData().textureContainerData.pTexturePtrArray.Get()[i].Get();

        if (pResTexture == nullptr) {
            break;
        }

        ResourceTextureInfo* pTextureInfo =
            mTextures.RegisterResourceTexture(pResTexture->ToData().pName.Get()->GetData());
        nn::ui2d::LoadTexture(pTextureInfo, pDevice, pResTexture);
    }
}

/**
 * @brief Registers and loads every shader of the archive's shader directory.
 * @param pDevice Device the shaders are created on.
 */
void ArchiveHandle::LoadShaderAll(nn::gfx::Device* pDevice) {
    char shaderDirectory[] = "bgsh/";
    int handle = 0;
    ArcEntry entry;

    while (mExtractor.ReadEntry(&handle, &entry, 1) != 0) {
        const char* pFound = std::strstr(entry.name, shaderDirectory);

        if (pFound == nullptr) {
            continue;
        }

        char fileName[128];
        nn::util::Strlcpy(fileName, pFound + std::strlen(shaderDirectory), sizeof(fileName));
        const char* pPrefix = ResourceAccessor::ArchiveShaderPrefix;
        const char* pNameStart = std::strstr(fileName, pPrefix);

        if (pNameStart == nullptr) {
            continue;
        }

        pNameStart += std::strlen(pPrefix);
        const char* pNameEnd = std::strstr(pNameStart, ResourceAccessor::ArchiveShaderSuffix);

        if (pNameEnd == nullptr) {
            continue;
        }

        if (!IsArchiveShaderFile(fileName)) {
            pNameStart++;
        }

        char shaderName[ShaderNameMax] = {};
        nn::util::Strlcpy(shaderName, pNameStart, pNameEnd - pNameStart + 1);
        void* pShader = GetFile(&mExtractor, entry.name);
        const nn::util::BytePtr archiveStart(mArchiveStart);
        ShaderInfo* pShaderInfo;
        const void* pVariationTable;

        if (!IsArchiveShaderFile(fileName)) {
            pShaderInfo = mShaders.RegisterShader(shaderName, true);
            pVariationTable = nullptr;
        } else {
            std::strcpy(fileName, shaderDirectory);
            std::strncat(fileName, ArchiveShaderVariationTableFileName,
                         sizeof(fileName) - std::strlen(fileName) - 1);
            pVariationTable = GetFile(&mExtractor, fileName);
            pShaderInfo = mShaders.RegisterShader(ArchiveShaderRegisterName, true);
        }

        nn::ui2d::LoadArchiveShader(pShaderInfo, pDevice, pShader, pVariationTable, mMemoryPool,
                                    archiveStart.Distance(pShader) + mMemoryPoolOffset,
                                    mMemoryPoolSize, 3);
    }
}

/** @return Extractor reading the archive. */
ArcExtractor* ArchiveHandle::GetArcExtractor() {
    return &mExtractor;
}

/**
 * @brief Loads a texture from the combined texture file of the archive.
 * @param pResTextureInfo Receives the texture.
 * @param pDevice Device the texture is created on.
 * @param pName Texture name.
 * @return Whether the texture was loaded.
 */
bool ArchiveHandle::LoadTexture(ResourceTextureInfo* pResTextureInfo, nn::gfx::Device* pDevice,
                                const char* pName) {
    InitializeBntxIfNeeded(pDevice);

    if (mTextureFile != nullptr) {
        nn::gfx::ResTextureContainerData& rContainer = mTextureFile->ToData().textureContainerData;
        const int index = rContainer.pTextureDic.Get()->FindIndex(pName);

        if (index != -1) {
            nn::gfx::ResTexture* pResTexture = rContainer.pTexturePtrArray.Get()[index].Get();

            if (pResTexture != nullptr) {
                return nn::ui2d::LoadTexture(pResTextureInfo, pDevice, pResTexture);
            }
        }
    }

    GetResourceSub(this, GetResRootDir(), ResourceTypeTexture, pName, nullptr);
    return false;
}

/**
 * @brief Creates a font from a font resource of the archive.
 * @param pDevice Device the font is created on.
 * @param pName Font resource name.
 * @return The new font, or nullptr when it could not be created.
 */
nn::font::Font* ArchiveHandle::LoadFont(nn::gfx::Device* pDevice, const char* pName) {
    void* pResource = GetResourceSub(this, GetResRootDir(), ResourceTypeFont, pName, nullptr);

    if (pResource == nullptr) {
        return nullptr;
    }

    nn::font::ResFont* pFont = Layout::NewObj<nn::font::ResFont>();

    if (pFont == nullptr) {
        return nullptr;
    }

    if (!pFont->SetResource(pDevice, pResource, mMemoryPool,
                            nn::util::BytePtr(mArchiveStart).Distance(pResource) +
                                mMemoryPoolOffset,
                            mMemoryPoolSize)) {
        Layout::DeleteObj(pFont);
        return nullptr;
    }

    return pFont;
}

/**
 * @brief Loads a shader from the archive.
 * @param pShaderInfo Receives the shader.
 * @param pDevice Device the shader is created on.
 * @param pName Shader name.
 * @return Whether the shader was loaded.
 */
bool ArchiveHandle::LoadShader(ShaderInfo* pShaderInfo, nn::gfx::Device* pDevice,
                               const char* pName) {
    char fileName[ArchivePathMax];
    nn::util::SNPrintf(fileName, sizeof(fileName), "ArchiveShader-%s.bnsh", pName);
    void* pShader = GetResourceSub(this, GetResRootDir(), ResourceTypeShader, fileName, nullptr);

    if (pShader == nullptr) {
        return false;
    }

    nn::ui2d::LoadArchiveShader(pShaderInfo, pDevice, pShader, nullptr, nullptr, 0, 0, 3);
    return true;
}

/**
 * @brief Loads the archive shader variation matching a set of keys.
 * @param pShaderInfo Receives the shader.
 * @param pDevice Device the shader is created on.
 * @param signature Shader signature.
 * @param keyCount Number of keys.
 * @param pKeys Keys selecting the variation.
 * @return Whether the shader was loaded.
 */
bool ArchiveHandle::LoadArchiveShader(ShaderInfo* pShaderInfo, nn::gfx::Device* pDevice,
                                      u32 signature, size_t keyCount, const u32* pKeys) {
    void* pArchiveShader = mArchiveShader;

    if (pArchiveShader == nullptr) {
        pArchiveShader = GetResourceSub(this, GetResRootDir(), ResourceTypeShader,
                                        ArchiveShaderFileName, nullptr);
        mArchiveShader = pArchiveShader;

        if (pArchiveShader == nullptr) {
            return false;
        }
    }

    void* pVariationTable = mArchiveShaderVariationTable;

    if (pVariationTable == nullptr) {
        pVariationTable = GetResourceSub(this, GetResRootDir(), ResourceTypeShader,
                                         ArchiveShaderVariationTableFileName, nullptr);
        mArchiveShaderVariationTable = pVariationTable;

        if (pVariationTable == nullptr) {
            return false;
        }
    }

    if (SearchShaderVariationIndexFromTable(pVariationTable, signature, keyCount, pKeys) == -1) {
        return false;
    }

    nn::ui2d::LoadArchiveShader(
        pShaderInfo, pDevice, pArchiveShader, pVariationTable, mMemoryPool,
        nn::util::BytePtr(mArchiveStart).Distance(pArchiveShader) + mMemoryPoolOffset,
        mMemoryPoolSize, 3);
    return true;
}

/** @return Extractor reading the archive. */
const ArcExtractor* ArchiveHandle::GetArcExtractor() const {
    return &mExtractor;
}

/** @return Fonts loaded from the archive. */
FontContainer* ArchiveHandle::GetFontList() {
    return &mFonts;
}

/** @return Textures loaded from the archive. */
TextureContainer* ArchiveHandle::GetTextureList() {
    return &mTextures;
}

/** @return Shaders loaded from the archive. */
ShaderContainer* ArchiveHandle::GetShaderList() {
    return &mShaders;
}

/**
 * @brief Registers a font owned by the archive.
 * @param pName Font name.
 * @param pFont Font to register.
 * @return Handle of the registration.
 */
const void* ArchiveHandle::RegisterFont(const char* pName, nn::font::Font* pFont) {
    return mFonts.RegisterFont(pName, pFont, true);
}

/**
 * @brief Registers a texture owned by the archive.
 * @param pName Texture name.
 * @return The new texture.
 */
ResourceTextureInfo* ArchiveHandle::RegisterTexture(const char* pName) {
    return mTextures.RegisterResourceTexture(pName);
}

/**
 * @brief Registers a shader owned by the archive.
 * @param pName Shader name.
 * @return The new shader.
 */
ShaderInfo* ArchiveHandle::RegisterShader(const char* pName) {
    return mShaders.RegisterShader(pName, true);
}

/** @brief Removes every registered font, texture and shader without releasing them. */
void ArchiveHandle::UnregisterAll() {
    GetFontList()->mFonts.clear();
    GetTextureList()->mTextures.clear();
    GetShaderList()->mShaders.clear();
}

/**
 * @brief Allocates descriptor slots for the font and texture views of the archive.
 * @param pRegisterTextureViewSlot Allocates one descriptor slot.
 * @param pUserData User data passed to pRegisterTextureViewSlot.
 */
void ArchiveHandle::RegisterTextureViewToDescriptorPool(RegisterTextureView pRegisterTextureViewSlot,
                                                        void* pUserData) {
    mFonts.RegisterTextureViewToDescriptorPool(pRegisterTextureViewSlot, pUserData);
    mTextures.RegisterTextureViewToDescriptorPool(pRegisterTextureViewSlot, pUserData);
}

/**
 * @brief Releases the descriptor slots of the font and texture views of the archive.
 * @param pUnregisterTextureViewSlot Releases one descriptor slot.
 * @param pUserData User data passed to pUnregisterTextureViewSlot.
 */
void ArchiveHandle::UnregisterTextureViewFromDescriptorPool(
    UnregisterTextureView pUnregisterTextureViewSlot, void* pUserData) {
    mFonts.UnregisterTextureViewFromDescriptorPool(pUnregisterTextureViewSlot, pUserData);
    mTextures.UnregisterTextureViewFromDescriptorPool(pUnregisterTextureViewSlot, pUserData);
}

/** @return Start of the archive data. */
const void* ArchiveHandle::GetArchiveDataStart() const {
    return mExtractor.m_pArchiveBlockHeader;
}

/** @brief Constructs an accessor with no archive attached. */
ArcResourceAccessor::ArcResourceAccessor() : m_pArcBuf(nullptr) {
    m_ResRootDir[0] = '\0';
}

/** @brief Destroys the accessor. */
ArcResourceAccessor::~ArcResourceAccessor() = default;

/**
 * @brief Releases every resource loaded or registered through the accessor.
 * @param pDevice Device the resources were created on.
 */
void ArcResourceAccessor::Finalize(nn::gfx::Device* pDevice) {
    m_ArcHandle.Finalize(pDevice);
    m_FontList.Finalize(pDevice);
    m_TextureList.Finalize(pDevice);
    m_ShaderList.Finalize(pDevice);
    ResourceAccessor::Finalize(pDevice);
}

/**
 * @brief Attaches an archive.
 * @param pArchiveStart Start of the archive data.
 * @param pResourceRootDirectory Resource root directory inside the archive.
 * @param pMemoryPool Memory pool holding the archive, or nullptr.
 * @param memoryPoolOffset Offset of the archive in pMemoryPool.
 * @param memoryPoolSize Size of the archive's region in pMemoryPool.
 * @return Whether the archive was attached.
 */
bool ArcResourceAccessor::Attach(void* pArchiveStart, const char* pResourceRootDirectory,
                                 nn::gfx::MemoryPool* pMemoryPool, ptrdiff_t memoryPoolOffset,
                                 size_t memoryPoolSize) {
    if (!m_ArcHandle.Initialize(pArchiveStart, pResourceRootDirectory, pMemoryPool,
                                memoryPoolOffset, memoryPoolSize)) {
        return false;
    }

    m_pArcBuf = pArchiveStart;
    nn::util::Strlcpy(m_ResRootDir, pResourceRootDirectory, sizeof(m_ResRootDir));
    return SetupOnPostAttachSuccess_();
}

/**
 * @brief Detaches the archive.
 * @return Start of the detached archive data.
 */
void* ArcResourceAccessor::Detach() {
    void* pArcBuf = m_pArcBuf;
    m_pArcBuf = nullptr;
    return pArcBuf;
}

/**
 * @param pSize Receives the resource size; may be nullptr.
 * @param resType Resource type signature.
 * @param pName Resource name.
 * @return The resource, or nullptr when it is not found.
 */
void* ArcResourceAccessor::FindResourceByName(size_t* pSize, u32 resType, const char* pName) {
    return GetResourceSub(&m_ArcHandle, m_ResRootDir, resType, pName, pSize);
}

/**
 * @brief Calls a callback for every resource of one type.
 * @param resType Resource type signature.
 * @param pCallback Called with each resource, its size, its path and pParam.
 * @param pParam User data passed to pCallback.
 */
void ArcResourceAccessor::FindResourceByType(u32 resType, ResourceCallback pCallback,
                                             void* pParam) const {
    FindResourceByTypeSub(const_cast<ArchiveHandle*>(&m_ArcHandle), m_ResRootDir, resType,
                          pCallback, pParam);
}

/**
 * @param pResTextureInfo Receives the texture.
 * @param pDevice Device the texture is created on.
 * @param pName Texture name.
 * @return Whether the texture was loaded.
 */
bool ArcResourceAccessor::LoadTexture(ResourceTextureInfo* pResTextureInfo,
                                      nn::gfx::Device* pDevice, const char* pName) {
    return m_ArcHandle.LoadTexture(pResTextureInfo, pDevice, pName);
}

/**
 * @param pDevice Device the font is created on.
 * @param pName Font resource name.
 * @return The new font, or nullptr when it could not be created.
 */
nn::font::Font* ArcResourceAccessor::LoadFont(nn::gfx::Device* pDevice, const char* pName) {
    return m_ArcHandle.LoadFont(pDevice, pName);
}

/**
 * @param pShaderInfo Receives the shader.
 * @param pDevice Device the shader is created on.
 * @param pName Shader name.
 * @return Whether the shader was loaded.
 */
bool ArcResourceAccessor::LoadShader(ShaderInfo* pShaderInfo, nn::gfx::Device* pDevice,
                                     const char* pName) {
    return m_ArcHandle.LoadShader(pShaderInfo, pDevice, pName);
}

/**
 * @param pShaderInfo Receives the shader.
 * @param pDevice Device the shader is created on.
 * @param signature Shader signature.
 * @param keyCount Number of keys.
 * @param pKeys Keys selecting the variation.
 * @return Whether the shader was loaded.
 */
bool ArcResourceAccessor::LoadArchiveShader(ShaderInfo* pShaderInfo, nn::gfx::Device* pDevice,
                                            u32 signature, size_t keyCount, const u32* pKeys) {
    return m_ArcHandle.LoadArchiveShader(pShaderInfo, pDevice, signature, keyCount, pKeys);
}

/**
 * @brief Finds a font, loading it from the archive if needed.
 * @param pDevice Device the font is created on.
 * @param pName Font name.
 * @return The font, or nullptr when it is not found.
 */
nn::font::Font* ArcResourceAccessor::AcquireFont(nn::gfx::Device* pDevice, const char* pName) {
    nn::font::Font* pFont = m_FontList.FindFontByName(pName);

    if (pFont != nullptr) {
        return pFont;
    }

    pFont = m_ArcHandle.GetFontList()->FindFontByName(pName);

    if (pFont != nullptr) {
        return pFont;
    }

    pFont = LoadFont(pDevice, pName);

    if (pFont != nullptr) {
        m_ArcHandle.RegisterFont(pName, pFont);
    }

    return pFont;
}

/**
 * @brief Registers a font owned by the caller.
 * @param pName Font name.
 * @param pFont Font to register.
 * @return Handle of the registration.
 */
const void* ArcResourceAccessor::RegisterFont(const char* pName, nn::font::Font* pFont) {
    return m_FontList.RegisterFont(pName, pFont, false);
}

/** @param pFontRef Handle returned by RegisterFont. */
void ArcResourceAccessor::UnregisterFont(const void* pFontRef) {
    m_FontList.UnregisterFont(pFontRef);
}

/**
 * @brief Finds a texture, loading it from the archive if needed.
 * @param pDevice Device the texture is created on.
 * @param pName Texture name.
 * @return The texture, or nullptr when it could not be registered.
 */
TextureInfo* ArcResourceAccessor::AcquireTexture(nn::gfx::Device* pDevice, const char* pName) {
    TextureInfo* pTexture = m_TextureList.FindTextureByName(pName);

    if (pTexture != nullptr) {
        return pTexture;
    }

    pTexture = m_ArcHandle.GetTextureList()->FindTextureByName(pName);

    if (pTexture != nullptr) {
        return pTexture;
    }

    ResourceTextureInfo* pResTexture = m_ArcHandle.RegisterTexture(pName);

    if (pResTexture != nullptr) {
        LoadTexture(pResTexture, pDevice, pName);
        // The validity is only checked; a texture that failed to load is still returned.
        pResTexture->IsValid();
    }

    return pResTexture;
}

/**
 * @brief Registers a texture owned by the caller.
 * @param pName Texture name.
 * @return The new texture.
 */
PlacementTextureInfo* ArcResourceAccessor::RegisterTexture(const char* pName) {
    return m_TextureList.RegisterPlacementTexture(pName, false);
}

/** @param pTexture Texture returned by RegisterTexture. */
void ArcResourceAccessor::UnregisterTexture(TextureInfo* pTexture) {
    m_TextureList.UnregisterTexture(pTexture);
}

/**
 * @brief Finds a shader, loading it from the archive if needed.
 * @param pDevice Device the shader is created on.
 * @param pName Shader name.
 * @return The shader, or nullptr when it could not be registered.
 */
ShaderInfo* ArcResourceAccessor::AcquireShader(nn::gfx::Device* pDevice, const char* pName) {
    ShaderInfo* pShader = m_ShaderList.FindShaderByName(pName);

    if (pShader != nullptr) {
        return pShader;
    }

    pShader = m_ArcHandle.GetShaderList()->FindShaderByName(pName);

    if (pShader != nullptr) {
        return pShader;
    }

    pShader = m_ArcHandle.RegisterShader(pName);

    if (pShader != nullptr) {
        LoadShader(pShader, pDevice, pName);
    }

    return pShader;
}

/**
 * @brief Finds the archive shader, loading it from the archive if needed.
 * @param pDevice Device the shader is created on.
 * @param signature Shader signature.
 * @param keyCount Number of keys.
 * @param pKeys Keys selecting the variation.
 * @return The shader, or nullptr when it could not be registered.
 */
ShaderInfo* ArcResourceAccessor::AcquireArchiveShader(nn::gfx::Device* pDevice, u32 signature,
                                                      size_t keyCount, const u32* pKeys) {
    ShaderInfo* pShader = m_ShaderList.FindShaderByName(ArchiveShaderRegisterName);

    if (pShader != nullptr) {
        return pShader;
    }

    pShader = m_ArcHandle.GetShaderList()->FindShaderByName(ArchiveShaderRegisterName);

    if (pShader != nullptr) {
        return pShader;
    }

    pShader = m_ArcHandle.RegisterShader(ArchiveShaderRegisterName);

    if (pShader != nullptr) {
        LoadArchiveShader(pShader, pDevice, signature, keyCount, pKeys);
    }

    return pShader;
}

/**
 * @brief Registers a shader owned by the caller.
 * @param pName Shader name.
 * @param isInitialized Whether the shader shares archive data.
 * @return The new shader.
 */
ShaderInfo* ArcResourceAccessor::RegisterShader(const char* pName, bool isInitialized) {
    return m_ShaderList.RegisterShader(pName, isInitialized);
}

/** @param pShader Shader returned by RegisterShader. */
void ArcResourceAccessor::UnregisterShader(ShaderInfo* pShader) {
    m_ShaderList.UnregisterShader(pShader);
}

/**
 * @brief Allocates descriptor slots for every font and texture view.
 * @param pRegisterTextureViewSlot Allocates one descriptor slot.
 * @param pUserData User data passed to pRegisterTextureViewSlot.
 */
void ArcResourceAccessor::RegisterTextureViewToDescriptorPool(
    RegisterTextureView pRegisterTextureViewSlot, void* pUserData) {
    m_FontList.RegisterTextureViewToDescriptorPool(pRegisterTextureViewSlot, pUserData);
    m_TextureList.RegisterTextureViewToDescriptorPool(pRegisterTextureViewSlot, pUserData);
    m_ArcHandle.RegisterTextureViewToDescriptorPool(pRegisterTextureViewSlot, pUserData);
}

/**
 * @brief Releases the descriptor slots of every font and texture view.
 * @param pUnregisterTextureViewSlot Releases one descriptor slot.
 * @param pUserData User data passed to pUnregisterTextureViewSlot.
 */
void ArcResourceAccessor::UnregisterTextureViewFromDescriptorPool(
    UnregisterTextureView pUnregisterTextureViewSlot, void* pUserData) {
    m_FontList.UnregisterTextureViewFromDescriptorPool(pUnregisterTextureViewSlot, pUserData);
    m_TextureList.UnregisterTextureViewFromDescriptorPool(pUnregisterTextureViewSlot, pUserData);
    m_ArcHandle.UnregisterTextureViewFromDescriptorPool(pUnregisterTextureViewSlot, pUserData);
}

/**
 * @brief Registers a render target texture owned by the caller.
 * @param pName Texture name.
 * @return The new texture.
 */
TextureInfo* ArcResourceAccessor::RegisterRenderTargetTexture(const char* pName) {
    return m_TextureList.RegisterRenderTargetTexture(pName, false);
}

/** @param pTexture Texture returned by RegisterRenderTargetTexture. */
void ArcResourceAccessor::UnregisterRenderTargetTexture(TextureInfo* pTexture) {
    m_TextureList.UnregisterTexture(pTexture);
}

/** @brief Constructs an accessor with no archive attached. */
MultiArcResourceAccessor::MultiArcResourceAccessor() = default;

/** @brief Destroys the accessor. */
MultiArcResourceAccessor::~MultiArcResourceAccessor() = default;

/**
 * @brief Releases every resource and detaches every archive.
 * @param pDevice Device the resources were created on.
 */
void MultiArcResourceAccessor::Finalize(nn::gfx::Device* pDevice) {
    for (auto it = m_ArcList.begin(); it != m_ArcList.end();) {
        auto current = it++;
        current->GetArchiveHandle()->Finalize(pDevice);
        m_ArcList.erase(current);
        Layout::DeleteObj(&*current);
    }

    m_FontList.Finalize(pDevice);
    m_TextureList.Finalize(pDevice);
    m_ShaderList.Finalize(pDevice);
    ResourceAccessor::Finalize(pDevice);
}

/**
 * @brief Attaches an archive; it is searched after the archives attached before it.
 * @param pArchiveHandle Archive to attach.
 */
void MultiArcResourceAccessor::Attach(ArchiveHandle* pArchiveHandle) {
    ArcResourceLink* pLink = Layout::NewObj<ArcResourceLink>();
    pLink->SetArchiveHandle(pArchiveHandle);
    m_ArcList.push_back(*pLink);
}

/**
 * @brief Detaches an archive.
 * @param pArchiveHandle Archive to detach.
 */
void MultiArcResourceAccessor::Detach(const ArchiveHandle* pArchiveHandle) {
    for (auto it = m_ArcList.begin(); it != m_ArcList.end(); ++it) {
        if (it->GetArchiveHandle() == pArchiveHandle) {
            m_ArcList.erase(it);
            Layout::DeleteObj(&*it);
            return;
        }
    }
}

/** @brief Detaches every archive. */
void MultiArcResourceAccessor::DetachAll() {
    for (auto it = m_ArcList.begin(); it != m_ArcList.end();) {
        auto current = it++;
        m_ArcList.erase(current);
        Layout::DeleteObj(&*current);
    }
}

/**
 * @param pSize Receives the resource size; may be nullptr.
 * @param resType Resource type signature.
 * @param pName Resource name.
 * @return The resource from the first archive containing it, or nullptr.
 */
void* MultiArcResourceAccessor::FindResourceByName(size_t* pSize, u32 resType, const char* pName) {
    for (auto& link : m_ArcList) {
        ArchiveHandle* pArchiveHandle = link.GetArchiveHandle();
        void* pResource = GetResourceSub(pArchiveHandle, pArchiveHandle->GetResRootDir(), resType,
                                         pName, pSize);

        if (pResource != nullptr) {
            return pResource;
        }
    }

    return nullptr;
}

/**
 * @brief Calls a callback for every resource of one type in every archive.
 * @param resType Resource type signature.
 * @param pCallback Called with each resource, its size, its path and pParam.
 * @param pParam User data passed to pCallback.
 */
void MultiArcResourceAccessor::FindResourceByType(u32 resType, ResourceCallback pCallback,
                                                  void* pParam) const {
    for (const auto& link : m_ArcList) {
        ArchiveHandle* pArchiveHandle = link.GetArchiveHandle();
        FindResourceByTypeSub(pArchiveHandle, pArchiveHandle->GetResRootDir(), resType, pCallback,
                              pParam);
    }
}

/**
 * @brief Finds a font, loading it from the archives if needed.
 * @param pDevice Device the font is created on.
 * @param pName Font name.
 * @return The font, or nullptr when it is not found.
 */
nn::font::Font* MultiArcResourceAccessor::AcquireFont(nn::gfx::Device* pDevice, const char* pName) {
    nn::font::Font* pFont = m_FontList.FindFontByName(pName);

    if (pFont != nullptr) {
        return pFont;
    }

    for (auto& link : m_ArcList) {
        pFont = link.GetArchiveHandle()->GetFontList()->FindFontByName(pName);

        if (pFont != nullptr) {
            return pFont;
        }
    }

    ArchiveHandle* pArchiveHandle = FindFontArchive(pName);
    pFont = LoadFont(pDevice, pName);

    if (pFont != nullptr) {
        pArchiveHandle->RegisterFont(pName, pFont);
    }

    return pFont;
}

/**
 * @param pName Font resource name.
 * @return The first archive containing the font resource, or nullptr.
 */
ArchiveHandle* MultiArcResourceAccessor::FindFontArchive(const char* pName) {
    for (auto& link : m_ArcList) {
        ArchiveHandle* pArchiveHandle = link.GetArchiveHandle();

        if (GetResourceSub(pArchiveHandle, pArchiveHandle->GetResRootDir(), ResourceTypeFont,
                           pName, nullptr) != nullptr) {
            return link.GetArchiveHandle();
        }
    }

    return nullptr;
}

/**
 * @brief Registers a font owned by the caller.
 * @param pName Font name.
 * @param pFont Font to register.
 * @return Handle of the registration.
 */
const void* MultiArcResourceAccessor::RegisterFont(const char* pName, nn::font::Font* pFont) {
    return m_FontList.RegisterFont(pName, pFont, false);
}

/** @param pFontRef Handle returned by RegisterFont. */
void MultiArcResourceAccessor::UnregisterFont(const void* pFontRef) {
    m_FontList.UnregisterFont(pFontRef);
}

/**
 * @brief Finds a texture, loading it from the archives if needed.
 * @param pDevice Device the texture is created on.
 * @param pName Texture name.
 * @return The texture, or nullptr when it could not be registered.
 */
TextureInfo* MultiArcResourceAccessor::AcquireTexture(nn::gfx::Device* pDevice, const char* pName) {
    TextureInfo* pTexture = m_TextureList.FindTextureByName(pName);

    if (pTexture != nullptr) {
        return pTexture;
    }

    for (auto& link : m_ArcList) {
        pTexture = link.GetArchiveHandle()->GetTextureList()->FindTextureByName(pName);

        if (pTexture != nullptr) {
            return pTexture;
        }
    }

    ResourceTextureInfo* pResTexture = FindTextureArchive(pName)->RegisterTexture(pName);

    if (pResTexture != nullptr) {
        LoadTexture(pResTexture, pDevice, pName);
        // The validity is only checked; a texture that failed to load is still returned.
        pResTexture->IsValid();
    }

    return pResTexture;
}

/**
 * @param pName Texture name.
 * @return The first archive containing the texture, or nullptr.
 */
ArchiveHandle* MultiArcResourceAccessor::FindTextureArchive(const char* pName) {
    for (auto& link : m_ArcList) {
        ArchiveHandle* pArchiveHandle = link.GetArchiveHandle();
        void* pBntx = GetResourceSub(pArchiveHandle, pArchiveHandle->GetResRootDir(),
                                     ResourceTypeTexture, CombinedTextureFileName, nullptr);

        if (pBntx != nullptr) {
            nn::gfx::ResTextureFile* pTextureFile = nn::gfx::ResTextureFile::ResCast(pBntx);

            if (pTextureFile != nullptr && pTextureFile->ToData()
                                                   .textureContainerData.pTextureDic.Get()
                                                   ->FindIndex(pName) != -1) {
                return link.GetArchiveHandle();
            }
        }

        if (GetResourceSub(pArchiveHandle, pArchiveHandle->GetResRootDir(), ResourceTypeTexture,
                           pName, nullptr) != nullptr) {
            return link.GetArchiveHandle();
        }
    }

    return nullptr;
}

/**
 * @param pResTextureInfo Receives the texture.
 * @param pDevice Device the texture is created on.
 * @param pName Texture name.
 * @return Whether some archive could load the texture.
 */
bool MultiArcResourceAccessor::LoadTexture(ResourceTextureInfo* pResTextureInfo,
                                           nn::gfx::Device* pDevice, const char* pName) {
    for (auto& link : m_ArcList) {
        if (link.GetArchiveHandle()->LoadTexture(pResTextureInfo, pDevice, pName)) {
            return true;
        }
    }

    return false;
}

/**
 * @param pShaderInfo Receives the shader.
 * @param pDevice Device the shader is created on.
 * @param pName Shader name.
 * @return Whether some archive could load the shader.
 */
bool MultiArcResourceAccessor::LoadShader(ShaderInfo* pShaderInfo, nn::gfx::Device* pDevice,
                                          const char* pName) {
    for (auto& link : m_ArcList) {
        if (link.GetArchiveHandle()->LoadShader(pShaderInfo, pDevice, pName)) {
            return true;
        }
    }

    return false;
}

/**
 * @param pShaderInfo Receives the shader.
 * @param pDevice Device the shader is created on.
 * @param signature Shader signature.
 * @param keyCount Number of keys.
 * @param pKeys Keys selecting the variation.
 * @return Whether some archive could load the shader.
 */
bool MultiArcResourceAccessor::LoadArchiveShader(ShaderInfo* pShaderInfo,
                                                 nn::gfx::Device* pDevice, u32 signature,
                                                 size_t keyCount, const u32* pKeys) {
    for (auto& link : m_ArcList) {
        if (link.GetArchiveHandle()->LoadArchiveShader(pShaderInfo, pDevice, signature, keyCount,
                                                       pKeys)) {
            return true;
        }
    }

    return false;
}

/**
 * @brief Registers a texture owned by the caller.
 * @param pName Texture name.
 * @return The new texture.
 */
PlacementTextureInfo* MultiArcResourceAccessor::RegisterTexture(const char* pName) {
    return m_TextureList.RegisterPlacementTexture(pName, false);
}

/** @param pTexture Texture returned by RegisterTexture. */
void MultiArcResourceAccessor::UnregisterTexture(TextureInfo* pTexture) {
    m_TextureList.UnregisterTexture(pTexture);
}

/**
 * @brief Finds a shader, loading it from the archives if needed.
 * @param pDevice Device the shader is created on.
 * @param pName Shader name.
 * @return The shader, or nullptr when it could not be registered.
 */
ShaderInfo* MultiArcResourceAccessor::AcquireShader(nn::gfx::Device* pDevice, const char* pName) {
    ShaderInfo* pShader = m_ShaderList.FindShaderByName(pName);

    if (pShader != nullptr) {
        return pShader;
    }

    for (auto& link : m_ArcList) {
        pShader = link.GetArchiveHandle()->GetShaderList()->FindShaderByName(pName);

        if (pShader != nullptr) {
            return pShader;
        }
    }

    pShader = FindShaderArchive(pName)->RegisterShader(pName);

    if (pShader != nullptr) {
        LoadShader(pShader, pDevice, pName);
    }

    return pShader;
}

/**
 * @param pName Shader name.
 * @return The first archive containing the shader, or nullptr.
 */
ArchiveHandle* MultiArcResourceAccessor::FindShaderArchive(const char* pName) {
    char fileName[ArchivePathMax];
    nn::util::SNPrintf(fileName, sizeof(fileName), "ArchiveShader-%s.bnsh", pName);

    for (auto& link : m_ArcList) {
        ArchiveHandle* pArchiveHandle = link.GetArchiveHandle();

        if (GetResourceSub(pArchiveHandle, pArchiveHandle->GetResRootDir(), ResourceTypeShader,
                           fileName, nullptr) != nullptr) {
            return link.GetArchiveHandle();
        }
    }

    return nullptr;
}

/**
 * @brief Finds the archive shader matching a set of keys, loading it if needed.
 * @param pDevice Device the shader is created on.
 * @param signature Shader signature.
 * @param keyCount Number of keys.
 * @param pKeys Keys selecting the variation.
 * @return The shader, or nullptr when it could not be registered.
 */
ShaderInfo* MultiArcResourceAccessor::AcquireArchiveShader(nn::gfx::Device* pDevice,
                                                           u32 signature, size_t keyCount,
                                                           const u32* pKeys) {
    ShaderInfo* pShader = m_ShaderList.FindShaderByName(ArchiveShaderRegisterName);

    if (pShader != nullptr) {
        return pShader;
    }

    for (auto& link : m_ArcList) {
        pShader =
            link.GetArchiveHandle()->GetShaderList()->FindShaderByName(ArchiveShaderRegisterName);

        if (pShader != nullptr &&
            SearchShaderVariationIndexFromTable(pShader->m_pVariationTable, signature, keyCount,
                                                pKeys) != -1) {
            return pShader;
        }
    }

    pShader = FindArchiveShaderArchive(signature, keyCount, pKeys)
                  ->RegisterShader(ArchiveShaderRegisterName);

    if (pShader != nullptr) {
        LoadArchiveShader(pShader, pDevice, signature, keyCount, pKeys);
    }

    return pShader;
}

/**
 * @param signature Shader signature.
 * @param keyCount Number of keys.
 * @param pKeys Keys selecting the variation.
 * @return The first archive whose archive shader has the variation, or nullptr.
 */
ArchiveHandle* MultiArcResourceAccessor::FindArchiveShaderArchive(u32 signature, size_t keyCount,
                                                                  const u32* pKeys) {
    for (auto& link : m_ArcList) {
        ArchiveHandle* pArchiveHandle = link.GetArchiveHandle();

        if (GetResourceSub(pArchiveHandle, pArchiveHandle->GetResRootDir(), ResourceTypeShader,
                           ArchiveShaderFileName, nullptr) == nullptr) {
            continue;
        }

        const void* pVariationTable =
            GetResourceSub(pArchiveHandle, pArchiveHandle->GetResRootDir(), ResourceTypeShader,
                           ArchiveShaderVariationTableFileName, nullptr);

        if (SearchShaderVariationIndexFromTable(pVariationTable, signature, keyCount, pKeys) !=
            -1) {
            return link.GetArchiveHandle();
        }
    }

    return nullptr;
}

/**
 * @brief Registers a shader owned by the caller.
 * @param pName Shader name.
 * @return The new shader.
 */
ShaderInfo* MultiArcResourceAccessor::RegisterShader(const char* pName) {
    return m_ShaderList.RegisterShader(pName, false);
}

/** @param pShader Shader returned by RegisterShader. */
void MultiArcResourceAccessor::UnregisterShader(ShaderInfo* pShader) {
    m_ShaderList.UnregisterShader(pShader);
}

/**
 * @brief Allocates descriptor slots for every font and texture view.
 * @param pRegisterTextureViewSlot Allocates one descriptor slot.
 * @param pUserData User data passed to pRegisterTextureViewSlot.
 */
void MultiArcResourceAccessor::RegisterTextureViewToDescriptorPool(
    RegisterTextureView pRegisterTextureViewSlot, void* pUserData) {
    m_FontList.RegisterTextureViewToDescriptorPool(pRegisterTextureViewSlot, pUserData);
    m_TextureList.RegisterTextureViewToDescriptorPool(pRegisterTextureViewSlot, pUserData);

    for (auto& link : m_ArcList) {
        link.GetArchiveHandle()->RegisterTextureViewToDescriptorPool(pRegisterTextureViewSlot,
                                                                     pUserData);
    }
}

/**
 * @brief Releases the descriptor slots of every font and texture view.
 * @param pUnregisterTextureViewSlot Releases one descriptor slot.
 * @param pUserData User data passed to pUnregisterTextureViewSlot.
 */
void MultiArcResourceAccessor::UnregisterTextureViewFromDescriptorPool(
    UnregisterTextureView pUnregisterTextureViewSlot, void* pUserData) {
    m_FontList.UnregisterTextureViewFromDescriptorPool(pUnregisterTextureViewSlot, pUserData);
    m_TextureList.UnregisterTextureViewFromDescriptorPool(pUnregisterTextureViewSlot, pUserData);

    for (auto& link : m_ArcList) {
        link.GetArchiveHandle()->UnregisterTextureViewFromDescriptorPool(
            pUnregisterTextureViewSlot, pUserData);
    }
}

/**
 * @brief Registers a render target texture owned by the caller.
 * @param pName Texture name.
 * @return The new texture.
 */
TextureInfo* MultiArcResourceAccessor::RegisterRenderTargetTexture(const char* pName) {
    return m_TextureList.RegisterRenderTargetTexture(pName, false);
}

/** @param pTexture Texture returned by RegisterRenderTargetTexture. */
void MultiArcResourceAccessor::UnregisterRenderTargetTexture(TextureInfo* pTexture) {
    m_TextureList.UnregisterTexture(pTexture);
}

}  // namespace nn::ui2d
