#pragma once
#include "Library/Scene/ISceneObj.hpp"
#include "Library/Execute/IUseExecutor.hpp"
namespace al {
class IUseSceneObjHolder;
struct SceneInitInfo;
class ExecuteDirector;
class PlayerHolder;
class NetworkSystem;
}
class GameDataHolder;
class GhostPlayerRecorder;
class GhostPlayerPlayer;

class GhostPlayerDirector : public al::ISceneObj, public al::IUseExecutor {
public:
    GhostPlayerDirector(const al::IUseSceneObjHolder*, const al::SceneInitInfo&,
                        al::ExecuteDirector*, al::PlayerHolder*, const GameDataHolder*);
    void execute() override;
    bool tryStartRecord();
    bool isEndRecord() const;
    bool tryEndRecord(bool);
    bool trySaveRecord();
    bool tryStartPlay();
    bool tryEndPlay();
    const char* getSceneObjName() const override;
    void registerPlayer(GhostPlayerPlayer* pPlayer) { mPlayer = pPlayer; }
    GhostPlayerRecorder* getRecorder() const { return mRecorder; }
    GhostPlayerPlayer* getPlayer() const { return mPlayer; }
private:
    al::NetworkSystem* mNetworkSystem = nullptr;
    const GameDataHolder* mGameData;
    GhostPlayerRecorder* mRecorder = nullptr;
    GhostPlayerPlayer* mPlayer = nullptr;
};
static_assert(sizeof(GhostPlayerDirector) == 0x30);
