#pragma once

#include <math/seadMatrix.h>

namespace al {
class ActorInitInfo;
class LiveActor;
class GraphicsSystemInfo;
struct PlacementInfo;
}  // namespace al
class DemoSceneActor;
class DemoSceneActorPlayer;
class DemoEffectObj;
class DemoSceneCamera;
class DemoLayoutObj;
class DemoScenePlayer;
class IDemoScenePlayerInfo;
class PlayerRetargettingSelector;

class DemoSceneActorHolder {
public:
    explicit DemoSceneActorHolder(int maxPlayers);
    void initPlacementDemoActor(const al::PlacementInfo& rPlacement, const al::ActorInitInfo& rInfo,
                                PlayerRetargettingSelector* pSelector,
                                const sead::Matrix34f* pBaseMtx, bool isUseFigure);
    void initPlacementDemoLayout(const char* pName, const al::ActorInitInfo& rInfo);
    void setDemoScenePlayerInfo(IDemoScenePlayerInfo* pInfo);
    al::LiveActor* tryFindDemoSceneActor(const char* pName);
    void appear();
    void startAction(int index, bool loop);
    void tryStartActionByName(const char* pName);
    bool isAllActionEnd() const;
    void kill();
    void killAllEffect();
    void tryCancelEffect();
    bool isExistCamera() const;
    bool isExistLayout() const;
    al::LiveActor* getDemoScenePlayerActor(int index) const;
    al::LiveActor* getDemoScenePlayerActorModel(int index) const;
    al::LiveActor* tryFindPlayerModel(int index);
    bool isActionEndCamera(int frames) const;
    al::LiveActor* getDemoSceneActor(int index) const;
    bool isContainKoopaJr() const;
    void overrideBaseMtx(const sead::Matrix34f* pMtx);
    void setEndCameraInterpolateFrame(int frames);
    int getMaxCameraFrame() const;
    int getCurFrame() const;
    void update();
    void tryCancelAudio(int frames, bool flag);
    void tryCancel(bool flag);
    void setForceIgnoreCharId();
    void setGraphicsAreaFindByPlayerPos();
    void setCameraFovyDegree(float fovy);
    /** @brief Gets the actor count. @return Number of demo actors. */
    int getActorCount() const { return mActorCount; }
    /** @brief Gets the player actor count. @return Number of demo players. */
    int getPlayerCount() const { return mPlayerCount; }
    /** @brief Checks whether startup has completed. @return Startup completion flag. */
    bool isFullyStarted() const { return mIsFullyStarted; }

private:
    sead::Matrix34f mBaseMtx;
    DemoSceneActor** mActors = nullptr;
    DemoSceneActorPlayer** mPlayers = nullptr;
    DemoEffectObj** mEffects = nullptr;
    DemoSceneCamera* mCamera = nullptr;
    DemoLayoutObj* mLayout = nullptr;
    DemoScenePlayer* mScenePlayer = nullptr;
    void* mUnused = nullptr;
    al::GraphicsSystemInfo* mGraphics = nullptr;
    int mActorCount = 0;
    int mPlayerCount = 0;
    int mMaxPlayers;
    int mEffectCount = 0;
    bool mIsFullyStarted = false;
    bool mFindGraphicsByPlayer = false;
};

static_assert(sizeof(DemoSceneActorHolder) == 0x88);
