#pragma once

#include <basis/seadTypes.h>

/// Extends a jump (lower gravity) while the jump button stays held, up to a frame limit.
class PlayerJumpExtension {
public:
    PlayerJumpExtension(u32 maxFrame);

    void checkInput(bool isButtonOn);
    void reset();
    bool isEnableToStart() const;
    bool isExtend() const;

    void setMaxFrame(u32 maxFrame) { mMaxFrame = maxFrame; }
    bool isCanceled() const { return mIsCanceled; }

private:
    u32 mMaxFrame;     // 0x0
    u32 mFrame;        // 0x4
    bool mIsCanceled;  // 0x8, the button was released before the limit
};
