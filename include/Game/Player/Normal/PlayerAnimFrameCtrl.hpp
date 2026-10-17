#pragma once

#include <basis/seadTypes.h>
#include <prim/seadSafeString.h>

namespace al {
class LiveActor;
}

/// Keeps the frame of the player's main animation independently of the model playing it.
class PlayerAnimFrameCtrl {
public:
    PlayerAnimFrameCtrl();

    void startAction(al::LiveActor* pActor, const sead::SafeString& rName);
    void changeActionName(al::LiveActor* pActor, const sead::SafeString& rName);
    void update();
    void updateSync(al::LiveActor* pActor);
    void setFrame(f32 frame);
    const char* getActionName() const;
    f32 getCurrentFrame() const;
    f32 getRate() const;
    bool isActionEnd() const;

    /** @brief Gets the last frame of the animation. @return Max frame. */
    f32 getFrameMax() const { return mFrameMax; }

    /** @brief Sets the play rate. @param rate Frame rate. */
    void setRate(f32 rate) { mRate = rate; }

private:
    u8 _0[0xa0];
    f32 mFrameMax;  // 0xa0
    f32 mRate;      // 0xa4
    u8 _a8[0xb0 - 0xa8];
};

static_assert(sizeof(PlayerAnimFrameCtrl) == 0xb0);
