#pragma once

#include <math/seadMatrix.h>

namespace al {
class ActorInitInfo;
class LiveActor;
struct PlacementInfo;
}  // namespace al
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
    void kill();
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
    /** @brief Gets the player actor count. @return Number of demo players. */
    int getPlayerCount() const { return mPlayerCount; }
    /** @brief Checks whether startup has completed. @return Startup completion flag. */
    bool isFullyStarted() const { return mIsFullyStarted; }

private:
    unsigned char _0[0x74];
    int mPlayerCount;
    unsigned char _78[8];
    bool mIsFullyStarted;
    unsigned char _81[0x88 - 0x81];
};
