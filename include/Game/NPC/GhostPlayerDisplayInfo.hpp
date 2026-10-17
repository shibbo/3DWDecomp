#pragma once

#include <basis/seadTypes.h>
#include <prim/seadSafeString.h>

namespace al {
class NexDataStoreDownloadInfo;
}  // namespace al

/** @brief Display information for one loaded ghost (owner name and flags). */
class GhostPlayerDisplayInfo {
public:
    GhostPlayerDisplayInfo(const char* pUserName);
    GhostPlayerDisplayInfo();

    void init(const al::NexDataStoreDownloadInfo& rInfo, bool isMii);

    sead::WFixedSafeString<32> mUserName;
    bool mIsMii = false;
    bool mIsValid = false;
};
