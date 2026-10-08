#pragma once

#include <basis/seadTypes.h>

#include "Library/Scene/ISceneObj.hpp"

namespace al {
class IUseSceneObjHolder;
class LayoutInitInfo;
}  // namespace al

/** @brief Priority of a guide message; a higher one replaces a lower one already shown. */
enum GuideMessagePriority : s32 {};

/** @brief Scene object (SceneObjID_GuideGameWindow) showing the in-game guide messages. */
class GuideGameWindow : public al::ISceneObj {
public:
    GuideGameWindow(const al::LayoutInitInfo& rInfo, bool isSingleMode);

    bool isWaitConfirm() const;
    void endHide(const void* pUser);

    /** @brief Sets the lowest priority a guide message needs to be shown. */
    void setPriorityLimit(GuideMessagePriority priority) { mPriorityLimit = priority; }

private:
    u8 _8[0x24 - 0x8];
    GuideMessagePriority mPriorityLimit;  // 0x24
};

static_assert(sizeof(GuideGameWindow) == 0x28);

namespace rc {
void appearGuideGameWindow(const al::IUseSceneObjHolder* pHolder, const char* pCategory,
                           const char* pLabel, s32 frame, f32 delay);
void appearGuideGameWindowWithConfirm(const al::IUseSceneObjHolder* pHolder, const char* pMessage,
                                      bool isConfirm);
bool appearGuideGameWindowWithPriority(const al::IUseSceneObjHolder* pHolder,
                                       const char* pCategory, const char* pMessage,
                                       GuideMessagePriority priority, s32 frame, f32 delay);
void disappearGuideGameWindow(const al::IUseSceneObjHolder* pHolder);
bool isCurrentGuideGameWindowUser(const al::IUseSceneObjHolder* pHolder);
void unHideGuideGameWindow(const al::IUseSceneObjHolder* pHolder);
void hideGuideGameWindow(const al::IUseSceneObjHolder* pHolder);
bool isGuideGameWindowWaitConfirm(const al::IUseSceneObjHolder* pHolder);
bool isGuideGameWindowActive(const al::IUseSceneObjHolder* pHolder);
void disableGuideGameWindowPriority(const al::IUseSceneObjHolder* pHolder);
void enableGuideGameWindowPriority(const al::IUseSceneObjHolder* pHolder);
}  // namespace rc
