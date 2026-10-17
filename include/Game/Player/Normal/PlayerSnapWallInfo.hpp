#pragma once

#include <math/seadVector.h>

class PlayerFigureDirector;
struct PlayerProperty;

/// The wall the player snapped to this frame.
class PlayerSnapWallInfo {
public:
    PlayerSnapWallInfo();

    void clear();
    void record(PlayerProperty* pProperty, const sead::Vector3f& rPos,
                const sead::Vector3f& rNormal, const char* pWallCode);

    void setFigureDirector(const PlayerFigureDirector* pFigureDirector) {
        mFigureDirector = pFigureDirector;
    }

    bool isExist() const { return mIsExist; }

    const sead::Vector3f& getPos() const { return mPos; }

    const sead::Vector3f& getNormal() const { return mNormal; }

    const sead::Vector3f& getLastNormal() const { return mLastNormal; }

private:
    const PlayerFigureDirector* mFigureDirector;  // 0x0
    bool mIsExist;                                // 0x8
    u8 _9[0x1f];
    sead::Vector3f mPos;         // 0x28
    sead::Vector3f mNormal;      // 0x34
    sead::Vector3f mLastNormal;  // 0x40
    u8 _4c[0x4];
};

static_assert(sizeof(PlayerSnapWallInfo) == 0x50);
