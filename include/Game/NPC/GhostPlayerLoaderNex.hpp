#pragma once
#include "NPC/GhostPlayerLoaderBase.hpp"
#include <container/seadPtrArray.h>
namespace al { class NexDataStoreAccessor; class NetworkSystem; }
class GhostPlayerLoaderNex : public GhostPlayerLoaderBase {
public:
    GhostPlayerLoaderNex(const char*, al::NexDataStoreAccessor*, const al::NetworkSystem*);
    bool isExistParamByOwnerId(int) const;
    bool load(u8**, u32, int, int) override;
    void extractGhostDataUserName(sead::BufferedSafeString*, int) const override;
    const GhostPlayerDisplayInfo* getDisplayInfo(int) const override;
    int getGhostPlayDataNum() const override;
    virtual bool isNeedReadBuffer() const;
private:
    al::NexDataStoreAccessor* mAccessor;
    sead::FixedSafeString<32> mStageName;
    sead::PtrArray<u8> mGhostData;
    sead::PtrArray<GhostPlayerDisplayInfo> mDisplayInfo;
    const al::NetworkSystem* mNetworkSystem;
};
static_assert(sizeof(GhostPlayerLoaderNex) == 0x78);