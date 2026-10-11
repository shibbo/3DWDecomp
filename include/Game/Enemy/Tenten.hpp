#pragma once

#include "Library/LiveActor/LiveActor.hpp"
#include <math/seadVector.h>

class TentenGenerator;

/**
 * @brief Single Tenten (Biddybud) spawned and moved in formation by a TentenGenerator.
 * @note Minimal declaration: only what the reconstructed code uses is declared so far.
 */
class Tenten : public al::LiveActor {
public:
    Tenten(const char* pName, TentenGenerator* pGenerator);

    bool isDown() const;
    void moveSyncRailAddOffset(f32 speed);
    void setOffset(f32 offsetWidth, f32 offsetHeight);
    void startSupportFreeze(const al::LiveActor* pTouchActor);
    void endSupportFreeze();
    void setReverseFront();
    bool isHipDropDown() const;
    void reset();

    /** @brief Gets the front direction the Tenten is turning towards. */
    const sead::Vector3f& getTargetFront() const { return mTargetFront; }

private:
    u8 _144[0x2c];
    sead::Vector3f mTargetFront;
    u8 _17c[0x24];
};

static_assert(sizeof(Tenten) == 0x1a0);
