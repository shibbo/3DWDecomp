#pragma once

#include <math/seadMatrix.h>
#include <prim/seadSafeString.h>

class PlayerModel;
class PlayerModelHolder;

namespace al {
class ActorInitInfo;
class IUseHioNode;
class LiveActor;
class PartsModel;
}
class PlayerRetargettingSelector;

/** @brief Factory interface for player models used by demo scenes. */
class DemoScenePlayerModel {
public:
    DemoScenePlayerModel();
    static DemoScenePlayerModel* createAll(const al::ActorInitInfo& rInfo, const char* pCharacter,
        PlayerRetargettingSelector* pSelector, al::IUseHioNode* pHost, const char* pSuffix);
    static DemoScenePlayerModel* createSingle(const al::ActorInitInfo& rInfo, const char* pCharacter,
        int figure, PlayerRetargettingSelector* pSelector, al::IUseHioNode* pHost, const char* pSuffix);
    void setDemoActor(const al::LiveActor* pActor);
    void initialize();
    void releaseDemoActor();
    void kill();
    void setFigure(int figure);
    PlayerModel* getPlayerModel();
    al::LiveActor* getModel();
    /** @brief Sets the animated placement to follow. @param pMtx Locator transform. */
    void setFollowMtx(const sead::Matrix34f* pMtx) { mFollowMtx = pMtx; }
    void startDemo(const sead::Matrix34f& rMtx);
    void startAction(const char* pName);
    void showShadow();
    void hideShadow();
    void killAllEffect();
    void update(al::LiveActor* pActor);
    void appear();
    void setSeMaterialCode(const char* pCode);
    void resetSeMaterialCode();
    void showSilhouette();
    void hideSilhouette();
    al::PartsModel* getClimbCloth();

private:
    friend class DemoScenePlayer;
    PlayerRetargettingSelector* mSelector = nullptr;
    PlayerModelHolder* mModels = nullptr;
    al::PartsModel* mClimbCloth = nullptr;
    const sead::Matrix34f* mFollowMtx = nullptr;
    int mStartDelay = 0;
    bool mActionStarted = false;
    sead::FixedSafeString<64> mActionName;
    const al::LiveActor* mDemoActor = nullptr;
};

static_assert(sizeof(DemoScenePlayerModel) == 0x88);
