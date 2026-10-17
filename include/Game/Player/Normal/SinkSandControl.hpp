#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>

class IUsePlayerCheckArea;
class PlayerSimpleFlag;
class PlayerConstParam;
struct PlayerProperty;

/// Sinking into quicksand and poison-free ink.
class SinkSandControl {
public:
    SinkSandControl(const IUsePlayerCheckArea*, const PlayerSimpleFlag*, const PlayerConstParam*,
                    const PlayerProperty*);

    void jump();
    void update(const sead::Vector3f&);
    void forceOff();
    bool isInSinkSandAreaStrict() const;
    bool isInInk() const;

    bool isInSinkSand() const { return mIsInSinkSand; }
    f32 getSurfaceHeight() const { return mSurfaceHeight; }

private:
    const IUsePlayerCheckArea* mCheckArea;  // 0x0
    const PlayerSimpleFlag* mFlag;          // 0x8
    const PlayerConstParam* mConstParam;    // 0x10
    const PlayerProperty* mProperty;        // 0x18
    bool mIsInSinkSand;                     // 0x20
    bool _21;
    f32 mSurfaceHeight;  // 0x24, height the player sinks towards
    u8 _28[0x30 - 0x28];
};
