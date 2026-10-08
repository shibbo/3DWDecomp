#pragma once

#include <container/seadPtrArray.h>
#include "Library/Nerve/NerveStateBase.hpp"

namespace al {
class ActorInitInfo;
class AudioDirector;
class LayoutActor;
class LayoutInitInfo;
class PadRumbleDirector;
class SimpleLayoutAppear;
class SimpleLayoutAppearWait;
class WipeSimple;
}  // namespace al

class DemoSceneActorHolder;
class GameDataHolder;
class GameOverGraphicsState;
class GameOverMenu;
class SingleModeScene;

/**
 * @brief State of the Bowser's Fury scene playing the miss, the game over demo and the
 *        continue menu.
 */
class SingleModeStateGameOver : public al::NerveStateBase {
public:
    using KillLayoutArray = sead::FixedPtrArray<al::LayoutActor, 8>;

    SingleModeStateGameOver(SingleModeScene* pScene, const al::LayoutInitInfo& rLayoutInfo,
                            const al::ActorInitInfo& rActorInfo, GameDataHolder* pGameDataHolder,
                            al::AudioDirector* pAudioDirector, al::WipeSimple* pWipe,
                            al::SimpleLayoutAppearWait* pMissLayout);

    void appear() override;
    void control() override;

    void entryKillLayout(al::LayoutActor* pLayout);
    bool isContinue() const;
    bool isQuit() const;
    bool isDemo() const;
    void killAllLayouts();

    void exeWait() {}

    void exeMiss();
    void exeDemoGameOver();
    void exeGameOverMenu();
    void exeDemoContinue();
    void exeSave();
    void exeDone();

private:
    SingleModeScene* mScene = nullptr;                    // 0x18
    GameDataHolder* mGameDataHolder = nullptr;            // 0x20
    al::AudioDirector* mAudioDirector = nullptr;          // 0x28
    al::WipeSimple* mWipe = nullptr;                      // 0x30
    al::SimpleLayoutAppearWait* mMissLayout = nullptr;    // 0x38
    al::SimpleLayoutAppear* mGameOverLayout = nullptr;    // 0x40
    GameOverMenu* mGameOverMenu = nullptr;                // 0x48
    al::WipeSimple* mWipeFadeBlack = nullptr;             // 0x50
    DemoSceneActorHolder* mDemoHolder = nullptr;          // 0x58
    KillLayoutArray mKillLayouts;                         // 0x60
    GameOverGraphicsState* mGraphicsState = nullptr;      // 0xb0
    al::PadRumbleDirector* mPadRumbleDirector = nullptr;  // 0xb8
};

static_assert(sizeof(SingleModeStateGameOver) == 0xc0);
