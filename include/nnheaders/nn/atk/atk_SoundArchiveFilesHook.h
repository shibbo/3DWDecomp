#pragma once

#include <nn/types.h>

namespace nn::atk {
namespace detail {
namespace fnd {
class FileStream;
}  // namespace fnd

/** @brief Lets a tool replace sound archive files (for example while editing sounds live). */
class SoundArchiveFilesHook {
public:
    static const char ItemTypeSequenceSound[];
    static const char ItemTypeWaveSound[];
    static const char FileTypeSequenceBinary[];
    static const char FileTypeWaveSoundBinary[];
    static const char FileTypeStreamBinary[];

    virtual ~SoundArchiveFilesHook() {}

    /** @brief Checks whether the hook currently overrides files. @return True when enabled. */
    bool GetIsEnable() const { return m_IsEnabled; }

    /**
     * @brief Enables or disables the hook.
     * @param isEnabled True to let the hook override files.
     */
    void SetIsEnable(bool isEnabled) { m_IsEnabled = isEnabled; }

    /**
     * @brief Checks whether the hook overrides an item.
     * @param pItemLabel Label of the archive item.
     * @return True when the hook provides data for the item.
     */
    bool IsTargetItem(const char* pItemLabel) { return IsTargetItemImpl(pItemLabel); }

    /** @brief Blocks other threads from changing the hooked files. */
    void Lock() { LockImpl(); }

    /** @brief Releases the lock taken by Lock. */
    void Unlock() { UnlockImpl(); }

    /**
     * @brief Gets the replacement data of an item file.
     * @param pItemLabel Label of the archive item.
     * @param pItemType Item type name, such as ItemTypeSequenceSound.
     * @param pFileType File type name, such as FileTypeSequenceBinary.
     * @param fileIndex Index of the file within the item.
     * @return Replacement file data, or nullptr when the hook has none.
     */
    const void* GetFileAddress(const char* pItemLabel, const char* pItemType,
                               const char* pFileType, u32 fileIndex) {
        return GetFileAddressImpl(pItemLabel, pItemType, pFileType, fileIndex);
    }

    /**
     * @brief Opens the replacement file of an item.
     * @param pBuffer Storage the hook constructs the returned stream in.
     * @param bufferSize Size of pBuffer in bytes.
     * @param pCacheBuffer Read cache for the stream, or nullptr for none.
     * @param cacheSize Size of pCacheBuffer in bytes.
     * @param pItemLabel Label of the archive item.
     * @param pFileType File type name, such as FileTypeStreamBinary.
     * @return Opened stream inside pBuffer, or nullptr when the hook has no file.
     */
    fnd::FileStream* OpenFile(void* pBuffer, size_t bufferSize, void* pCacheBuffer,
                              size_t cacheSize, const char* pItemLabel, const char* pFileType) {
        return OpenFileImpl(pBuffer, bufferSize, pCacheBuffer, cacheSize, pItemLabel, pFileType);
    }

protected:
    virtual bool IsTargetItemImpl(const char* pItemLabel) = 0;
    virtual void LockImpl() = 0;
    virtual void UnlockImpl() = 0;
    virtual fnd::FileStream* OpenFileImpl(void* pBuffer, size_t bufferSize, void* pCacheBuffer,
                                          size_t cacheSize, const char* pItemLabel,
                                          const char* pFileType) = 0;
    virtual const void* GetFileAddressImpl(const char* pItemLabel, const char* pItemType,
                                           const char* pFileType, u32 fileIndex) = 0;

private:
    bool m_IsEnabled;
};
}  // namespace detail
}  // namespace nn::atk
