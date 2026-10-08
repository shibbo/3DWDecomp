#pragma once

#include <math/seadMatrix.h>

class DemoSceneActorHolder {
public:
    void startAction(int index, bool loop);
    void kill();
    bool isActionEndCamera(int frames) const;
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
};
