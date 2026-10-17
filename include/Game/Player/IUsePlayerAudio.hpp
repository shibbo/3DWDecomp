#pragma once

#include <prim/seadSafeString.h>

/// Sound effects of the player (implemented by PlayerAudio).
class IUsePlayerAudio {
public:
    virtual ~IUsePlayerAudio() = default;
    virtual void startSe(const sead::SafeString& rName) const = 0;
    virtual bool startSeOld(const sead::SafeString& rName) const = 0;
    virtual void holdSe(const sead::SafeString& rName) const = 0;
    virtual void stopSeOld(const sead::SafeString& rName, s32 fadeFrames) const = 0;
    virtual void stopSe(const sead::SafeString& rName) const = 0;
    virtual void stopAllSe(s32 fadeFrames) const = 0;
    virtual bool isInWater() = 0;
    virtual void tryUpdateMaterial(const char* pMaterialName) = 0;

    /** @brief Resets the flag selecting old or new sound effect versions (does nothing by default). */
    virtual void resetSeVersionFlag() {}
};
