#pragma once

#include <basis/seadTypes.h>
#include <container/seadOffsetList.h>
#include <heap/seadDisposer.h>
#include <prim/seadSafeString.h>
#include <resource/seadResource.h>

namespace nn::gfx {
class ResTextureFile;
}

namespace eui {
/** @brief Owner of the layout archives loaded for the UI. */
class ArcResourceMgr {
public:
    /** @brief One loaded layout archive. */
    class ArcResource : public sead::IDisposer {
    public:
        ArcResource(ArcResourceMgr* pMgr, const sead::SafeString& rName, void* pData);
        ~ArcResource() override;

        sead::ListNode mListNode;
        ArcResourceMgr* mMgr;
        sead::FixedSafeString<64> mName;
        void* mData;
        nn::gfx::ResTextureFile* mTextureFile;
    };

    static_assert(sizeof(ArcResource) == 0xa0, "ArcResource size");

    /** @brief Factory that creates a single resource in place, without allocating it. */
    class OneTimeBinaryResourceFactory : public sead::DirectResourceFactoryBase {
        SEAD_RTTI_OVERRIDE(OneTimeBinaryResourceFactory, sead::DirectResourceFactoryBase)

    public:
        OneTimeBinaryResourceFactory() : mIsCreated(false) {}

        sead::DirectResource* newResource_(sead::Heap* pHeap, s32 alignment) override;

        alignas(sead::DirectResource) u8 mResourceBuffer[sizeof(sead::DirectResource)];
        bool mIsCreated;
    };

    using ArcResourceList = sead::OffsetList<ArcResource>;

    ArcResourceMgr();
    virtual ~ArcResourceMgr();
    virtual void loadArchivesInDirectory(sead::Heap* pHeap, const sead::SafeString& rPath);
    virtual void loadArchive(sead::Heap* pHeap, const sead::SafeString& rPath);
    virtual void* findArchiveData(const sead::SafeString& rName) const;
    virtual void unloadAllArchives();
    virtual void addArchiveToList(ArcResource* pArchive);
    virtual void eraseArchiveFromList(ArcResource* pArchive);
    virtual ArcResource* findArcResource(const sead::SafeString& rName) const;

    static void finalizeInitializedShaderResource(void* pArchive);
    static void reinitializeShaderResource(void* pArchive);

private:
    ArcResourceList mList;
};

static_assert(sizeof(ArcResourceMgr) == 0x20, "ArcResourceMgr size");
}  // namespace eui
