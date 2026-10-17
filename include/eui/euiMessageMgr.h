#pragma once
#include <heap/seadDisposer.h>
#include <prim/seadRuntimeTypeInfo.h>
#include <container/seadOffsetList.h>
#include <container/seadBuffer.h>
#include <gfx/seadColor.h>
#include <prim/seadSafeString.h>
namespace eui {
class MessageSet;
class MessageMgr {
    SEAD_SINGLETON_DISPOSER(MessageMgr);
public:
    SEAD_RTTI_BASE(MessageMgr);
    class Archive;
    struct GradationColor { sead::Color4u8 top; sead::Color4u8 bottom; };
    MessageMgr();
    virtual ~MessageMgr();
    void initialize(sead::Heap* pHeap, u32 gradationColorNum);
    virtual void loadArchive(sead::Heap* pHeap, void* pData, u32 size);
    virtual void unloadArchive(void* pData);
    void setGradationColor(u32 index, sead::Color4u8 top, sead::Color4u8 bottom);
    void dumpLastGotMessageSetInfo();
    const MessageSet* getLayoutMessageSet(const sead::SafeString& rName) const;
    sead::OffsetList<Archive> mArchives;
    sead::Buffer<GradationColor> mGradationColors;
    bool mTextBoxWidthSizeOverColorEnabled;
};

static_assert(sizeof(MessageMgr) == 0x58, "MessageMgr size");
}
