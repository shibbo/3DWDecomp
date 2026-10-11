#pragma once

#include <basis/seadTypes.h>

namespace al {
class CameraTargetBase;
class LiveActor;
}  // namespace al

/**
 * @brief Camera target following the main player (and, in Bowser's Fury, Fury Bowser).
 * @note Derives from al::CameraTargetBase in the game; only what reconstructed code needs is
 *       declared so far.
 */
class PlayerCameraTarget {
public:
    explicit PlayerCameraTarget(const al::LiveActor* pPlayer);

    /**
     * Gets the camera target base of this target.
     * @return The camera target (al::CameraTargetBase is the primary base at offset 0).
     */
    al::CameraTargetBase* getCameraTarget() {
        return reinterpret_cast<al::CameraTargetBase*>(this);
    }

    /**
     * Sets the player the camera follows.
     * @param pPlayer The player.
     */
    void setPlayer(const al::LiveActor* pPlayer) { mPlayer = pPlayer; }

    /**
     * Sets the boss the camera also keeps in view.
     * @param pBoss The boss actor.
     */
    void setBoss(const al::LiveActor* pBoss) { mBoss = pBoss; }

private:
    void* mVtable;                  // 0x00 (al::CameraTargetBase)
    u8 _8[0x10 - 0x8];
    const al::LiveActor* mPlayer;   // 0x10
    const al::LiveActor* mBoss;     // 0x18
    u8 _20[0x40 - 0x20];
};
static_assert(sizeof(PlayerCameraTarget) == 0x40);
