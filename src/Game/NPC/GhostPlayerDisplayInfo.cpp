#include "NPC/GhostPlayerDisplayInfo.hpp"
#include "Util/GhostPlayerUtil.hpp"
#include <prim/seadStringUtil.h>

/**
 * @brief Builds the display info of a built-in (ROM) ghost from its user table entry.
 * @param pUserName Key of the ghost owner in the built-in user table.
 */
GhostPlayerDisplayInfo::GhostPlayerDisplayInfo(const char* pUserName) {
    const rc::GhostPlayerUserInfo* pInfo = rc::findGhostPlayerUserInfo(pUserName);
    const char* pName = pInfo->mUserName;
    sead::StringUtil::convertSjisToUtf16(mUserName.getBuffer(), mUserName.getBufferSize(), pName,
                                         -1);
    mIsMii = pInfo->mIsMii;
    mIsValid = true;
}

/** @brief Creates an empty, invalid display info. */
GhostPlayerDisplayInfo::GhostPlayerDisplayInfo() = default;

/**
 * @brief Fills the display info from a downloaded data store entry.
 * @param rInfo Download info of the data store entry.
 * @param isMii Whether the ghost owner is shown with a Mii.
 */
void GhostPlayerDisplayInfo::init(const al::NexDataStoreDownloadInfo& rInfo, bool isMii) {
    mIsMii = isMii;
    mIsValid = true;
}
