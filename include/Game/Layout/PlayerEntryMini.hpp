#pragma once

#include <basis/seadTypes.h>

#include "Library/Layout/LayoutActor.hpp"

namespace al {
class LayoutInitInfo;
class PlayerHolder;
}  // namespace al

class GameDataHolder;
class PlayerEntryItem;

/// Small player-entry window listing one PlayerEntryItem per controller user.
class PlayerEntryMini : public al::LayoutActor {
public:
    /// Scene the entry window is shown in.
    enum SceneType : s32 {
        SceneType_Stage = 0,
        SceneType_PreStageWipe = 1,
        SceneType_CourseSelect = 2,
        SceneType_KinopioStage = 3,
        SceneType_KinopioPreStageWipe = 4,
    };

    static constexpr s32 cItemNum = 4;

    PlayerEntryMini(const al::LayoutInitInfo& rInfo, GameDataHolder* pHolder,
                    al::PlayerHolder* pPlayerHolder, s32 sceneType);

    void appear() override;
    void movement() override;

    bool isPreStageWipe() const;
    bool isKinopioPreStageWipe() const;
    bool isKinopioAny() const;
    bool isKinopioStage() const;
    bool isStageScene() const;
    bool isCourseSelectScene() const;
    GameDataHolder* getGameDataHolder();
    void startCharacterSelect();
    void endCharacterSelect();
    void startPlayerEntry(bool isDeactivate);
    void startShuffle();
    void startDemo();
    void hide();
    void endDemo();
    void setItemActive(s32 userId);
    bool isAllPlayerDecided() const;
    bool isPlayerEntryEnd() const;
    s32 calcActiveItemNum() const;
    bool isEnableShowBatteryParts(s32 userId) const;
    bool playerEntry(s32 userId, s32 characterType);
    void playerCancel(s32 userId);
    void changeUserPort(s32 srcUserId, s32 dstUserId);
    void onDecideItem(PlayerEntryItem* pItem);
    void checkPadConnection();
    void requestStartLrAssignMode();
    void requestStopLrAssignMode();

    void exeWait();
    void exePlayerEntry();
    void exePlayerEntryAllDecide();
    void exePlayerEntryEnd();
    void exeShuffle();

private:
    SceneType mSceneType;
    s32 mAllDecideFrame;
    GameDataHolder* mGameDataHolder;
    al::PlayerHolder* mPlayerHolder;
    PlayerEntryItem** mItems;
    bool mIsItemDecided;
};

static_assert(sizeof(PlayerEntryMini) == 0x150);
