#pragma once
#include <math/seadVector.h>
#include "Library/Scene/ISceneObj.hpp"
class IslandHolder;
namespace al {
class ActorInitInfo;
class IUseSceneObjHolder;
class LiveActor;
class LiveActorKit;
class StageInfo;
class StageResourceList;
}  // namespace al
class GameDataHolder;
class IntroFlyOverCamera;
class IslandKeeper : public al::ISceneObj {
public:
    explicit IslandKeeper(int);
    int getActiveIslandIndex() const { return mActiveIslandIndex; }
    int getUnknown14() const { return _14; }
    void* findIsland(int index) const;
    static IslandKeeper* tryGetIslandKeeper(const al::IUseSceneObjHolder* pHolder);
    void addActorLinkToIsland(al::LiveActor* pActor, int islandId);
    int getIndexHolderNum() const;
    IslandHolder* getIslandHolderIndex(int index) const;
    bool getIslandStartPos(int islandId, sead::Vector3f& rPos, sead::Vector3f& rFront);
    const al::ActorInitInfo* getIslandStartPos(int islandId);
    /** @brief Id of the island the player is currently on. @return The island id. */
    int getCurrentIslandId() const { return mCurrentIslandId; }
    void setIslandLODDisable(int islandId, bool isDisable);
    void tryUpdateLastIslandScenario();
    void init(al::LiveActorKit* pKit);
    void endInit(GameDataHolder* pGameDataHolder, IntroFlyOverCamera* pIntroCamera);
    void updateScenarios();
    bool tryTriggerIntro();
    void setActiveIslandIndex(int index);
    bool createIslandHolders(const al::StageInfo* pStageInfo, const al::ActorInitInfo& rInfo);
    void push(IslandHolder* pHolder);
    void setIslandStartPos(const al::StageResourceList* pList, const al::ActorInitInfo& rInfo);
    int getHolderNum() const;
    void update(int islandIndex);
private:
    int mCurrentIslandId;
    u8 mUnknownC[4];
    int mActiveIslandIndex;
    int _14;
    u8 mUnknown18[0x40];
};
static_assert(sizeof(IslandKeeper) == 0x58);
