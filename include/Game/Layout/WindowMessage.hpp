#pragma once

#include <basis/seadTypes.h>

namespace al {
class LayoutInitInfo;
}

// Partial layout: the layout actor and unreconstructed window state remain opaque.
class alignas(8) WindowMessage {
public:
    WindowMessage(const al::LayoutInitInfo& rInfo, const char* pLayoutName,
                  const char* pActorName, const char* pGroupName);
    void appearWithSystemMessage(const char* pSystemName, const char* pMessageId, int padPort);

    /**
     * @brief Check whether the message window is active.
     * @return Whether the underlying layout actor is alive.
     */
    bool isAlive() const { return mIsAlive; }

private:
    u8 mLayoutActor[0x120];
    bool mIsAlive;
    u8 mUnreconstructed121[0x1f];
};

static_assert(sizeof(WindowMessage) == 0x140);
