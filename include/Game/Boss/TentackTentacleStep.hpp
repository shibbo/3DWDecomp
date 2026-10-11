#pragma once
#include <math/seadVector.h>
#include "Library/LiveActor/LiveActor.hpp"

class TentackTentacle;

/** @brief Scaffold block a Tentack tentacle carries up and throws off. */
class TentackTentacleStep : public al::LiveActor {
public:
    TentackTentacleStep(const char* pName, TentackTentacle* pTentacle,
                        const sead::Vector3f* pBasePos);
    void appearBreakable();
    void setBreak();
    void switchCollisionParts(bool isTentacleStiff);
    void startThrow(f32 speed);
    void startFall(f32 speed);
    bool isExistStep() const;
    bool isThrow() const;

private:
    unsigned char mUnknown148[0x20];
};
static_assert(sizeof(TentackTentacleStep) == 0x168);
