#pragma once

#include <nn/atk/atk_SoundArchive.h>
#include <nn/atk/atk_SoundHeap.h>
#include <nn/util/util_IntrusiveList.h>

namespace nn::atk {
namespace detail {
namespace driver {
class DisposeCallback {
public:
    util::IntrusiveListNode m_DisposeLink;

    virtual ~DisposeCallback() {}
    virtual void InvalidateData(const void* pStart, const void* pEnd) = 0;
};
}  // namespace driver

class SoundArchiveLoader {
public:
    SoundArchiveLoader();
    virtual ~SoundArchiveLoader();

    bool IsAvailable() const;
    bool IsDataLoaded(SoundArchive::ItemId id, u32 loadFlag) const;
    bool IsDataLoaded(const char* pItemName, u32 loadFlag) const;
    bool LoadData(SoundArchive::ItemId id, SoundMemoryAllocatable* pAllocator, u32 loadFlag,
                  size_t loadBlockSize);
    bool LoadData(const char* pItemName, SoundMemoryAllocatable* pAllocator, u32 loadFlag,
                  size_t loadBlockSize);
    bool detail_LoadWaveArchiveByWaveSoundFile(const void* pWaveSoundFile, int index,
                                              SoundMemoryAllocatable* pAllocator);
    bool detail_LoadWaveArchiveByBankFile(const void* pBankFile, SoundMemoryAllocatable* pAllocator);
    const void* detail_GetFileAddressByItemId(SoundArchive::ItemId id) const;
    const void* GetFileAddressFromSoundArchive(SoundArchive::FileId id) const;
    void SetSoundArchive(const SoundArchive* pArchive);
    /** @brief Gets the archive currently attached to this loader. @return Archive pointer, or nullptr. */
    const SoundArchive* GetSoundArchive() const { return mArchive; }

protected:
    virtual const void* SetFileAddressToTable(SoundArchive::FileId fileId, const void* pAddress) = 0;
    virtual const void* GetFileAddressFromTable(SoundArchive::FileId fileId) const = 0;
    virtual const void* GetFileAddressImpl(SoundArchive::FileId fileId) const = 0;

private:
    const SoundArchive* mArchive;
    u8 _10[0x218 - 0x10];
};

/** @brief An archive item to load, and where it ended up once loaded. */
struct LoadItemInfo {
    /** @brief Creates an entry naming no item. */
    LoadItemInfo() : itemId(SoundArchive::InvalidId), address(nullptr) {}

    SoundArchive::ItemId itemId;
    const void* address;
};
}  // namespace detail

class SoundDataManager : public detail::driver::DisposeCallback, public detail::SoundArchiveLoader {
public:
    SoundDataManager();
    ~SoundDataManager() override;

    size_t GetRequiredMemSize(const SoundArchive* pArchive) const;
    bool Initialize(const SoundArchive* pArchive, void* pBuffer, size_t size);
    bool CreateTables(void** ppCursor, const SoundArchive* pArchive, void* pEnd);
    const void* detail_GetFileAddress(SoundArchive::FileId fileId) const;
    SoundArchive::FileId detail_GetFileIdFromTable(const void* pAddress) const;
    const void* SetFileAddress(SoundArchive::FileId fileId, const void* pAddress);
    bool SetFileAddressInGroupFile(const void* pGroupFile, size_t size);
    void ClearFileAddressInGroupFile(const void* pGroupFile, size_t size);
    void InvalidateSoundData(const void* pMemory, size_t size);
    void Finalize();

    void InvalidateData(const void* pStart, const void* pEnd) override;

protected:
    const void* SetFileAddressToTable(SoundArchive::FileId fileId, const void* pAddress) override;
    const void* GetFileAddressFromTable(SoundArchive::FileId fileId) const override;
    const void* GetFileAddressImpl(SoundArchive::FileId fileId) const override;
private:
    struct FileTable {
        u32 count;
        const void* addresses[1];
    };
    class FileAddressProvider {
    public:
        /** @brief Destroys the optional external file-address provider interface. */
        virtual ~FileAddressProvider() = default;
        virtual const void* GetFileAddress(SoundArchive::FileId fileId) = 0;
    };
    FileTable* mFileTable;
    FileAddressProvider* mFileAddressProvider;
};
static_assert(sizeof(SoundDataManager) == 0x240);
}  // namespace nn::atk
