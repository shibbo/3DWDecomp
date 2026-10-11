#pragma once

#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"

/**
 * @brief A bush that shakes when hit and can hide an item or a rabbit.
 * @note Only what reconstructed code needs is declared so far.
 */
class Bush : public al::LiveActor {
public:
    explicit Bush(const char* pName);

    void initBushOnRabbit(const al::ActorInitInfo& rInfo, const sead::Vector3f& rTrans,
                          s32 color);
    bool tryStartRabbitAppear();

private:
    u8 mUnreconstructed[0x2c];
};

static_assert(sizeof(Bush) == 0x170);
