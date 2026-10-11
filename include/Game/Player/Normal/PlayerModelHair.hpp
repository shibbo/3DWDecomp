#pragma once

#include <basis/seadTypes.h>

namespace al {
class LiveActor;
}

class PlayerModelWorldMtxCallbackHolder;

/// Hair controller of a player model (drives the hair sub actor).
class PlayerModelHair {
public:
    PlayerModelHair(al::LiveActor* pHairActor, const al::LiveActor* pParentActor,
                    PlayerModelWorldMtxCallbackHolder* pCallbackHolder);

    /** @brief Enables the hair control (if it isn't already). */
    void validate() {
        if (!mIsValid) {
            mIsValid = true;
        }
    }

    /** @brief Disables the hair control (if it isn't already). */
    void invalidate() {
        if (mIsValid) {
            mIsValid = false;
        }
    }

    /** @brief Checks if the hair control is enabled. @return True if enabled. */
    bool isValid() const { return mIsValid; }

private:
    u8 _0[0x10];
    bool mIsValid;  // 0x10
    u8 _11[0x28 - 0x11];
};
