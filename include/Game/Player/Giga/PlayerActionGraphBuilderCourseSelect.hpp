#pragma once

#include "Player/Giga/PlayerActionGraphBuilder.hpp"

namespace al {
struct ActorSceneInfo;
}  // namespace al

class CourseSelectPlayerActor;
class GameDataHolder;
class IUsePlayerPanicControl;
class PlayerConstParam;

/**
 * @brief Builds the action graph of the players walking on the course select map.
 * @note Minimal declaration: only what the reconstructed code uses is declared so far.
 */
class PlayerActionGraphBuilderCourseSelect : public IUsePlayerActionGraphBuilder {
public:
    PlayerActionGraphBuilderCourseSelect(const al::ActorSceneInfo* pSceneInfo,
                                         GameDataHolder* pGameDataHolder,
                                         CourseSelectPlayerActor* pPlayer,
                                         IUsePlayerPanicControl* pPanicControl,
                                         PlayerConstParam* pConstParam);
    PlayerActionGraph* create(Player* pPlayer) override;

private:
    const al::ActorSceneInfo* mSceneInfo;
    GameDataHolder* mGameDataHolder;
    CourseSelectPlayerActor* mPlayer;
    IUsePlayerPanicControl* mPanicControl;
    PlayerConstParam* mConstParam;
};
