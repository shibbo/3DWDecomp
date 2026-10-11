#pragma once

#include "Library/Execute/IUseExecutor.hpp"
#include "Library/Scene/ISceneObj.hpp"

class GameDataHolder;
class GhostPlayerPlayer;
class GhostPlayerRecorder;

namespace al {
class ExecuteDirector;
class IUseSceneObjHolder;
class NetworkSystem;
class PlayerHolder;
struct SceneInitInfo;
}  // namespace al

/** @brief Scene object owning the ghost recorder (and the ghost player) of a stage. */
class GhostPlayerDirector : public al::ISceneObj, public al::IUseExecutor {
public:
    GhostPlayerDirector(const al::IUseSceneObjHolder* pHolder, const al::SceneInitInfo& rInfo,
                        al::ExecuteDirector* pExecuteDirector, al::PlayerHolder* pPlayerHolder,
                        const GameDataHolder* pGameDataHolder);

    void execute() override;
    bool tryStartRecord();
    bool isEndRecord() const;
    bool tryEndRecord(bool isClear);
    bool trySaveRecord();
    void tryStartPlay();
    void tryEndPlay();
    const char* getSceneObjName() const override;

    /**
     * @brief The ghost recorder of the stage.
     * @return The recorder, or nullptr if none was created.
     */
    GhostPlayerRecorder* getRecorder() const { return mRecorder; }

    /**
     * @brief The ghost player of the stage.
     * @return The ghost player, or nullptr if none was created.
     */
    GhostPlayerPlayer* getPlayer() const { return mPlayer; }

private:
    al::NetworkSystem* mNetworkSystem = nullptr;
    const GameDataHolder* mGameDataHolder;
    GhostPlayerRecorder* mRecorder = nullptr;
    GhostPlayerPlayer* mPlayer = nullptr;
};

static_assert(sizeof(GhostPlayerDirector) == 0x30);
