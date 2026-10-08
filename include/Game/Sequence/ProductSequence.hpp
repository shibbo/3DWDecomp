#pragma once

#include <basis/seadTypes.h>
#include "Library/Sequence/Sequence.hpp"
#include "Project/Memory/MemorySystem.hpp"

class ControllerConnectChecker;
class GameDataHolder;
class LoadingLayout;
class ProductAsyncResourceLoader;
class ProductStageStartParam;
class ProductStateAfterEndingEvent;
class ProductStateBoot;
class ProductStateCourseSelect;
class ProductStateEnding;
class ProductStateLuigiBros;
class ProductStateSingleMode;
class ProductStateSingleModeEnding;
class ProductStateSingleOpeningDemo;
class ProductStateStage;
class ProductStateTitle;
class ProductStateTopMenu;
class SequenceWindowKeeper;
class StageWipeKeeper;
class WipeCurtainResult;

namespace al {
class LayoutKit;
}  // namespace al

namespace sead {
class Viewport;
}  // namespace sead

/**
 * @brief The game's root sequence, switching between the title, stages and the menus.
 */
class ProductSequence : public al::Sequence, public al::StageSizeAdjuster {
public:
    ProductSequence(const char* pName);
    ~ProductSequence() override;

    s32 adjustStageSize(const char* pStageName, s32 size,
                        al::MemorySceneHeapCustomAlloc* pAlloc) override;
    static void deleteStationedAssets();
    bool isDisposable() const override;
    void refreshDebugMenu();
    GameDataHolder* getGameDataHolderBase() const;
    void init(const al::SequenceInitInfo& rInfo) override;
    void update() override;
    void drawMain() const override;
    void drawSub() const override;
    void startLoadScreen(bool isFadeIn, bool isShowTips);
    void requestLoadScreenEnd();
    bool isLoadLayoutEnd();
    void exeTopMenu();
    void exeBoot();
    void exeTitle();
    void exeSingleModeOpeningDemo();
    void exeCourseSelect();
    void exeStage();
    void exeSingleMode();
    void exeEnding();
    void exeSingleModeEnding();
    void exeAfterEndingEvent();
    void exeLuigiBros();
    void requestCaptureTopBottom();
    void offDrawScreenCapture();
    void setKioskStageStartParam(s32 worldId, s32 stageId);

    al::Scene* getCurrentScene() const override { return nullptr; }

private:
    s32 getUnlockedSingleModePhase() const;

    ProductStageStartParam* mStageStartParam = nullptr;                // 0xb8
    GameDataHolder* mGameDataHolder = nullptr;                         // 0xc0
    StageWipeKeeper* mStageWipeKeeper = nullptr;                       // 0xc8
    SequenceWindowKeeper* mSequenceWindowKeeper = nullptr;             // 0xd0
    WipeCurtainResult* mWipeCurtainResult = nullptr;                   // 0xd8
    ProductStateBoot* mStateBoot = nullptr;                            // 0xe0
    ProductStateTitle* mStateTitle = nullptr;                          // 0xe8
    ProductStateSingleOpeningDemo* mStateSingleOpeningDemo = nullptr;  // 0xf0
    ProductStateSingleMode* mStateSingleMode = nullptr;                // 0xf8
    ProductStateSingleModeEnding* mStateSingleModeEnding = nullptr;    // 0x100
    ProductStateCourseSelect* mStateCourseSelect = nullptr;            // 0x108
    ProductStateStage* mStateStage = nullptr;                          // 0x110
    ProductStateEnding* mStateEnding = nullptr;                        // 0x118
    ProductStateAfterEndingEvent* mStateAfterEndingEvent = nullptr;    // 0x120
    ProductStateTopMenu* mStateTopMenu = nullptr;                      // 0x128
    ProductStateLuigiBros* mStateLuigiBros = nullptr;                  // 0x130
    al::LayoutKit* mLayoutKit = nullptr;                               // 0x138
    sead::Viewport* mViewportTop = nullptr;                            // 0x140
    sead::Viewport* mViewportBottom = nullptr;                         // 0x148
    const al::GameSystemInfo* mGameSystemInfo = nullptr;               // 0x150
    al::ScreenCaptureExecutor* mScreenCaptureExecutor = nullptr;       // 0x158
    bool mIsShowResult = false;                                        // 0x160
    ProductAsyncResourceLoader* mAsyncResourceLoader = nullptr;        // 0x168
    void* _170 = nullptr;
    void* _178 = nullptr;
    ControllerConnectChecker* mControllerConnectChecker = nullptr;     // 0x180
    bool mIsRequestPauseBySystemError = false;                         // 0x188
    bool mIsPauseBySystemError = false;                                // 0x189
    bool mIsGotoTitleFromTopMenu = true;                               // 0x18a
    LoadingLayout* mLoadingLayout;                                     // 0x190
    void* _198;
};

static_assert(sizeof(ProductSequence) == 0x1a0);
