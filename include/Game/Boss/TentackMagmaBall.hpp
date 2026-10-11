#pragma once
#include "Boss/TentackRockBase.hpp"
#include "Library/LiveActor/LiveActor.hpp"

class TentackBase;

/** @brief Lava ball dropped onto the arena by the Tentack boss. */
class TentackMagmaBall : public al::LiveActor, public TentackRockBase {
public:
    TentackMagmaBall(const char* pName, TentackBase* pHost);
    void startFall(const sead::Vector3f& rPosition) override;
    bool isDeadRock() const override;
    void startFall(const sead::Vector2f* pPoint, f32 height);

private:
    unsigned char mUnknown150[0x10];
};
static_assert(sizeof(TentackMagmaBall) == 0x160);
