#pragma once

#include <basis/seadTypes.h>

#include "Library/Nerve/IUseNerve.hpp"

namespace al {
class ActorInitInfo;
class LiveActor;
class NerveKeeper;
class PlayerHolder;
}  // namespace al

class GuideFrameOut;
class PlayerActor;
class PlayerAliveWatcher;
class PlayerAliveWatcherGroup;
class PlayerAmiiboDirector;
class TractorBubble;

/// Tracks the alive / dead / bubble state of one player of a PlayerAliveWatcherGroup.
class PlayerAliveWatcherCharacter : public al::IUseNerve {
    friend class PlayerAliveWatcherGroup;

public:
    PlayerAliveWatcherCharacter(const al::ActorInitInfo& rInfo, PlayerActor* pActor,
                                PlayerAliveWatcherGroup* pGroup);

    al::NerveKeeper* getNerveKeeper() const override { return mNerveKeeper; }

    void appear();
    void startDemo();
    void endDemo();
    void startPause();
    void endPause();
    bool isGameOver() const;
    bool isDead() const;
    bool isWaitBubble() const;
    bool isInBubble() const;
    void deactivate();
    bool isWaitBubbleForRevive() const;
    bool isAlive() const;
    bool isDeadOrBubble() const;
    bool isKill() const;
    bool isScreenOut(s32 frame) const;
    bool isScreenIconOut(s32 screenOutFrame, s32 iconOutFrame) const;
    PlayerActor* startBubbleRevive();
    void startBubbleScreenOut();
    void startBubbleInput();
    void deadPlayer();
    void setAmiiboDirector(PlayerAmiiboDirector* pDirector);
    void update();

    void exeInit();
    void exeKill();
    void exeAlive();
    void exeDead();
    void exeAbyss();
    void exeBubbleWait();
    void exeBubbleWaitForScreenOut();
    void exeBubbleWaitForInput();
    void exeReviveWait();

    /**
     * @brief Get the watched player.
     * @return The player actor.
     */
    PlayerActor* getActor() const { return mActor; }

private:
    void tryReassignAmiiboDirector();

    al::NerveKeeper* mNerveKeeper = nullptr;  // 0x08
    PlayerAliveWatcherGroup* mGroup;          // 0x10
    PlayerActor* mActor;                      // 0x18
    GuideFrameOut* mFrameOut = nullptr;       // 0x20
    bool mIsDemo = false;                     // 0x28
};

static_assert(sizeof(PlayerAliveWatcherCharacter) == 0x30);

/// The players of one character type (Mario, Luigi, ...) watched by PlayerAliveWatcher.
class PlayerAliveWatcherGroup {
    friend class PlayerAliveWatcherCharacter;

public:
    PlayerAliveWatcherGroup(PlayerAliveWatcher* pWatcher, const al::ActorInitInfo& rInfo,
                            const al::PlayerHolder* pHolder, s32 charaType);

    void handleAmiiboReassignment(PlayerAmiiboDirector* pDirector);
    void appear();
    void startDemo();
    void endDemo();
    void onGameOver();
    void startPause();
    void endPause();
    s32 tryCalcUserId() const;
    bool isAllDeactive() const;
    void deactivateAll();
    bool isAllDead() const;
    bool isAllDeadOrBubble() const;
    bool isGameOver() const;
    bool isEnableExitStage() const;
    bool isEnableIslandWarp() const;
    bool isWaitBubbleForRevive() const;
    s32 calcNoKillPlayerNum() const;
    bool isLastOne(const al::LiveActor* pActor) const;
    s32 calcActivePlayerNum() const;
    PlayerActor* findActivePlayerFirst() const;
    PlayerActor* tryFindActivePlayerFirst() const;
    PlayerActor* activatePlayer();
    bool isEnableOtherGroupBubbleWithInput() const;
    void changeShadowLight();
    void changeShadowNormal();
    bool update();

    /**
     * @brief Get the character type watched by this group.
     * @return The character type.
     */
    s32 getCharaType() const { return mCharaType; }

private:
    PlayerAliveWatcher* mWatcher;                   // 0x00
    PlayerAliveWatcherCharacter** mCharacters = nullptr;  // 0x08
    TractorBubble* mTractorBubble = nullptr;        // 0x10
    s32 mCharaType;                                 // 0x18
    s32 mCharacterNum = 0;                          // 0x1c
    bool mIsReviveRequested = false;                // 0x20
    u32 mOnGroundFrame = 0;                         // 0x24
};

static_assert(sizeof(PlayerAliveWatcherGroup) == 0x28);
