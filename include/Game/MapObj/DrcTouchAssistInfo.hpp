#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>

/**
 * @brief Touch-screen (DRC) state shared with touch-driven actors.
 * @note Minimal declaration: only the fields read by reconstructed code are named.
 *       Defined in DrcAssistDirector.
 */
class DrcTouchAssistInfo {
public:
    DrcTouchAssistInfo(bool, bool, bool, const sead::Vector3f&, const sead::Vector2f&);

    void reset();

    bool isTouch() const { return mIsTouch; }

    const sead::Vector3f& getTouchPos() const { return mTouchPos; }

    const sead::Vector2f& getScreenPos() const { return mScreenPos; }

    bool isUseScreenPos() const { return mIsUseScreenPos; }

    /// Position used by the touch cursor, in layout space.
    const sead::Vector2f& getLayoutPos() const { return _1c; }

    bool mIsTouch;              // 0x00
    bool _1;                    // 0x01
    bool _2;                    // 0x02
    bool _3;                    // 0x03
    sead::Vector3f mTouchPos;   // 0x04
    sead::Vector3f _10;         // 0x10
    sead::Vector2f _1c;         // 0x1C
    sead::Vector2f mScreenPos;  // 0x24
    sead::Vector2f _2c;         // 0x2C
    sead::Vector2f _34;         // 0x34
    sead::Vector2f _3c;         // 0x3C
    s32 _44;                    // 0x44
    bool _48;                   // 0x48
    bool _49;                   // 0x49
    bool mIsUseScreenPos;       // 0x4A
    bool _4b;                   // 0x4B
};

static_assert(sizeof(DrcTouchAssistInfo) == 0x4c);
