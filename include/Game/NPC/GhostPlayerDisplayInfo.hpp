#pragma once
#include <prim/seadSafeString.h>
namespace al { class NexDataStoreDownloadInfo; }
class GhostPlayerDisplayInfo {
public:
    GhostPlayerDisplayInfo();
    GhostPlayerDisplayInfo(const char*);
    void init(const al::NexDataStoreDownloadInfo&, bool);
    sead::WFixedSafeString<32> mUserName;
    bool mIsFriend = false;
    bool mIsValid = false;
};
static_assert(sizeof(GhostPlayerDisplayInfo) == 0x60);
