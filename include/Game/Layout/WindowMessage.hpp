#pragma once
#include "Library/Layout/LayoutActor.hpp"
namespace al { class LayoutInitInfo; }
/** @brief System-message window with paging and player input handling. */
class WindowMessage : public al::LayoutActor {
public:
    WindowMessage(const al::LayoutInitInfo& rInfo, const char* pLayoutName,
                  const char* pName, const char* pGroupName);
    void appearWithCurrentMessage(int port);
    void appearWithMessage(const char16_t* pMessage, int port);
    void appearWithSystemMessage(const char* pFile, const char* pMessage, int port);
    int getCurrentPageNum() const;
    void exeAppear();
    void exeWait();
    void exePageEnd();
    void exePageStart();
    void exeEnd();
private:
    const char16_t* mMessage = nullptr;
    int mPort;
    int mPageCount = 0;
    int mCurrentPage = 0;
    bool mIsSingleMode;
};
static_assert(sizeof(WindowMessage) == 0x140);
