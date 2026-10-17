#pragma once
#include "Library/Nerve/NerveExecutor.hpp"
namespace al { class LayoutInitInfo; class NetworkSystem; class ErrorViewer; class MiiversePostDataKeeper; }
class WindowMiiverseDetail;
class WindowProcessing;
class WindowConfirm;
/** @brief Manages Miiverse detail interactions and confirmation dialogs. */
class MiiverseStateDetail : public al::NerveExecutor {
public:
    MiiverseStateDetail(const al::LayoutInitInfo& rInfo, al::NetworkSystem* pNetwork, al::ErrorViewer* pErrorViewer);
    ~MiiverseStateDetail() override = default;
    void appear(const al::MiiversePostDataKeeper* pPost, int port);
    bool isEnd() const;
    void exeDetail();
    void exeYeahWait();
    void exeYeahReport();
    void exeComment();
    void exeError();
    void exeEnd();
private:
    WindowMiiverseDetail* mDetail = nullptr;
    WindowProcessing* mProcessing = nullptr;
    WindowConfirm* mReport = nullptr;
    WindowConfirm* mConfirm = nullptr;
    const al::MiiversePostDataKeeper* mPost = nullptr;
    al::NetworkSystem* mNetwork;
    al::ErrorViewer* mErrorViewer;
    int mPort = 5;
};
static_assert(sizeof(MiiverseStateDetail) == 0x50);
