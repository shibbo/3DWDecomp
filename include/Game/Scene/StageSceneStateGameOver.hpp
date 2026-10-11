#pragma once

#include "Library/Nerve/NerveStateBase.hpp"

namespace al {
class ActorInitInfo;
class AudioDirector;
class LayoutActor;
class LayoutInitInfo;
class SimpleLayoutAppearWait;
class WipeSimple;
}  // namespace al

class GameDataHolder;
class StageScene;

/**
 * @brief State of the stage scene playing the game over.
 * @note Minimal declaration: only what the reconstructed code uses is declared so far.
 */
class StageSceneStateGameOver : public al::NerveStateBase {
public:
    StageSceneStateGameOver(StageScene* pScene, const al::LayoutInitInfo& rLayoutInfo,
                            const al::ActorInitInfo& rActorInfo, GameDataHolder* pGameDataHolder,
                            al::AudioDirector* pAudioDirector, al::WipeSimple* pWipe,
                            al::SimpleLayoutAppearWait* pMissLayout,
                            const al::LayoutActor* pTimeUpLayout);

    void entryKillLayout(al::LayoutActor* pLayout);
    bool isDemo() const;
    bool isContinue() const;

private:
    u8 _pad[0xc8 - sizeof(al::NerveStateBase)];
};

static_assert(sizeof(StageSceneStateGameOver) == 0xc8);
