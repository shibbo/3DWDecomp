#pragma once

#include <filedevice/seadArchiveFileDevice.h>
#include <resource/seadSharcArchiveRes.h>

namespace eui {

class SharcArchive {
public:
    class FileReader {
    public:
        /** @brief Creates a reader with no associated archive or current entry. */
        FileReader() : m_FileDevice(nullptr), mIndex(-1) {}
        ~FileReader();
        bool readNext();

        /** @return The file device reading the archive. */
        const sead::ArchiveFileDevice& getFileDevice() const { return m_FileDevice; }
        /** @return The index of the current entry. */
        s32 getIndex() const { return mIndex; }
        /** @return The current entry. */
        const sead::DirectoryEntry& getEntry() const { return m_Entry; }

    private:
        friend class SharcArchive;
        sead::ArchiveFileDevice m_FileDevice;
        s32 mIndex;
        sead::DirectoryHandle m_Handle;
        sead::DirectoryEntry m_Entry;
    };

    SharcArchive();
    ~SharcArchive();
    void initialize(sead::Heap* pHeap, void* pData, u32 size);
    void finalize();
    sead::FileDevice* startFileReader(FileReader* pReader) const;

    /** @return The archive resource. */
    sead::SharcArchiveRes* getArchive() const { return m_pArchive; }

private:
    sead::SharcArchiveRes* m_pArchive;
};

}  // namespace eui
