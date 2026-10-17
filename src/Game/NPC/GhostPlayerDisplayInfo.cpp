#include "NPC/GhostPlayerDisplayInfo.hpp"
#include "NPC/GhostPlayerUserInfo.hpp"
#include <prim/seadStringUtil.h>

/** @brief Converts a built-in ghost's display name to UTF-16. */
GhostPlayerDisplayInfo::GhostPlayerDisplayInfo(const char* pId) {
    const auto* pUserInfo = rc::findGhostPlayerUserInfo(pId);
    const char* pUserName = pUserInfo->userName;
    sead::StringUtil::convertSjisToUtf16(mUserName.getBuffer(), mUserName.getBufferSize(),
                                       pUserName, -1);
    mIsFriend = pUserInfo->isFriend;
    mIsValid = true;
}

/** @brief Creates empty, unavailable ghost display information. */
GhostPlayerDisplayInfo::GhostPlayerDisplayInfo() {}

/** @brief Marks downloaded display information available with its friend flag. */
void GhostPlayerDisplayInfo::init(const al::NexDataStoreDownloadInfo&, bool isFriend) {
    mIsFriend = isFriend;
    mIsValid = true;
}
