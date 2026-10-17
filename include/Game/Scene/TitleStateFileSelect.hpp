#pragma once
#include "Library/Nerve/NerveStateBase.hpp"
class RCS_SaveDataLayout;
class WindowProcessing;
class WindowConfirm;
class GameDataHolder;
namespace al {
class LayoutInitInfo;
}
/** Title-menu file selection, including copy and deletion confirmation windows. */
class TitleStateFileSelect : public al::NerveStateBase {
public:
    TitleStateFileSelect(RCS_SaveDataLayout* pLayout, GameDataHolder* pGameDataHolder,
                         const al::LayoutInitInfo& rInfo);
    ~TitleStateFileSelect() override = default;
    void appear() override;
    void setControllerPort(s32 port);
    void exeSelect();
    void exeCopyConfirm();
    void exeCopy();
    void exeCopyResult();
    void exeCopyEnd();
    void backToSelect();
    void exeDeleteConfirm();
    void exeDelete();
    void exeDeleteResult();
    void exeDeleteEnd();

private:
    RCS_SaveDataLayout* mSaveDataLayout;
    WindowProcessing* mProcessing = nullptr;
    WindowConfirm* mReport = nullptr;
    WindowConfirm* mConfirm = nullptr;
    GameDataHolder* mGameDataHolder;
    s32 mResult = 0;
    s32 mFileId = -1;
};
static_assert(sizeof(TitleStateFileSelect) == 0x48);
