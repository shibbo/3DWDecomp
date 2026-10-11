#pragma once

#include "Library/Layout/LayoutActor.hpp"

namespace al { class LayoutInitInfo; }

/** @brief System-message window with player input handling. */
class WindowMessage : public al::LayoutActor {
public:
    WindowMessage(const al::LayoutInitInfo& rInfo, const char* pName,
                  const char* pLayoutName, const char* pGroupName);
    void appearWithSystemMessage(const char* pFile, const char* pMessage, int port);

private:
    u8 _121[0x1f];
};

static_assert(sizeof(WindowMessage) == 0x140);
