#pragma once

namespace al {
class ActorInitInfo;
class IUseHioNode;
class LiveActor;
}
class PlayerRetargettingSelector;

/** @brief Factory interface for player models used by demo scenes. */
class DemoScenePlayerModel {
public:
    static DemoScenePlayerModel* createAll(const al::ActorInitInfo& rInfo, const char* pCharacter,
        PlayerRetargettingSelector* pSelector, al::IUseHioNode* pHost, const char* pSuffix);
    static DemoScenePlayerModel* createSingle(const al::ActorInitInfo& rInfo, const char* pCharacter,
        int figure, PlayerRetargettingSelector* pSelector, al::IUseHioNode* pHost, const char* pSuffix);
    void setDemoActor(const al::LiveActor* pActor);
    void initialize();
    void releaseDemoActor();
    void kill();
};
